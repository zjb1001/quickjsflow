#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "quickjsflow/parser.h"

// keyword helper
static int is_keyword(Token *t, const char *kw) {
    return t->type == TOKEN_IDENTIFIER && t->lexeme && strcmp(t->lexeme, kw) == 0;
}

// punctuation helper
static int is_punct(Token *t, const char *s) {
    return t->type == TOKEN_PUNCTUATOR && t->lexeme && strcmp(t->lexeme, s) == 0;
}

static Position pos_start(Token *t) { Position p = { t->start_line, t->start_col }; return p; }
static Position pos_end(Token *t) { Position p = { t->end_line, t->end_col }; return p; }

static Token next_tok(Parser *p) {
    if (p->has_lookahead) {
        p->has_lookahead = 0;
        return p->lookahead;
    }
    return lexer_next(&p->lx);
}

static Token peek_tok(Parser *p) {
    if (!p->has_lookahead) {
        p->lookahead = lexer_next(&p->lx);
        p->has_lookahead = 1;
    }
    return p->lookahead;
}

void parser_init(Parser *p, const char *input, size_t length) {
    lexer_init(&p->lx, input, length);
    p->has_lookahead = 0;
    p->comment_sink = NULL;
}

// forward decls
static AstNode *parse_expression(Parser *p);
static AstNode *parse_statement(Parser *p);
static AstNode *parse_variable_declaration(Parser *p, VarKind kind);
static AstNode *parse_function(Parser *p, int is_decl);

// Phase 2: Modern Features
#ifdef ENABLE_ARROW_FUNCTION_PARSING
static AstNode *parse_arrow_function(Parser *p, AstNode *param_or_params);
#endif
static AstNode *parse_class(Parser *p, int is_decl);
static AstNode *parse_template_literal(Parser *p);

static char *dup_unquoted_string(const Token *tok) {
    const char *lex = (tok && tok->lexeme) ? tok->lexeme : "";
    size_t len = strlen(lex);
    if (len >= 2 && ((lex[0] == '"' && lex[len - 1] == '"') || (lex[0] == '\'' && lex[len - 1] == '\''))) {
        size_t n = len - 2;
        char *out = (char *)malloc(n + 1);
        if (!out) return NULL;
        memcpy(out, lex + 1, n);
        out[n] = '\0';
        return out;
    }
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, lex, len + 1);
    return out;
}

static void record_comment(Parser *p, const Token *tok) {
    if (!p || !tok || !p->comment_sink) return;
    Comment *c = (Comment *)calloc(1, sizeof(Comment));
    if (!c) return;
    c->is_block = (tok->type == TOKEN_COMMENT_BLOCK);
    if (tok->lexeme) {
        const char *lex = tok->lexeme;
        size_t len = strlen(lex);
        size_t start = 0, end = len;
        if (len >= 2 && lex[0] == '/' && lex[1] == '/') start = 2;
        else if (len >= 4 && lex[0] == '/' && lex[1] == '*') { start = 2; if (lex[len - 2] == '*' && lex[len - 1] == '/') end = len - 2; }
        size_t out_len = (end > start) ? (end - start) : 0;
        c->text = (char *)malloc(out_len + 1);
        if (c->text) {
            memcpy(c->text, lex + start, out_len);
            c->text[out_len] = '\0';
        }
    }
    c->start = pos_start((Token *)tok);
    c->end = pos_end((Token *)tok);
    commentvec_push(p->comment_sink, c);
}

static int expect_punct(Parser *p, const char *s, Token *out) {
    Token t = peek_tok(p);
    if (!is_punct(&t, s)) return 0;
    next_tok(p);
    if (out) *out = t; else token_free(&t);
    return 1;
}

// block statement { ... }
static AstNode *parse_block(Parser *p) {
    Token lbrace;
    if (!expect_punct(p, "{", &lbrace)) {
        return ast_error("ExpectedBlockOpen", pos_start(&lbrace), pos_end(&lbrace));
    }
    Position s = pos_start(&lbrace);
    AstNode *blk = ast_block_statement(s, s);
    BlockStatement *bs = (BlockStatement *)blk->data;
    for (;;) {
        Token t = peek_tok(p);
        if (t.type == TOKEN_EOF) break;
        if (is_punct(&t, "}")) { next_tok(p); blk->end = pos_end(&t); break; }
        // skip comments but record them
        if (t.type == TOKEN_COMMENT_LINE || t.type == TOKEN_COMMENT_BLOCK) { Token ct = next_tok(p); record_comment(p, &ct); token_free(&ct); continue; }
        AstNode *stmt = parse_statement(p);
        if (!stmt) break;
        astvec_push(&bs->body, stmt);
    }
    token_free(&lbrace);
    return blk;
}

static AstNode *parse_function(Parser *p, int is_decl) {
    Token ft = next_tok(p); // consume 'function'
    Position s = pos_start(&ft);

    Token name_tok = peek_tok(p);
    char *name_cstr = NULL;
    int has_name = 0;
    if (name_tok.type == TOKEN_IDENTIFIER) {
        has_name = 1;
        next_tok(p);
        name_cstr = name_tok.lexeme;
    }
    // For declarations, name is required
    if (is_decl && !has_name) {
        return ast_error("ExpectedFunctionName", s, s);
    }

    Token lparen;
    if (!expect_punct(p, "(", &lparen)) return ast_error("ExpectedOpenParen", s, s);
    AstVec params; astvec_init(&params);
    Token t = peek_tok(p);
    if (!is_punct(&t, ")")) {
        for (;;) {
            Token ptok = peek_tok(p);
            if (ptok.type != TOKEN_IDENTIFIER) return ast_error("ExpectedParam", pos_start(&ptok), pos_end(&ptok));
            Token pid = next_tok(p);
            AstNode *pidn = ast_identifier(pid.lexeme, pos_start(&pid), pos_end(&pid));
            astvec_push(&params, pidn);
            Token comma = peek_tok(p);
            if (!is_punct(&comma, ",")) break;
            next_tok(p);
        }
    }
    Token rparen;
    if (!expect_punct(p, ")", &rparen)) return ast_error("ExpectedCloseParen", pos_start(&rparen), pos_end(&rparen));

    AstNode *body = parse_block(p);
    Position e = body ? body->end : pos_end(&rparen);
    AstNode *fn = is_decl ? ast_function_declaration(name_cstr ? name_cstr : "", s, e)
                           : ast_function_expression(name_cstr, s, e);
    FunctionBody *fb = (FunctionBody *)fn->data;
    fb->params = params; // shallow move
    fb->body = body;
    token_free(&ft);
    if (has_name) token_free(&name_tok);
    return fn;
}

static AstNode *parse_if(Parser *p) {
    Token ift = next_tok(p);
    Position s = pos_start(&ift);
    if (!expect_punct(p, "(", NULL)) return ast_error("ExpectedOpenParen", s, s);
    AstNode *test = parse_expression(p);
    Token rparen;
    if (!expect_punct(p, ")", &rparen)) return ast_error("ExpectedCloseParen", pos_start(&rparen), pos_end(&rparen));
    AstNode *cons = parse_statement(p);
    Token t = peek_tok(p);
    AstNode *alt = NULL;
    if (is_keyword(&t, "else")) { next_tok(p); alt = parse_statement(p); }
    Position e = alt ? alt->end : (cons ? cons->end : pos_end(&rparen));
    return ast_if_statement(test, cons, alt, s, e);
}

static AstNode *parse_while(Parser *p) {
    Token wt = next_tok(p);
    Position s = pos_start(&wt);
    if (!expect_punct(p, "(", NULL)) return ast_error("ExpectedOpenParen", s, s);
    AstNode *test = parse_expression(p);
    Token rparen;
    if (!expect_punct(p, ")", &rparen)) return ast_error("ExpectedCloseParen", pos_start(&rparen), pos_end(&rparen));
    AstNode *body = parse_statement(p);
    Position e = body ? body->end : pos_end(&rparen);
    return ast_while_statement(test, body, s, e);
}

static AstNode *parse_do_while(Parser *p) {
    Token dt = next_tok(p);
    Position s = pos_start(&dt);
    AstNode *body = parse_statement(p);
    Token wt = peek_tok(p);
    if (!is_keyword(&wt, "while")) return ast_error("ExpectedWhile", pos_start(&wt), pos_end(&wt));
    next_tok(p);
    if (!expect_punct(p, "(", NULL)) return ast_error("ExpectedOpenParen", pos_start(&wt), pos_end(&wt));
    AstNode *test = parse_expression(p);
    Token rparen;
    if (!expect_punct(p, ")", &rparen)) return ast_error("ExpectedCloseParen", pos_start(&rparen), pos_end(&rparen));
    // optional trailing ;
    Token semi = peek_tok(p);
    if (is_punct(&semi, ";")) next_tok(p);
    Position e = body ? body->end : pos_end(&rparen);
    return ast_do_while_statement(body, test, s, e);
}

static AstNode *parse_switch(Parser *p) {
    Token st = next_tok(p);
    Position s = pos_start(&st);
    if (!expect_punct(p, "(", NULL)) return ast_error("ExpectedOpenParen", s, s);
    AstNode *disc = parse_expression(p);
    if (!expect_punct(p, ")", NULL)) return ast_error("ExpectedCloseParen", s, s);
    if (!expect_punct(p, "{", NULL)) return ast_error("ExpectedOpenBrace", s, s);
    AstNode *sw = ast_switch_statement(disc, s, s);
    SwitchStatement *ss = (SwitchStatement *)sw->data;
    for (;;) {
        Token t = peek_tok(p);
        if (is_punct(&t, "}")) { next_tok(p); sw->end = pos_end(&t); break; }
        if (is_keyword(&t, "case")) {
            next_tok(p);
            AstNode *test = parse_expression(p);
            if (!expect_punct(p, ":", NULL)) return ast_error("ExpectedColon", pos_start(&t), pos_end(&t));
            AstNode *scn = ast_switch_case(test);
            SwitchCase *scd = (SwitchCase *)scn->data;
            for (;;) {
                Token tt = peek_tok(p);
                if (tt.type == TOKEN_EOF || is_punct(&tt, "}" ) || is_keyword(&tt, "case") || is_keyword(&tt, "default")) break;
                AstNode *stn = parse_statement(p);
                if (!stn) break;
                astvec_push(&scd->consequent, stn);
            }
            astvec_push(&ss->cases, scn);
            continue;
        }
        if (is_keyword(&t, "default")) {
            next_tok(p);
            if (!expect_punct(p, ":", NULL)) return ast_error("ExpectedColon", pos_start(&t), pos_end(&t));
            AstNode *scn = ast_switch_case(NULL);
            SwitchCase *scd = (SwitchCase *)scn->data;
            for (;;) {
                Token tt = peek_tok(p);
                if (tt.type == TOKEN_EOF || is_punct(&tt, "}" ) || is_keyword(&tt, "case") || is_keyword(&tt, "default")) break;
                AstNode *stn = parse_statement(p);
                if (!stn) break;
                astvec_push(&scd->consequent, stn);
            }
            astvec_push(&ss->cases, scn);
            continue;
        }
        // unexpected, bail
        return ast_error("ExpectedCase", pos_start(&t), pos_end(&t));
    }
    return sw;
}

static AstNode *parse_try(Parser *p) {
    Token tt = next_tok(p);
    Position s = pos_start(&tt);
    AstNode *block = parse_block(p);
    AstNode *try_stmt = ast_try_statement(block, s, s);
    TryStatement *ts = (TryStatement *)try_stmt->data;

    Token t = peek_tok(p);
    if (is_keyword(&t, "catch")) {
        next_tok(p);
        if (!expect_punct(p, "(", NULL)) return ast_error("ExpectedOpenParen", s, s);
        Token idt = peek_tok(p);
        AstNode *param = NULL;
        if (idt.type == TOKEN_IDENTIFIER) {
            next_tok(p);
            param = ast_identifier(idt.lexeme, pos_start(&idt), pos_end(&idt));
            token_free(&idt);
        } else {
            return ast_error("ExpectedCatchParam", pos_start(&idt), pos_end(&idt));
        }
        if (!expect_punct(p, ")", NULL)) return ast_error("ExpectedCloseParen", s, s);
        AstNode *cb = parse_block(p);
        AstNode *cc = ast_catch_clause(param, cb);
        astvec_push(&ts->handlers, cc);
    }

    t = peek_tok(p);
    if (is_keyword(&t, "finally")) {
        next_tok(p);
        ts->finalizer = parse_block(p);
    }

    Position e = block ? block->end : s;
    if (ts->finalizer) e = ts->finalizer->end;
    try_stmt->end = e;
    return try_stmt;
}

static AstNode *parse_throw(Parser *p) {
    Token th = next_tok(p);
    Position s = pos_start(&th);
    AstNode *arg = parse_expression(p);
    Token semi = peek_tok(p);
    if (is_punct(&semi, ";")) next_tok(p);
    Position e = arg ? arg->end : pos_end(&th);
    return ast_throw_statement(arg, s, e);
}

static AstNode *parse_import(Parser *p) {
    Token it = next_tok(p);
    Position s = pos_start(&it);
    AstNode *imp = ast_import_declaration("", s, s);
    ImportDeclaration *id = (ImportDeclaration *)imp->data;

    Token t = peek_tok(p);
    
    // default import: import defaultExport from 'module'
    if (t.type == TOKEN_IDENTIFIER) {
        Token def = next_tok(p);
        AstNode *local = ast_identifier(def.lexeme, pos_start(&def), pos_end(&def));
        AstNode *spec = ast_import_default_specifier(local, pos_start(&def), pos_end(&def));
        astvec_push(&id->specifiers, spec);
        token_free(&def);
        
        // Check for comma (mixed import: import defaultExport, { named } from 'module')
        Token comma = peek_tok(p);
        if (is_punct(&comma, ",")) {
            next_tok(p);
            t = peek_tok(p);
        }
    }
    
    // namespace import: import * as name from 'module'
    if (is_punct(&t, "*")) {
        next_tok(p); // consume *
        Token as_tok = peek_tok(p);
        if (!is_keyword(&as_tok, "as")) return ast_error("ExpectedAs", pos_start(&as_tok), pos_end(&as_tok));
        next_tok(p); // consume 'as'
        
        Token name = peek_tok(p);
        if (name.type != TOKEN_IDENTIFIER) return ast_error("ExpectedIdentifier", pos_start(&name), pos_end(&name));
        next_tok(p);
        
        AstNode *local = ast_identifier(name.lexeme, pos_start(&name), pos_end(&name));
        AstNode *spec = ast_import_namespace_specifier(local, pos_start(&name), pos_end(&name));
        astvec_push(&id->specifiers, spec);
        token_free(&name);
        t = peek_tok(p);
    }

    // named imports: import { x, y } from 'module'
    t = peek_tok(p);
    if (is_punct(&t, "{")) {
        next_tok(p);
        Token nt = peek_tok(p);
        if (!is_punct(&nt, "}")) {
            for (;;) {
                Token ntok = peek_tok(p);
                if (ntok.type != TOKEN_IDENTIFIER) return ast_error("ExpectedImportSpecifier", pos_start(&ntok), pos_end(&ntok));
                Token name = next_tok(p);
                AstNode *imported = ast_identifier(name.lexeme, pos_start(&name), pos_end(&name));
                AstNode *local = ast_identifier(name.lexeme, pos_start(&name), pos_end(&name));
                
                // Check for 'as' alias: import { x as y }
                Token as_check = peek_tok(p);
                if (is_keyword(&as_check, "as")) {
                    next_tok(p); // consume 'as'
                    Token alias = peek_tok(p);
                    if (alias.type != TOKEN_IDENTIFIER) return ast_error("ExpectedIdentifier", pos_start(&alias), pos_end(&alias));
                    next_tok(p);
                    ast_release(local);
                    local = ast_identifier(alias.lexeme, pos_start(&alias), pos_end(&alias));
                    token_free(&alias);
                }
                
                AstNode *spec = ast_import_specifier(imported, local);
                astvec_push(&id->specifiers, spec);
                token_free(&name);
                Token comma = peek_tok(p);
                if (!is_punct(&comma, ",")) break;
                next_tok(p);
            }
        }
        if (!expect_punct(p, "}", NULL)) return ast_error("ExpectedCloseBrace", pos_start(&t), pos_end(&t));
    }

    // from "module"
    Token fromt = peek_tok(p);
    if (!is_keyword(&fromt, "from")) return ast_error("ExpectedFrom", pos_start(&fromt), pos_end(&fromt));
    next_tok(p);
    Token src = peek_tok(p);
    if (src.type != TOKEN_STRING) return ast_error("ExpectedModuleString", pos_start(&src), pos_end(&src));
    next_tok(p);
    free(id->source);
    id->source = dup_unquoted_string(&src);
    token_free(&src);
    Token semi = peek_tok(p);
    if (is_punct(&semi, ";")) next_tok(p);
    imp->end = pos_end(&src);
    return imp;
}

static AstNode *parse_export(Parser *p) {
    Token et = next_tok(p);
    Position s = pos_start(&et);
    Token t = peek_tok(p);
    if (is_keyword(&t, "default")) {
        next_tok(p);
        Token ft = peek_tok(p);
        AstNode *decl = NULL;
        AstNode *expr = NULL;
        if (is_keyword(&ft, "function")) {
            decl = parse_function(p, 0);
        } else {
            expr = parse_expression(p);
        }
        Token semi = peek_tok(p);
        if (is_punct(&semi, ";")) next_tok(p);
        AstNode *ed = ast_export_default_declaration(s, expr ? expr->end : (decl ? decl->end : s));
        ExportDefaultDeclaration *edd = (ExportDefaultDeclaration *)ed->data;
        edd->declaration = decl;
        edd->expression = expr;
        return ed;
    }

    // export { ... } from "module"; (or without from)
    if (is_punct(&t, "{")) {
        next_tok(p);
        AstNode *ed = ast_export_named_declaration(NULL, s, s);
        ExportNamedDeclaration *end = (ExportNamedDeclaration *)ed->data;
        Token nt = peek_tok(p);
        if (!is_punct(&nt, "}")) {
            for (;;) {
                Token ntok = peek_tok(p);
                if (ntok.type != TOKEN_IDENTIFIER) return ast_error("ExpectedExportSpecifier", pos_start(&ntok), pos_end(&ntok));
                next_tok(p);
                AstNode *idn = ast_identifier(ntok.lexeme, pos_start(&ntok), pos_end(&ntok));
                astvec_push(&end->specifiers, idn);
                Token comma = peek_tok(p);
                if (!is_punct(&comma, ",")) break;
                next_tok(p);
            }
        }
        if (!expect_punct(p, "}", NULL)) return ast_error("ExpectedCloseBrace", pos_start(&t), pos_end(&t));
        Token fromt = peek_tok(p);
        if (is_keyword(&fromt, "from")) {
            next_tok(p);
            Token src = peek_tok(p);
            if (src.type != TOKEN_STRING) return ast_error("ExpectedModuleString", pos_start(&src), pos_end(&src));
            next_tok(p);
            free(end->source);
            end->source = dup_unquoted_string(&src);
            token_free(&src);
        }
        Token semi = peek_tok(p);
        if (is_punct(&semi, ";")) next_tok(p);
        return ed;
    }

    // export function declaration
    if (is_keyword(&t, "function")) {
        AstNode *decl = parse_function(p, 1);
        AstNode *ed = ast_export_named_declaration(NULL, s, decl ? decl->end : s);
        ExportNamedDeclaration *end = (ExportNamedDeclaration *)ed->data;
        end->declaration = decl;
        return ed;
    }

    return ast_error("UnsupportedExport", s, s);
}

static AstNode *parse_for(Parser *p) {
    Token ft = next_tok(p);
    Position s = pos_start(&ft);
    if (!expect_punct(p, "(", NULL)) return ast_error("ExpectedOpenParen", s, s);

    // init/left side
    AstNode *left = NULL;
    Token t = peek_tok(p);
    if (is_keyword(&t, "var")) { 
        next_tok(p); 
        left = parse_variable_declaration(p, VD_Var); 
    }
    else if (is_keyword(&t, "let")) { 
        next_tok(p); 
        left = parse_variable_declaration(p, VD_Let); 
    }
    else if (is_keyword(&t, "const")) { 
        next_tok(p); 
        left = parse_variable_declaration(p, VD_Const); 
    }
    else if (!is_punct(&t, ";")) {
        // Try to parse left side - could be identifier for for-in/for-of
        left = parse_expression(p);
    }

    // Check for 'of' keyword
    Token look = peek_tok(p);
    if (is_keyword(&look, "of")) {
        next_tok(p); // consume 'of'
        AstNode *right = parse_expression(p);
        Token rparen;
        expect_punct(p, ")", &rparen);
        AstNode *body = parse_statement(p);
        Position e = body ? body->end : pos_end(&rparen);
        return ast_for_of_statement(left, right, body, s, e);
    }
    
    // Check for 'in' keyword
    if (is_keyword(&look, "in")) {
        next_tok(p); // consume 'in'
        AstNode *right = parse_expression(p);
        Token rparen;
        expect_punct(p, ")", &rparen);
        AstNode *body = parse_statement(p);
        Position e = body ? body->end : pos_end(&rparen);
        return ast_for_in_statement(left, right, body, s, e);
    }

    // Regular for loop
    AstNode *test = NULL;
    t = peek_tok(p);
    if (!is_punct(&t, ";")) { test = parse_expression(p); }
    Token semi2;
    expect_punct(p, ";", &semi2);

    // update
    AstNode *update = NULL;
    t = peek_tok(p);
    if (!is_punct(&t, ")")) { update = parse_expression(p); }
    Token rparen;
    expect_punct(p, ")", &rparen);

    AstNode *body = parse_statement(p);
    Position e = body ? body->end : pos_end(&rparen);
    return ast_for_statement(left, test, update, body, s, e);
}

static AstNode *parse_return(Parser *p) {
    Token rt = next_tok(p);
    Position s = pos_start(&rt);
    Token t = peek_tok(p);
    AstNode *arg = NULL;
    if (!is_punct(&t, ";") && t.type != TOKEN_EOF && !is_punct(&t, "}")) {
        arg = parse_expression(p);
    }
    Token semi = peek_tok(p);
    if (is_punct(&semi, ";")) { next_tok(p); }
    Position e = arg ? arg->end : pos_end(&rt);
    return ast_return_statement(arg, s, e);
}

static AstNode *parse_break(Parser *p) {
    Token bt = next_tok(p);
    Position s = pos_start(&bt);
    Token semi = peek_tok(p);
    if (is_punct(&semi, ";")) next_tok(p);
    return ast_break_statement(s, pos_end(&bt));
}

static AstNode *parse_continue(Parser *p) {
    Token ct = next_tok(p);
    Position s = pos_start(&ct);
    Token semi = peek_tok(p);
    if (is_punct(&semi, ";")) next_tok(p);
    return ast_continue_statement(s, pos_end(&ct));
}

// literal keywords: null/true/false/undefined
static AstNode *parse_literal_keyword(Token t) {
    Position s = pos_start(&t);
    Position e = pos_end(&t);
    AstNode *lit = NULL;
    
    if (is_keyword(&t, "true")) {
        lit = ast_literal(LIT_Boolean, "true", s, e);
    } else if (is_keyword(&t, "false")) {
        lit = ast_literal(LIT_Boolean, "false", s, e);
    } else if (is_keyword(&t, "null")) {
        lit = ast_literal(LIT_Null, "null", s, e);
    } else if (is_keyword(&t, "undefined")) {
        lit = ast_literal(LIT_Undefined, "undefined", s, e);
    } else {
        // Fallback for other keywords as string literals
        lit = ast_literal(LIT_String, t.lexeme, s, e);
    }
    
    token_free(&t);
    return lit;
}

// object literal
static AstNode *parse_object_literal(Parser *p) {
    Token lbrace = next_tok(p);
    Position s = pos_start(&lbrace);
    AstNode *obj = ast_object_expression(s, s);
    ObjectExpression *oe = (ObjectExpression *)obj->data;

    Token look = peek_tok(p);
    if (!is_punct(&look, "}")) {
        for (;;) {
            Token key_tok = next_tok(p);
            AstNode *key = NULL;
            if (key_tok.type == TOKEN_IDENTIFIER) {
                key = ast_identifier(key_tok.lexeme, pos_start(&key_tok), pos_end(&key_tok));
            } else if (key_tok.type == TOKEN_STRING) {
                key = ast_literal(LIT_String, key_tok.lexeme, pos_start(&key_tok), pos_end(&key_tok));
            } else {
                AstNode *err = ast_error("ExpectedPropertyKey", pos_start(&key_tok), pos_end(&key_tok));
                astvec_push(&oe->properties, err);
                token_free(&key_tok);
                break;
            }
            token_free(&key_tok);

            Token colon = peek_tok(p);
            if (!is_punct(&colon, ":")) {
                AstNode *err = ast_error("ExpectedColon", pos_start(&colon), pos_end(&colon));
                astvec_push(&oe->properties, err);
                break;
            }
            next_tok(p); // consume ':'

            AstNode *value = parse_expression(p);
            AstNode *prop = ast_property(key, value, 0);
            astvec_push(&oe->properties, prop);

            Token sep = peek_tok(p);
            if (is_punct(&sep, "}")) break;
            if (!is_punct(&sep, ",")) {
                AstNode *err = ast_error("ExpectedCommaOrCloseBrace", pos_start(&sep), pos_end(&sep));
                astvec_push(&oe->properties, err);
                break;
            }
            next_tok(p); // consume ','
        }
    }

    Token rbrace = peek_tok(p);
    if (!is_punct(&rbrace, "}")) {
        AstNode *err = ast_error("ExpectedCloseBrace", pos_start(&rbrace), pos_end(&rbrace));
        astvec_push(&oe->properties, err);
    } else {
        next_tok(p);
        obj->end = pos_end(&rbrace);
    }
    token_free(&lbrace);
    return obj;
}

// array literal
static AstNode *parse_array_literal(Parser *p) {
    Token lbracket = next_tok(p);
    Position s = pos_start(&lbracket);
    AstNode *arr = ast_array_expression(s, s);
    ArrayExpression *ae = (ArrayExpression *)arr->data;

    Token look = peek_tok(p);
    if (!is_punct(&look, "]")) {
        for (;;) {
            Token t = peek_tok(p);
            if (is_punct(&t, ",")) {
                astvec_push(&ae->elements, NULL); // hole
                next_tok(p);
                continue;
            }
            if (is_punct(&t, "]")) break;

            AstNode *elem = parse_expression(p);
            astvec_push(&ae->elements, elem);

            Token sep = peek_tok(p);
            if (is_punct(&sep, "]")) break;
            if (!is_punct(&sep, ",")) {
                AstNode *err = ast_error("ExpectedCommaOrCloseBracket", pos_start(&sep), pos_end(&sep));
                astvec_push(&ae->elements, err);
                break;
            }
            next_tok(p);
        }
    }

    Token rbracket = peek_tok(p);
    if (!is_punct(&rbracket, "]")) {
        AstNode *err = ast_error("ExpectedCloseBracket", pos_start(&rbracket), pos_end(&rbracket));
        astvec_push(&ae->elements, err);
    } else {
        next_tok(p);
        arr->end = pos_end(&rbracket);
    }
    token_free(&lbracket);
    return arr;
}

// primary expressions
static AstNode *parse_primary(Parser *p) {
    Token t = peek_tok(p);

    if (is_keyword(&t, "function")) return parse_function(p, 0);
    if (is_keyword(&t, "this")) {
        Token this_tok = next_tok(p);
        AstNode *node = ast_this_expression(pos_start(&this_tok), pos_end(&this_tok));
        token_free(&this_tok);
        return node;
    }
    if (is_keyword(&t, "super")) {
        Token super_tok = next_tok(p);
        AstNode *node = ast_super(pos_start(&super_tok), pos_end(&super_tok));
        token_free(&super_tok);
        return node;
    }
    if (is_punct(&t, "{")) return parse_object_literal(p);
    if (is_punct(&t, "[")) return parse_array_literal(p);
    
    // Check for template literal
    Token peek = peek_tok(p);
    if (peek.type == TOKEN_TEMPLATE) {
        return parse_template_literal(p);
    }

    if (is_punct(&t, "(")) {
        next_tok(p); // consume '('
        Position s = pos_start(&t);
        AstNode *expr = parse_expression(p);
        Token rparen = peek_tok(p);
        if (!is_punct(&rparen, ")")) {
            AstNode *err = ast_error("ExpectedCloseParen", pos_start(&rparen), pos_end(&rparen));
            return err;
        }
        next_tok(p);
        expr->start = s;
        expr->end = pos_end(&rparen);
        token_free(&t);
        return expr;
    }

    t = next_tok(p);

    if (is_keyword(&t, "null") || is_keyword(&t, "true") || is_keyword(&t, "false")) {
        return parse_literal_keyword(t);
    }

    if (t.type == TOKEN_IDENTIFIER) {
        AstNode *id = ast_identifier(t.lexeme, pos_start(&t), pos_end(&t));
        token_free(&t);
        return id;
    }
    if (t.type == TOKEN_NUMBER) {
        AstNode *lit = ast_literal(LIT_Number, t.lexeme, pos_start(&t), pos_end(&t));
        token_free(&t);
        return lit;
    }
    if (t.type == TOKEN_STRING) {
        AstNode *lit = ast_literal(LIT_String, t.lexeme, pos_start(&t), pos_end(&t));
        token_free(&t);
        return lit;
    }
    if (t.type == TOKEN_ERROR) {
        AstNode *err = ast_error(t.error_kind ? t.error_kind : "LexerError", pos_start(&t), pos_end(&t));
        token_free(&t);
        return err;
    }
    AstNode *err = ast_error("UnexpectedToken", pos_start(&t), pos_end(&t));
    token_free(&t);
    return err;
}

// postfix: member access, call, postfix ++/--
static AstNode *parse_postfix(Parser *p) {
    AstNode *expr = parse_primary(p);

    for (;;) {
        Token t = peek_tok(p);

        // member access: obj.prop
        if (is_punct(&t, ".")) {
            next_tok(p);
            Token prop = next_tok(p);
            if (prop.type != TOKEN_IDENTIFIER) {
                AstNode *err = ast_error("ExpectedIdentifier", pos_start(&prop), pos_end(&prop));
                token_free(&prop);
                return err;
            }
            AstNode *prop_node = ast_identifier(prop.lexeme, pos_start(&prop), pos_end(&prop));
            Position s = expr->start;
            Position e = prop_node->end;
            expr = ast_member_expression(expr, prop_node, 0, s, e);
            token_free(&prop);
            continue;
        }

        // computed member: obj[expr]
        if (is_punct(&t, "[")) {
            next_tok(p);
            AstNode *index = parse_expression(p);
            Token close = peek_tok(p);
            if (!is_punct(&close, "]")) {
                AstNode *err = ast_error("ExpectedCloseBracket", pos_start(&close), pos_end(&close));
                return err;
            }
            next_tok(p);
            Position s = expr->start;
            Position e = pos_end(&close);
            expr = ast_member_expression(expr, index, 1, s, e);
            continue;
        }

        // call expression
        if (is_punct(&t, "(")) {
            next_tok(p);
            Position s = expr->start;
            AstNode *call = ast_call_expression(expr, s, s);
            CallExpression *ce = (CallExpression *)call->data;

            Token arg_first = peek_tok(p);
            if (!is_punct(&arg_first, ")")) {
                for (;;) {
                    AstNode *arg = parse_expression(p);
                    astvec_push(&ce->arguments, arg);
                    Token comma = peek_tok(p);
                    if (!is_punct(&comma, ",")) break;
                    next_tok(p);
                }
            }

            Token rparen = peek_tok(p);
            if (!is_punct(&rparen, ")")) {
                AstNode *err = ast_error("ExpectedCloseParen", pos_start(&rparen), pos_end(&rparen));
                return err;
            }
            next_tok(p);
            call->end = pos_end(&rparen);
            expr = call;
            continue;
        }

        // postfix ++/--
        if ((is_punct(&t, "++") || is_punct(&t, "--")) && expr && expr->type == AST_Identifier) {
            next_tok(p);
            Position s = expr->start;
            Position e = pos_end(&t);
            expr = ast_update_expression(t.lexeme, 0, expr, s, e);
            continue;
        }

        break;
    }

    return expr;
}

// unary (prefix) including ++/--
static AstNode *parse_unary(Parser *p) {
    Token t = peek_tok(p);
    if (is_punct(&t, "++") || is_punct(&t, "--") || is_punct(&t, "-") || is_punct(&t, "+") || is_punct(&t, "!") || is_keyword(&t, "typeof") || is_keyword(&t, "void") || is_keyword(&t, "delete")) {
        next_tok(p);
        Position s = pos_start(&t);
        AstNode *arg = parse_unary(p);
        Position e = arg->end;
        AstNode *un = NULL;
        if (is_punct(&t, "++") || is_punct(&t, "--")) {
            un = ast_update_expression(t.lexeme, 1, arg, s, e);
        } else {
            un = ast_unary_expression(t.lexeme, 1, arg, s, e);
        }
        token_free(&t);
        return un;
    }
    return parse_postfix(p);
}

// binary precedence
static int get_binary_precedence(const char *op) {
    if (!op) return 0;
    if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0 || strcmp(op, "%") == 0) return 5;
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) return 4;
    if (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0) return 3;
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 || strcmp(op, "===") == 0 || strcmp(op, "!==") == 0) return 3;
    if (strcmp(op, "&&") == 0) return 2;
    if (strcmp(op, "||") == 0) return 1;
    return 0;
}

static AstNode *parse_binary_expr(Parser *p, int min_prec) {
    AstNode *left = parse_unary(p);

    for (;;) {
        Token t = peek_tok(p);
        if (t.type != TOKEN_PUNCTUATOR && !is_keyword(&t, "in") && !is_keyword(&t, "instanceof")) break;

        int prec = get_binary_precedence(t.lexeme);
        if (prec == 0 || prec < min_prec) break; // not a binary operator

        next_tok(p);
        AstNode *right = parse_binary_expr(p, prec + 1);
        Position s = left->start;
        Position e = right->end;
        left = ast_binary_expression(t.lexeme, left, right, s, e);
        token_free(&t);
    }

    return left;
}

// assignment expression
static int is_assign_op(Token *t) {
    return is_punct(t, "=") || is_punct(t, "+=") || is_punct(t, "-=") ||
           is_punct(t, "*=") || is_punct(t, "/=") || is_punct(t, "%=");
}

static AstNode *parse_assignment(Parser *p) {
    AstNode *left = parse_binary_expr(p, 0);

    Token t = peek_tok(p);
    
    // Check for arrow function: identifier => or (params) =>
    if (is_punct(&t, "=>")) {
        next_tok(p); // consume '=>'
        Position s = left->start;
        
        // Parse arrow function body
        Token body_peek = peek_tok(p);
        AstNode *body = NULL;
        if (is_punct(&body_peek, "{")) {
            // Block body
            body = parse_block(p);
        } else {
            // Expression body
            body = parse_assignment(p);
        }
        
        Position e = body ? body->end : pos_end(&t);
        AstNode *arrow = ast_arrow_function_expression(0, s, e);
        ArrowFunctionExpression *afe = (ArrowFunctionExpression *)arrow->data;
        
        // Handle parameters
        // If left is an identifier, it's a single param
        if (left && left->type == AST_Identifier) {
            astvec_push(&afe->params, left);
        } else {
            // TODO: Handle multiple params from parenthesized list
            // For now, just add the expression as-is
            if (left) astvec_push(&afe->params, left);
        }
        
        afe->body = body;
        token_free(&t);
        return arrow;
    }
    
    if (is_assign_op(&t)) {
        next_tok(p);
        AstNode *right = parse_assignment(p);
        Position s = left->start;
        Position e = right->end;
        AstNode *assign = ast_assignment_expression(t.lexeme, left, right, s, e);
        token_free(&t);
        return assign;
    }

    return left;
}

static AstNode *parse_expression(Parser *p) {
    return parse_assignment(p);
}

static AstNode *parse_variable_declaration(Parser *p, VarKind kind) {
    AstNode *decl = ast_variable_declaration(kind);
    VariableDeclaration *vd = (VariableDeclaration *)decl->data;

    Token t = peek_tok(p);
    if (t.type != TOKEN_IDENTIFIER) {
        AstNode *err = ast_error("ExpectedIdentifier", pos_start(&t), pos_end(&t));
        astvec_push(&vd->declarations, err);
        return decl;
    }
    Token idt = next_tok(p);
    AstNode *id = ast_identifier(idt.lexeme, pos_start(&idt), pos_end(&idt));
    token_free(&idt);

    AstNode *init = NULL;
    Token pt = peek_tok(p);
    if (is_punct(&pt, "=")) {
        next_tok(p);
        init = parse_expression(p);
    }

    AstNode *vdtr = ast_variable_declarator(id, init);
    astvec_push(&vd->declarations, vdtr);

    Token semi = peek_tok(p);
    if (is_punct(&semi, ";")) { next_tok(p); }
    return decl;
}

// Phase 2: Modern Features

static AstNode *parse_template_literal(Parser *p) {
    Token backtick = next_tok(p); // consume template token
    Position s = pos_start(&backtick);
    AstNode *tl_node = ast_template_literal(s, pos_end(&backtick));
    TemplateLiteral *tl = (TemplateLiteral *)tl_node->data;

    // Extract template string content (removing backticks)
    if (backtick.lexeme) {
        const char *lex = backtick.lexeme;
        size_t len = strlen(lex);
        if (len >= 2 && lex[0] == '`' && lex[len-1] == '`') {
            // Extract content between backticks
            size_t content_len = len - 2;
            char *content = (char *)malloc(content_len + 1);
            if (content) {
                memcpy(content, lex + 1, content_len);
                content[content_len] = '\0';
                
                // For now, create a single template element with the whole content
                // TODO: Parse ${expression} interpolations
                AstNode *elem = ast_template_element(content, 1, s, pos_end(&backtick));
                astvec_push(&tl->quasis, elem);
                free(content);
            }
        }
    }
    
    token_free(&backtick);
    return tl_node;
// Arrow function parser (currently not used, reserved for future implementation)
#ifdef ENABLE_ARROW_FUNCTION_PARSING
}

static AstNode *parse_arrow_function(Parser *p, AstNode *param_or_params) {
    // param_or_params is the parsed left side (single identifier or paren-enclosed list)
    // Now we expect '=>' and then the body
    Position s = param_or_params ? param_or_params->start : p->lx.pos > 0 ? ((Position){1, 1}) : ((Position){0, 0});
    
    // Consume '=>'
    Token arrow = peek_tok(p);
    if (!is_punct(&arrow, "=>")) {
        return param_or_params; // Not an arrow function, return the expr as-is
    }
    next_tok(p);

    // Parse body
    Token next = peek_tok(p);
    AstNode *body = NULL;
    if (is_punct(&next, "{")) {
        body = parse_block(p);
    } else {
        // Expression body
        body = parse_assignment(p);
    }

    AstNode *arrow_fn = ast_arrow_function_expression(0, s, body ? body->end : pos_end(&arrow));
    ArrowFunctionExpression *afe = (ArrowFunctionExpression *)arrow_fn->data;
    
    // Extract params from param_or_params
    if (param_or_params && param_or_params->type == AST_Identifier) {
        astvec_push(&afe->params, param_or_params);
    } else if (param_or_params) {
        // Could be from array pattern or other pattern
        astvec_push(&afe->params, param_or_params);
    }
    
    afe->body = body;
    return arrow_fn;
#endif
}

static AstNode *parse_class(Parser *p, int is_decl) {
    Token class_tok = next_tok(p); // consume 'class'
    Position s = pos_start(&class_tok);

    Token name_tok = peek_tok(p);
    AstNode *class_id = NULL;
    if (name_tok.type == TOKEN_IDENTIFIER) {
        class_id = ast_identifier(name_tok.lexeme, pos_start(&name_tok), pos_end(&name_tok));
        next_tok(p);
    } else if (is_decl) {
        return ast_error("ExpectedClassName", s, s);
    }

    AstNode *super_class = NULL;
    Token look = peek_tok(p);
    if (is_keyword(&look, "extends")) {
        next_tok(p); // consume 'extends'
        super_class = parse_primary(p);
    }

    if (!expect_punct(p, "{", NULL)) {
        return ast_error("ExpectedClassBody", s, s);
    }

    AstNode *class_node = is_decl ? ast_class_declaration(class_id, super_class, s, s)
                                   : ast_class_expression(class_id, super_class, s, s);
    
    // Parse class body methods
    for (;;) {
        Token t = peek_tok(p);
        if (is_punct(&t, "}")) {
            next_tok(p);
            class_node->end = pos_end(&t);
            break;
        }
        // For now, skip method parsing - simplified
        next_tok(p);
    }

    token_free(&class_tok);
    if (name_tok.type == TOKEN_IDENTIFIER) token_free(&name_tok);
    return class_node;
}

static AstNode *parse_statement(Parser *p) {
    Token t = peek_tok(p);
    // skip comments
    if (t.type == TOKEN_COMMENT_LINE || t.type == TOKEN_COMMENT_BLOCK) {
        Token ct = next_tok(p);
        record_comment(p, &ct);
        token_free(&ct);
        return parse_statement(p);
    }

    if (is_punct(&t, "{")) return parse_block(p);
    if (is_keyword(&t, "if")) return parse_if(p);
    if (is_keyword(&t, "while")) return parse_while(p);
    if (is_keyword(&t, "do")) return parse_do_while(p);
    if (is_keyword(&t, "for")) return parse_for(p);
    if (is_keyword(&t, "switch")) return parse_switch(p);
    if (is_keyword(&t, "try")) return parse_try(p);
    if (is_keyword(&t, "throw")) return parse_throw(p);
    if (is_keyword(&t, "function")) return parse_function(p, 1);
    if (is_keyword(&t, "class")) return parse_class(p, 1);
    if (is_keyword(&t, "import")) return parse_import(p);
    if (is_keyword(&t, "export")) return parse_export(p);
    if (is_keyword(&t, "return")) return parse_return(p);
    if (is_keyword(&t, "break")) return parse_break(p);
    if (is_keyword(&t, "continue")) return parse_continue(p);
    if (is_keyword(&t, "var")) { next_tok(p); return parse_variable_declaration(p, VD_Var); }
    if (is_keyword(&t, "let")) { next_tok(p); return parse_variable_declaration(p, VD_Let); }
    if (is_keyword(&t, "const")) { next_tok(p); return parse_variable_declaration(p, VD_Const); }
    if (t.type == TOKEN_EOF) { return NULL; }

    Position s = pos_start(&t);
    AstNode *expr = parse_expression(p);
    Token endt = peek_tok(p);
    Position e = pos_start(&endt);
    if (is_punct(&endt, ";")) { next_tok(p); endt = peek_tok(p); }
    AstNode *stmt = ast_expression_statement(expr, s, e);
    return stmt;
}

AstNode *parse_program(Parser *p) {
    AstNode *prog = ast_program();
    Program *pr = (Program *)prog->data;
    p->comment_sink = pr;
    for (;;) {
        Token t = peek_tok(p);
        if (t.type == TOKEN_COMMENT_LINE || t.type == TOKEN_COMMENT_BLOCK) {
            Token ct = next_tok(p);
            record_comment(p, &ct);
            token_free(&ct);
            continue;
        }
        if (t.type == TOKEN_EOF) { break; }
        AstNode *stmt = parse_statement(p);
        if (!stmt) break;
        astvec_push(&pr->body, stmt);
    }
    return prog;
}
