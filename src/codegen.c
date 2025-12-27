#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "quickjsflow/codegen.h"

// Simple growable string buffer.
typedef struct {
    char *data;
    size_t len;
    size_t cap;
    int error;
} StrBuf;

typedef struct {
    size_t gen_off; // offset in generated code
    int src_line;   // 0-based
    int src_col;    // 0-based
} MapEntry;

typedef struct {
    MapEntry *items;
    size_t count;
    size_t cap;
} MapVec;

static void sb_init(StrBuf *sb) {
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
    sb->error = 0;
}

static int sb_reserve(StrBuf *sb, size_t need) {
    if (sb->error) return 0;
    if (need <= sb->cap) return 1;
    size_t cap = sb->cap ? sb->cap * 2 : 128;
    while (cap < need) cap *= 2;
    char *p = (char *)realloc(sb->data, cap);
    if (!p) {
        sb->error = 1;
        return 0;
    }
    sb->data = p;
    sb->cap = cap;
    return 1;
}

static int sb_append_char(StrBuf *sb, char c) {
    if (!sb_reserve(sb, sb->len + 2)) return 0;
    sb->data[sb->len++] = c;
    sb->data[sb->len] = '\0';
    return 1;
}

static int sb_append(StrBuf *sb, const char *s) {
    if (!s) return 1;
    size_t n = strlen(s);
    if (!sb_reserve(sb, sb->len + n + 1)) return 0;
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
    return 1;
}

static void sb_free(StrBuf *sb) {
    free(sb->data);
    sb->data = NULL;
    sb->len = sb->cap = 0;
    sb->error = 0;
}

static void mapvec_push(MapVec *v, size_t gen_off, int src_line, int src_col) {
    if (!v) return;
    if (v->count + 1 > v->cap) {
        size_t cap = v->cap ? v->cap * 2 : 16;
        MapEntry *items = (MapEntry *)realloc(v->items, cap * sizeof(MapEntry));
        if (!items) return;
        v->items = items;
        v->cap = cap;
    }
    v->items[v->count].gen_off = gen_off;
    v->items[v->count].src_line = src_line;
    v->items[v->count].src_col = src_col;
    v->count++;
}

static void mapvec_free(MapVec *v) {
    if (!v) return;
    free(v->items);
    v->items = NULL;
    v->count = v->cap = 0;
}

// --- codegen context -----------------------------------------------------

typedef struct {
    StrBuf buf;
    CodegenOptions opts;
    int indent_level;
    MapVec mappings;
    Comment **comments;
    size_t comment_count;
    size_t comment_index;
} CGCtx;

static void cg_init(CGCtx *cg, const CodegenOptions *opts) {
    sb_init(&cg->buf);
    if (opts) {
        cg->opts = *opts;
    } else {
        cg->opts.indent_width = 2;
        cg->opts.indent_char = ' ';
        cg->opts.emit_source_map = 0;
        cg->opts.source_name = NULL;
    }
    if (cg->opts.indent_width <= 0) cg->opts.indent_width = 2;
    if (cg->opts.indent_char == '\0') cg->opts.indent_char = ' ';
    cg->indent_level = 0;
    cg->mappings.items = NULL;
    cg->mappings.count = 0;
    cg->mappings.cap = 0;
    cg->comments = NULL;
    cg->comment_count = 0;
    cg->comment_index = 0;
}

static int cg_indent(CGCtx *cg) {
    int width = cg->opts.indent_width;
    for (int i = 0; i < cg->indent_level * width; ++i) {
        if (!sb_append_char(&cg->buf, cg->opts.indent_char)) return 0;
    }
    return 1;
}

static int cg_newline(CGCtx *cg) {
    return sb_append_char(&cg->buf, '\n');
}

// --- precedence helpers --------------------------------------------------

static int precedence_for_binary(const char *op) {
    if (!op) return 1;
    if (strcmp(op, "||") == 0) return 1;
    if (strcmp(op, "&&") == 0) return 2;
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) return 3;
    if (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0) return 4;
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) return 5;
    if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0 || strcmp(op, "%") == 0) return 6;
    return 2;
}

static int precedence_of(const AstNode *n) {
    if (!n) return 0;
    switch (n->type) {
        case AST_AssignmentExpression: return 0;
        case AST_BinaryExpression: {
            BinaryExpression *be = (BinaryExpression *)n->data;
            return precedence_for_binary(be ? be->operator : NULL);
        }
        case AST_UpdateExpression: return 7;
        case AST_UnaryExpression: return 8;
        case AST_MemberExpression: return 9;
        case AST_CallExpression: return 10;
        case AST_ArrayExpression:
        case AST_ObjectExpression:
        case AST_Literal:
        case AST_Identifier: return 11;
        default: return 1;
    }
}

static int pos_le(Position a, Position b) {
    if (a.line < b.line) return 1;
    if (a.line > b.line) return 0;
    return a.column <= b.column;
}

// --- VLQ helpers for source map ----------------------------------------

static int sb_append_vlq(StrBuf *sb, int value) {
    unsigned int v = (value < 0) ? ((-value) << 1) | 1 : (value << 1);
    static const char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    do {
        unsigned int digit = v & 31u; // 5 bits
        v >>= 5;
        if (v) digit |= 32u; // continuation
        if (!sb_append_char(sb, b64[digit & 63u])) return 0;
    } while (v);
    return 1;
}

static char *build_source_map(CGCtx *cg, const char *source_name) {
    if (!cg) return NULL;
    if (cg->mappings.count == 0) {
        StrBuf map; sb_init(&map);
        sb_append(&map, "{\"version\":3,\"sources\":[\"");
        sb_append(&map, source_name ? source_name : "input.js");
        sb_append(&map, "\"],\"mappings\":\"\"}");
        return map.error ? NULL : map.data;
    }

    size_t mcount = cg->mappings.count;
    int *gen_lines = (int *)calloc(mcount, sizeof(int));
    int *gen_cols = (int *)calloc(mcount, sizeof(int));
    if (!gen_lines || !gen_cols) { free(gen_lines); free(gen_cols); return NULL; }

    size_t pos = 0;
    int line = 0, col = 0;
    for (size_t i = 0; i < mcount; ++i) {
        size_t target = cg->mappings.items[i].gen_off;
        if (target > cg->buf.len) target = cg->buf.len;
        while (pos < target) {
            char ch = cg->buf.data[pos++];
            if (ch == '\n') { line++; col = 0; }
            else { col++; }
        }
        gen_lines[i] = line;
        gen_cols[i] = col;
    }

    StrBuf map; sb_init(&map);
    sb_append(&map, "{\"version\":3,\"sources\":[\"");
    sb_append(&map, source_name ? source_name : "input.js");
    sb_append(&map, "\"],\"mappings\":\"");

    size_t idx = 0;
    int last_gen_line = gen_lines[mcount - 1];
    int prev_source = 0;
    int prev_orig_line = 0;
    int prev_orig_col = 0;
    for (int l = 0; l <= last_gen_line; ++l) {
        if (l > 0) sb_append_char(&map, ';');
        int prev_gen_col = 0;
        int first_in_line = 1;
        while (idx < mcount && gen_lines[idx] == l) {
            MapEntry *me = &cg->mappings.items[idx];
            int gen_col = gen_cols[idx];
            int delta_gen_col = gen_col - prev_gen_col;
            int delta_source = 0 - prev_source; // only one source
            int delta_orig_line = me->src_line - prev_orig_line;
            int delta_orig_col = me->src_col - prev_orig_col;
            if (!first_in_line) sb_append_char(&map, ',');
            first_in_line = 0;
            sb_append_vlq(&map, delta_gen_col);
            sb_append_vlq(&map, delta_source);
            sb_append_vlq(&map, delta_orig_line);
            sb_append_vlq(&map, delta_orig_col);

            prev_gen_col = gen_col;
            prev_source = 0;
            prev_orig_line = me->src_line;
            prev_orig_col = me->src_col;
            idx++;
        }
    }

    sb_append(&map, "\"}");
    free(gen_lines);
    free(gen_cols);
    return map.error ? NULL : map.data;
}

// forward declarations
static int emit_statement(CGCtx *cg, const AstNode *n);
static int emit_expression(CGCtx *cg, const AstNode *n, int parent_prec);
static int emit_block(CGCtx *cg, const AstNode *block, int newline_after);

static void add_mapping(CGCtx *cg, const AstNode *n) {
    if (!cg || !n) return;
    int sl = n->start.line > 0 ? n->start.line - 1 : 0;
    int sc = n->start.column > 0 ? n->start.column - 1 : 0;
    mapvec_push(&cg->mappings, cg->buf.len, sl, sc);
}

static int emit_paren_expr(CGCtx *cg, const AstNode *n, int parent_prec) {
    int prec = precedence_of(n);
    if (prec < parent_prec) {
        if (!sb_append_char(&cg->buf, '(')) return 0;
        if (!emit_expression(cg, n, 0)) return 0;
        return sb_append_char(&cg->buf, ')');
    }
    return emit_expression(cg, n, prec);
}

// --- emit helpers --------------------------------------------------------

static int emit_identifier(CGCtx *cg, const AstNode *n) {
    if (!n || n->type != AST_Identifier) return 0;
    Identifier *id = (Identifier *)n->data;
    add_mapping(cg, n);
    return sb_append(&cg->buf, id && id->name ? id->name : "");
}

static int emit_literal(CGCtx *cg, const AstNode *n) {
    if (!n || n->type != AST_Literal) return 0;
    Literal *lit = (Literal *)n->data;
    add_mapping(cg, n);
    return sb_append(&cg->buf, lit && lit->raw ? lit->raw : "null");
}

static int emit_variable_declarator(CGCtx *cg, const AstNode *n) {
    if (!n || n->type != AST_VariableDeclarator) return 0;
    VariableDeclarator *vd = (VariableDeclarator *)n->data;
    add_mapping(cg, n);
    if (!emit_expression(cg, vd->id, precedence_of(vd->id))) return 0;
    if (vd->init) {
        if (!sb_append(&cg->buf, " = ")) return 0;
        if (!emit_expression(cg, vd->init, precedence_of(vd->init))) return 0;
    }
    return 1;
}

static int emit_variable_declaration(CGCtx *cg, const AstNode *n) {
    if (!n || n->type != AST_VariableDeclaration) return 0;
    VariableDeclaration *vd = (VariableDeclaration *)n->data;
    add_mapping(cg, n);
    const char *kw = "var";
    if (vd) {
        if (vd->kind == VD_Let) kw = "let";
        else if (vd->kind == VD_Const) kw = "const";
    }
    if (!sb_append(&cg->buf, kw)) return 0;
    if (!sb_append(&cg->buf, " ")) return 0;
    for (size_t i = 0; vd && i < vd->declarations.count; ++i) {
        if (i > 0 && !sb_append(&cg->buf, ", ")) return 0;
        if (!emit_variable_declarator(cg, vd->declarations.items[i])) return 0;
    }
    return sb_append_char(&cg->buf, ';');
}

static int emit_comment(CGCtx *cg, Comment *c) {
    if (!cg || !c) return 1;
    if (!cg_indent(cg)) return 0;
    if (c->is_block) {
        if (!sb_append(&cg->buf, "/*")) return 0;
        if (!sb_append(&cg->buf, c->text ? c->text : "")) return 0;
        if (!sb_append(&cg->buf, "*/")) return 0;
    } else {
        if (!sb_append(&cg->buf, "//")) return 0;
        if (!sb_append(&cg->buf, c->text ? c->text : "")) return 0;
    }
    return cg_newline(cg);
}

static int emit_comments_up_to(CGCtx *cg, Position limit) {
    while (cg->comment_index < cg->comment_count) {
        Comment *c = cg->comments[cg->comment_index];
        if (!pos_le(c->start, limit)) break;
        if (!emit_comment(cg, c)) return 0;
        cg->comment_index++;
    }
    return 1;
}

static int emit_expression(CGCtx *cg, const AstNode *n, int parent_prec) {
    if (!n) return sb_append(&cg->buf, "null");
    int t = n->type;
    if (t != AST_Identifier && t != AST_Literal && t != AST_VariableDeclarator) {
        add_mapping(cg, n);
    }
    switch (n->type) {
        case AST_Identifier: return emit_identifier(cg, n);
        case AST_Literal: return emit_literal(cg, n);
        case AST_VariableDeclarator: return emit_variable_declarator(cg, n);
        case AST_ExpressionStatement: {
            ExpressionStatement *es = (ExpressionStatement *)n->data;
            return emit_expression(cg, es ? es->expression : NULL, parent_prec);
        }
        case AST_UpdateExpression: {
            UpdateExpression *ue = (UpdateExpression *)n->data;
            int need_paren = precedence_of(n) < parent_prec;
            if (need_paren && !sb_append_char(&cg->buf, '(')) return 0;
            if (ue && ue->prefix) {
                if (!sb_append(&cg->buf, ue->operator ? ue->operator : "")) return 0;
                if (!emit_paren_expr(cg, ue->argument, precedence_of(n))) return 0;
            } else {
                if (!emit_paren_expr(cg, ue ? ue->argument : NULL, precedence_of(n))) return 0;
                if (!sb_append(&cg->buf, ue && ue->operator ? ue->operator : "")) return 0;
            }
            if (need_paren && !sb_append_char(&cg->buf, ')')) return 0;
            return 1;
        }
        case AST_UnaryExpression: {
            UnaryExpression *ue = (UnaryExpression *)n->data;
            int need_paren = precedence_of(n) < parent_prec;
            if (need_paren && !sb_append_char(&cg->buf, '(')) return 0;
            if (!sb_append(&cg->buf, ue && ue->operator ? ue->operator : "")) return 0;
            if (ue && ue->prefix && strcmp(ue->operator, "typeof") != 0 && strcmp(ue->operator, "void") != 0) {
                if (!sb_append_char(&cg->buf, ' ')) return 0;
            }
            if (!emit_paren_expr(cg, ue ? ue->argument : NULL, precedence_of(n))) return 0;
            if (need_paren && !sb_append_char(&cg->buf, ')')) return 0;
            return 1;
        }
        case AST_BinaryExpression: {
            BinaryExpression *be = (BinaryExpression *)n->data;
            int prec = precedence_of(n);
            int need_paren = prec < parent_prec;
            if (need_paren && !sb_append_char(&cg->buf, '(')) return 0;
            if (!emit_paren_expr(cg, be ? be->left : NULL, prec)) return 0;
            if (!sb_append(&cg->buf, " " )) return 0;
            if (!sb_append(&cg->buf, be && be->operator ? be->operator : "")) return 0;
            if (!sb_append(&cg->buf, " " )) return 0;
            if (!emit_paren_expr(cg, be ? be->right : NULL, prec)) return 0;
            if (need_paren && !sb_append_char(&cg->buf, ')')) return 0;
            return 1;
        }
        case AST_AssignmentExpression: {
            AssignmentExpression *ae = (AssignmentExpression *)n->data;
            int prec = precedence_of(n);
            int need_paren = prec < parent_prec;
            if (need_paren && !sb_append_char(&cg->buf, '(')) return 0;
            if (!emit_paren_expr(cg, ae ? ae->left : NULL, prec)) return 0;
            if (!sb_append(&cg->buf, " " )) return 0;
            if (!sb_append(&cg->buf, ae && ae->operator ? ae->operator : "=")) return 0;
            if (!sb_append(&cg->buf, " " )) return 0;
            if (!emit_paren_expr(cg, ae ? ae->right : NULL, prec)) return 0;
            if (need_paren && !sb_append_char(&cg->buf, ')')) return 0;
            return 1;
        }
        case AST_ArrayExpression: {
            ArrayExpression *arr = (ArrayExpression *)n->data;
            if (!sb_append_char(&cg->buf, '[')) return 0;
            if (arr) {
                for (size_t i = 0; i < arr->elements.count; ++i) {
                    if (i > 0 && !sb_append(&cg->buf, ", ")) return 0;
                    if (!emit_expression(cg, arr->elements.items[i], precedence_of(n))) return 0;
                }
            }
            return sb_append_char(&cg->buf, ']');
        }
        case AST_ObjectExpression: {
            ObjectExpression *oe = (ObjectExpression *)n->data;
            if (!sb_append_char(&cg->buf, '{')) return 0;
            if (oe) {
                for (size_t i = 0; i < oe->properties.count; ++i) {
                    if (i > 0 && !sb_append(&cg->buf, ", ")) return 0;
                    AstNode *prop_node = oe->properties.items[i];
                    Property *prop = prop_node ? (Property *)prop_node->data : NULL;
                    if (prop_node && !prop_node->data) return 0;
                    if (prop && prop->computed) {
                        if (!sb_append_char(&cg->buf, '[')) return 0;
                        if (!emit_expression(cg, prop->key, precedence_of(prop->key))) return 0;
                        if (!sb_append_char(&cg->buf, ']')) return 0;
                    } else {
                        if (prop && prop->key && prop->key->type == AST_Identifier) {
                            if (!emit_identifier(cg, prop->key)) return 0;
                        } else if (prop) {
                            if (!emit_expression(cg, prop->key, precedence_of(prop->key))) return 0;
                        }
                    }
                    if (!sb_append(&cg->buf, ": ")) return 0;
                    if (!emit_expression(cg, prop ? prop->value : NULL, precedence_of(prop ? prop->value : NULL))) return 0;
                }
            }
            return sb_append_char(&cg->buf, '}');
        }
        case AST_MemberExpression: {
            MemberExpression *me = (MemberExpression *)n->data;
            int need_paren = precedence_of(n) < parent_prec;
            if (need_paren && !sb_append_char(&cg->buf, '(')) return 0;
            if (!emit_paren_expr(cg, me ? me->object : NULL, precedence_of(n))) return 0;
            if (me && me->computed) {
                if (!sb_append_char(&cg->buf, '[')) return 0;
                if (!emit_expression(cg, me->property, precedence_of(n))) return 0;
                if (!sb_append_char(&cg->buf, ']')) return 0;
            } else {
                if (!sb_append_char(&cg->buf, '.')) return 0;
                if (!emit_expression(cg, me ? me->property : NULL, precedence_of(n))) return 0;
            }
            if (need_paren && !sb_append_char(&cg->buf, ')')) return 0;
            return 1;
        }
        case AST_CallExpression: {
            CallExpression *ce = (CallExpression *)n->data;
            int need_paren = precedence_of(n) < parent_prec;
            if (need_paren && !sb_append_char(&cg->buf, '(')) return 0;
            if (!emit_paren_expr(cg, ce ? ce->callee : NULL, precedence_of(n))) return 0;
            if (!sb_append_char(&cg->buf, '(')) return 0;
            if (ce) {
                for (size_t i = 0; i < ce->arguments.count; ++i) {
                    if (i > 0 && !sb_append(&cg->buf, ", ")) return 0;
                    if (!emit_expression(cg, ce->arguments.items[i], precedence_of(n))) return 0;
                }
            }
            if (!sb_append_char(&cg->buf, ')')) return 0;
            if (need_paren && !sb_append_char(&cg->buf, ')')) return 0;
            return 1;
        }
        case AST_FunctionDeclaration:
        case AST_FunctionExpression: {
            FunctionBody *fb = (FunctionBody *)n->data;
            if (!sb_append(&cg->buf, "function")) return 0;
            if (fb && fb->name) {
                if (!sb_append_char(&cg->buf, ' ')) return 0;
                if (!sb_append(&cg->buf, fb->name)) return 0;
            }
            if (!sb_append_char(&cg->buf, '(')) return 0;
            if (fb) {
                for (size_t i = 0; i < fb->params.count; ++i) {
                    if (i > 0 && !sb_append(&cg->buf, ", ")) return 0;
                    if (!emit_expression(cg, fb->params.items[i], precedence_of(fb->params.items[i]))) return 0;
                }
            }
            if (!sb_append_char(&cg->buf, ')')) return 0;
            if (!sb_append_char(&cg->buf, ' ')) return 0;
            if (!emit_block(cg, fb ? fb->body : NULL, 0)) return 0;
            return 1;
        }
        default:
            // Unsupported node types fall back to JSON printer pointer
            return sb_append(&cg->buf, "/* unsupported */");
    }
}

static int emit_statement(CGCtx *cg, const AstNode *n) {
    if (!n) return 1;
    switch (n->type) {
        case AST_ExpressionStatement: {
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            ExpressionStatement *es = (ExpressionStatement *)n->data;
            if (!emit_expression(cg, es ? es->expression : NULL, 0)) return 0;
            if (!sb_append_char(&cg->buf, ';')) return 0;
            return cg_newline(cg);
        }
        case AST_VariableDeclaration: {
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!emit_variable_declaration(cg, n)) return 0;
            return cg_newline(cg);
        }
        case AST_BlockStatement: {
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            return emit_block(cg, n, 1);
        }
        case AST_IfStatement: {
            IfStatement *is = (IfStatement *)n->data;
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!sb_append(&cg->buf, "if (")) return 0;
            if (!emit_expression(cg, is ? is->test : NULL, 0)) return 0;
            if (!sb_append(&cg->buf, ") ")) return 0;
            if (!emit_block(cg, is ? is->consequent : NULL, 0)) return 0;
            if (is && is->alternate) {
                if (!sb_append(&cg->buf, " else ")) return 0;
                if (!emit_block(cg, is->alternate, 0)) return 0;
            }
            return cg_newline(cg);
        }
        case AST_WhileStatement: {
            WhileStatement *ws = (WhileStatement *)n->data;
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!sb_append(&cg->buf, "while (")) return 0;
            if (!emit_expression(cg, ws ? ws->test : NULL, 0)) return 0;
            if (!sb_append(&cg->buf, ") ")) return 0;
            if (!emit_block(cg, ws ? ws->body : NULL, 0)) return 0;
            return cg_newline(cg);
        }
        case AST_DoWhileStatement: {
            DoWhileStatement *dw = (DoWhileStatement *)n->data;
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!sb_append(&cg->buf, "do ")) return 0;
            if (!emit_block(cg, dw ? dw->body : NULL, 0)) return 0;
            if (!sb_append(&cg->buf, " while (")) return 0;
            if (!emit_expression(cg, dw ? dw->test : NULL, 0)) return 0;
            if (!sb_append(&cg->buf, ");")) return 0;
            return cg_newline(cg);
        }
        case AST_ForStatement: {
            ForStatement *fs = (ForStatement *)n->data;
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!sb_append(&cg->buf, "for (")) return 0;
            if (fs && fs->init) {
                if (fs->init->type == AST_VariableDeclaration) {
                    if (!emit_variable_declaration(cg, fs->init)) return 0;
                } else {
                    if (!emit_expression(cg, fs->init, 0)) return 0;
                    if (!sb_append_char(&cg->buf, ';')) return 0;
                }
            } else {
                if (!sb_append(&cg->buf, ";")) return 0;
            }
            if (!sb_append_char(&cg->buf, ' ')) return 0;
            if (fs && fs->test) {
                if (!emit_expression(cg, fs->test, 0)) return 0;
            }
            if (!sb_append(&cg->buf, "; ")) return 0;
            if (fs && fs->update) {
                if (!emit_expression(cg, fs->update, 0)) return 0;
            }
            if (!sb_append(&cg->buf, ") ")) return 0;
            if (!emit_block(cg, fs ? fs->body : NULL, 0)) return 0;
            return cg_newline(cg);
        }
        case AST_ReturnStatement: {
            ReturnStatement *rs = (ReturnStatement *)n->data;
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!sb_append(&cg->buf, "return")) return 0;
            if (rs && rs->argument) {
                if (!sb_append_char(&cg->buf, ' ')) return 0;
                if (!emit_expression(cg, rs->argument, 0)) return 0;
            }
            if (!sb_append_char(&cg->buf, ';')) return 0;
            return cg_newline(cg);
        }
        case AST_ThrowStatement: {
            ThrowStatement *ts = (ThrowStatement *)n->data;
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!sb_append(&cg->buf, "throw ")) return 0;
            if (!emit_expression(cg, ts ? ts->argument : NULL, 0)) return 0;
            if (!sb_append_char(&cg->buf, ';')) return 0;
            return cg_newline(cg);
        }
        case AST_BreakStatement: {
            BreakStatement *bs = (BreakStatement *)n->data;
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!sb_append(&cg->buf, "break")) return 0;
            if (bs && bs->label) {
                if (!sb_append_char(&cg->buf, ' ')) return 0;
                if (!sb_append(&cg->buf, bs->label)) return 0;
            }
            if (!sb_append_char(&cg->buf, ';')) return 0;
            return cg_newline(cg);
        }
        case AST_ContinueStatement: {
            ContinueStatement *cs = (ContinueStatement *)n->data;
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!sb_append(&cg->buf, "continue")) return 0;
            if (cs && cs->label) {
                if (!sb_append_char(&cg->buf, ' ')) return 0;
                if (!sb_append(&cg->buf, cs->label)) return 0;
            }
            if (!sb_append_char(&cg->buf, ';')) return 0;
            return cg_newline(cg);
        }
        case AST_SwitchStatement: {
            SwitchStatement *ss = (SwitchStatement *)n->data;
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!sb_append(&cg->buf, "switch (")) return 0;
            if (!emit_expression(cg, ss ? ss->discriminant : NULL, 0)) return 0;
            if (!sb_append(&cg->buf, ") {")) return 0;
            if (!cg_newline(cg)) return 0;
            if (ss) {
                cg->indent_level++;
                for (size_t i = 0; i < ss->cases.count; ++i) {
                    AstNode *case_node = ss->cases.items[i];
                    SwitchCase *sc = case_node ? (SwitchCase *)case_node->data : NULL;
                    if (!cg_indent(cg)) return 0;
                    if (sc && sc->test) {
                        if (!sb_append(&cg->buf, "case ")) return 0;
                        if (!emit_expression(cg, sc->test, 0)) return 0;
                    } else {
                        if (!sb_append(&cg->buf, "default")) return 0;
                    }
                    if (!sb_append(&cg->buf, ":")) return 0;
                    if (!cg_newline(cg)) return 0;
                    cg->indent_level++;
                    if (sc) {
                        for (size_t j = 0; j < sc->consequent.count; ++j) {
                            if (!emit_statement(cg, sc->consequent.items[j])) return 0;
                        }
                    }
                    cg->indent_level--;
                }
                cg->indent_level--;
            }
            if (!cg_indent(cg)) return 0;
            if (!sb_append_char(&cg->buf, '}')) return 0;
            return cg_newline(cg);
        }
        case AST_TryStatement: {
            TryStatement *ts = (TryStatement *)n->data;
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!sb_append(&cg->buf, "try ")) return 0;
            if (!emit_block(cg, ts ? ts->block : NULL, 0)) return 0;
            if (ts) {
                for (size_t i = 0; i < ts->handlers.count; ++i) {
                    AstNode *handler_node = ts->handlers.items[i];
                    CatchClause *cc = handler_node ? (CatchClause *)handler_node->data : NULL;
                    if (!sb_append_char(&cg->buf, ' ')) return 0;
                    if (!sb_append(&cg->buf, "catch (")) return 0;
                    if (!emit_expression(cg, cc ? cc->param : NULL, 0)) return 0;
                    if (!sb_append(&cg->buf, ") ")) return 0;
                    if (!emit_block(cg, cc ? cc->body : NULL, 0)) return 0;
                }
                if (ts->finalizer) {
                    if (!sb_append_char(&cg->buf, ' ')) return 0;
                    if (!sb_append(&cg->buf, "finally ")) return 0;
                    if (!emit_block(cg, ts->finalizer, 0)) return 0;
                }
            }
            return cg_newline(cg);
        }
        case AST_FunctionDeclaration: {
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!emit_expression(cg, n, 0)) return 0;
            return cg_newline(cg);
        }
        case AST_ImportDeclaration: {
            ImportDeclaration *id = (ImportDeclaration *)n->data;
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!sb_append(&cg->buf, "import {")) return 0;
            if (id) {
                for (size_t i = 0; i < id->specifiers.count; ++i) {
                    if (i > 0 && !sb_append(&cg->buf, ", ")) return 0;
                    ImportSpecifier *is = (ImportSpecifier *)id->specifiers.items[i]->data;
                    if (!emit_expression(cg, is ? is->imported : NULL, 0)) return 0;
                    if (is && is->local && is->local != is->imported) {
                        if (!sb_append(&cg->buf, " as ")) return 0;
                        if (!emit_expression(cg, is->local, 0)) return 0;
                    }
                }
                if (!sb_append(&cg->buf, "} from \"")) return 0;
                if (id->source && !sb_append(&cg->buf, id->source)) return 0;
                if (!sb_append(&cg->buf, "\";")) return 0;
            } else {
                if (!sb_append(&cg->buf, "} from \"\";")) return 0;
            }
            return cg_newline(cg);
        }
        case AST_ExportNamedDeclaration: {
            ExportNamedDeclaration *ed = (ExportNamedDeclaration *)n->data;
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!sb_append(&cg->buf, "export ")) return 0;
            if (ed && ed->declaration) {
                if (!emit_statement(cg, ed->declaration)) return 0;
                return 1;
            }
            if (!sb_append(&cg->buf, "{")) return 0;
            if (ed) {
                for (size_t i = 0; i < ed->specifiers.count; ++i) {
                    if (i > 0 && !sb_append(&cg->buf, ", ")) return 0;
                    if (!emit_expression(cg, ed->specifiers.items[i], 0)) return 0;
                }
            }
            if (!sb_append(&cg->buf, "}")) return 0;
            if (ed && ed->source) {
                if (!sb_append(&cg->buf, " from \"")) return 0;
                if (!sb_append(&cg->buf, ed->source)) return 0;
                if (!sb_append_char(&cg->buf, '"')) return 0;
            }
            if (!sb_append_char(&cg->buf, ';')) return 0;
            return cg_newline(cg);
        }
        case AST_ExportDefaultDeclaration: {
            ExportDefaultDeclaration *ed = (ExportDefaultDeclaration *)n->data;
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!sb_append(&cg->buf, "export default ")) return 0;
            if (ed && ed->declaration) {
                if (!emit_expression(cg, ed->declaration, 0)) return 0;
            } else if (ed && ed->expression) {
                if (!emit_expression(cg, ed->expression, 0)) return 0;
            }
            if (!sb_append_char(&cg->buf, ';')) return 0;
            return cg_newline(cg);
        }
        default:
            if (!cg_indent(cg)) return 0;
            add_mapping(cg, n);
            if (!sb_append(&cg->buf, "/* unsupported statement */")) return 0;
            return cg_newline(cg);
    }
}

static int emit_block(CGCtx *cg, const AstNode *block, int newline_after) {
    if (block && block->type != AST_BlockStatement) {
        // For single non-block statements, wrap in braces for safety
        if (!sb_append(&cg->buf, "{\n")) return 0;
        cg->indent_level++;
        if (!emit_statement(cg, block)) return 0;
        cg->indent_level--;
        if (!cg_indent(cg)) return 0;
        if (!sb_append_char(&cg->buf, '}')) return 0;
        if (newline_after) return cg_newline(cg);
        return 1;
    }
    BlockStatement *bs = block ? (BlockStatement *)block->data : NULL;
    if (!sb_append(&cg->buf, "{\n")) return 0;
    cg->indent_level++;
    if (bs) {
        for (size_t i = 0; i < bs->body.count; ++i) {
            AstNode *stmt = bs->body.items[i];
            if (stmt && !emit_comments_up_to(cg, stmt->start)) return 0;
            if (!emit_statement(cg, stmt)) return 0;
            if (stmt) {
                Position line_tail = { stmt->end.line, INT_MAX };
                if (!emit_comments_up_to(cg, line_tail)) return 0;
            }
        }
        // flush comments that belong to this block
        Position block_end = block ? block->end : (Position){ INT_MAX, INT_MAX };
        if (!emit_comments_up_to(cg, block_end)) return 0;
    }
    cg->indent_level--;
    if (!cg_indent(cg)) return 0;
    if (!sb_append_char(&cg->buf, '}')) return 0;
    if (newline_after) return cg_newline(cg);
    return 1;
}

// --- public API ----------------------------------------------------------

CodegenResult codegen_generate(const AstNode *root, const CodegenOptions *options) {
    CGCtx cg;
    cg_init(&cg, options);

    CodegenResult res = { NULL, NULL };
    if (!root) return res;

    if (root->type == AST_Program) {
        Program *pr = (Program *)root->data;
        cg.comments = pr ? pr->comments : NULL;
        cg.comment_count = pr ? pr->comment_count : 0;
        cg.comment_index = 0;
        if (pr) {
            for (size_t i = 0; i < pr->body.count; ++i) {
                AstNode *stmt = pr->body.items[i];
                if (stmt && !emit_comments_up_to(&cg, stmt->start)) {
                    sb_free(&cg.buf);
                    mapvec_free(&cg.mappings);
                    return res;
                }
                if (!emit_statement(&cg, stmt)) {
                    sb_free(&cg.buf);
                    mapvec_free(&cg.mappings);
                    return res;
                }
                if (stmt) {
                    Position line_tail = { stmt->end.line, INT_MAX };
                    if (!emit_comments_up_to(&cg, line_tail)) {
                        sb_free(&cg.buf);
                        mapvec_free(&cg.mappings);
                        return res;
                    }
                }
            }
            // flush trailing comments
            Position tail = { INT_MAX, INT_MAX };
            if (!emit_comments_up_to(&cg, tail)) {
                sb_free(&cg.buf);
                mapvec_free(&cg.mappings);
                return res;
            }
        }
    } else {
        // Treat as expression program
        if (!emit_expression(&cg, root, 0)) {
            sb_free(&cg.buf);
            mapvec_free(&cg.mappings);
            return res;
        }
    }

    if (cg.buf.error) {
        sb_free(&cg.buf);
        mapvec_free(&cg.mappings);
        return res;
    }

    res.code = cg.buf.data;
    if (cg.opts.emit_source_map) {
        res.source_map = build_source_map(&cg, cg.opts.source_name);
    }

    mapvec_free(&cg.mappings);
    return res;
}

void codegen_result_free(CodegenResult *result) {
    if (!result) return;
    free(result->code);
    free(result->source_map);
    result->code = NULL;
    result->source_map = NULL;
}
