#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "quickjsflow/scope.h"

struct ScopeMapEntry {
    const AstNode *node;
    Scope *scope;
};

static int pos_cmp(Position a, Position b) {
    if (a.line < b.line) return -1;
    if (a.line > b.line) return 1;
    if (a.column < b.column) return -1;
    if (a.column > b.column) return 1;
    return 0;
}

static void bindingvec_push(BindingVec *v, Binding *b) {
    if (v->count + 1 > v->capacity) {
        size_t cap = v->capacity ? v->capacity * 2 : 4;
        Binding **items = (Binding **)realloc(v->items, cap * sizeof(Binding *));
        if (!items) return;
        v->items = items;
        v->capacity = cap;
    }
    v->items[v->count++] = b;
}

static void referencevec_push(ReferenceVec *v, Reference *r) {
    if (v->count + 1 > v->capacity) {
        size_t cap = v->capacity ? v->capacity * 2 : 4;
        Reference **items = (Reference **)realloc(v->items, cap * sizeof(Reference *));
        if (!items) return;
        v->items = items;
        v->capacity = cap;
    }
    v->items[v->count++] = r;
}

static void scopevec_push(ScopeVec *v, Scope *s) {
    if (v->count + 1 > v->capacity) {
        size_t cap = v->capacity ? v->capacity * 2 : 4;
        Scope **items = (Scope **)realloc(v->items, cap * sizeof(Scope *));
        if (!items) return;
        v->items = items;
        v->capacity = cap;
    }
    v->items[v->count++] = s;
}

static void map_add(ScopeManager *sm, const AstNode *node, Scope *scope) {
    if (!node || !scope) return;
    if (sm->map_count + 1 > sm->map_capacity) {
        size_t cap = sm->map_capacity ? sm->map_capacity * 2 : 8;
        ScopeMapEntry *items = (ScopeMapEntry *)realloc(sm->map, cap * sizeof(ScopeMapEntry));
        if (!items) return;
        sm->map = items;
        sm->map_capacity = cap;
    }
    sm->map[sm->map_count].node = node;
    sm->map[sm->map_count].scope = scope;
    sm->map_count++;
}

static Scope *map_lookup(const ScopeManager *sm, const AstNode *node) {
    if (!sm || !node) return NULL;
    for (size_t i = 0; i < sm->map_count; ++i) {
        if (sm->map[i].node == node) return sm->map[i].scope;
    }
    return NULL;
}

void scope_manager_init(ScopeManager *sm) {
    if (!sm) return;
    sm->root = NULL;
    sm->map = NULL;
    sm->map_count = 0;
    sm->map_capacity = 0;
}

static void free_binding(Binding *b) {
    if (!b) return;
    free(b->name);
    free(b);
}

static void free_reference(Reference *r) {
    if (!r) return;
    free(r->name);
    free(r);
}

static void free_scope(Scope *s) {
    if (!s) return;
    for (size_t i = 0; i < s->children.count; ++i) {
        free_scope(s->children.items[i]);
    }
    for (size_t i = 0; i < s->bindings.count; ++i) free_binding(s->bindings.items[i]);
    for (size_t i = 0; i < s->references.count; ++i) free_reference(s->references.items[i]);
    free(s->children.items);
    free(s->bindings.items);
    free(s->references.items);
    free(s);
}

void scope_manager_free(ScopeManager *sm) {
    if (!sm) return;
    free_scope(sm->root);
    sm->root = NULL;
    free(sm->map);
    sm->map = NULL;
    sm->map_count = 0;
    sm->map_capacity = 0;
}

static char *dup_name(const char *name) {
    if (!name) return NULL;
    size_t len = strlen(name);
    char *n = (char *)malloc(len + 1);
    if (!n) return NULL;
    memcpy(n, name, len + 1);
    return n;
}

static Scope *new_scope(ScopeManager *sm, ScopeType type, Scope *parent, const AstNode *node) {
    Scope *s = (Scope *)calloc(1, sizeof(Scope));
    if (!s) return NULL;
    s->type = type;
    s->parent = parent;
    s->node = node;
    if (parent) scopevec_push(&parent->children, s);
    map_add(sm, node, s);
    return s;
}

Binding *scope_lookup_local(Scope *scope, const char *name) {
    if (!scope || !name) return NULL;
    for (size_t i = 0; i < scope->bindings.count; ++i) {
        Binding *b = scope->bindings.items[i];
        if (b && b->name && strcmp(b->name, name) == 0) return b;
    }
    return NULL;
}

Binding *scope_resolve(Scope *scope, const char *name) {
    Scope *s = scope;
    while (s) {
        Binding *b = scope_lookup_local(s, name);
        if (b) return b;
        s = s->parent;
    }
    return NULL;
}

Scope *scope_of_node(const ScopeManager *sm, const AstNode *node) {
    return map_lookup(sm, node);
}

static BindingKind var_kind_to_binding(VarKind k) {
    switch (k) {
        case VD_Var: return BIND_VAR;
        case VD_Let: return BIND_LET;
        case VD_Const: return BIND_CONST;
        default: return BIND_VAR;
    }
}

static Binding *add_binding(Scope *scope, BindingKind kind, const char *name, const AstNode *node, Position loc) {
    if (!scope || !name) return NULL;
    Binding *outer = scope ? scope_resolve(scope->parent, name) : NULL;
    Binding *b = (Binding *)calloc(1, sizeof(Binding));
    if (!b) return NULL;
    b->name = dup_name(name);
    b->kind = kind;
    b->loc = loc;
    b->node = node;
    b->scope = scope;
    b->shadowed = outer;
    bindingvec_push(&scope->bindings, b);
    return b;
}

static Reference *add_reference(Scope *scope, const char *name, int is_write, const AstNode *node) {
    if (!scope || !name) return NULL;
    Reference *r = (Reference *)calloc(1, sizeof(Reference));
    if (!r) return NULL;
    r->name = dup_name(name);
    r->is_write = is_write;
    r->node = node;
    r->loc = node ? node->start : (Position){0, 0};
    r->scope = scope;
    referencevec_push(&scope->references, r);
    return r;
}

static Scope *find_var_scope(Scope *scope) {
    Scope *s = scope;
    while (s) {
        if (s->type == SCOPE_FUNCTION || s->type == SCOPE_GLOBAL || s->type == SCOPE_MODULE) {
            return s;
        }
        s = s->parent;
    }
    return scope;
}

static int is_identifier(const AstNode *n) {
    return n && n->type == AST_Identifier;
}

static const char *identifier_name(const AstNode *n) {
    if (!n || n->type != AST_Identifier) return NULL;
    Identifier *id = (Identifier *)n->data;
    return id ? id->name : NULL;
}

// Forward declarations for the two-pass walk
static void collect_decls(ScopeManager *sm, Scope *scope, AstNode *node, int allow_block_scope);
static void collect_refs(ScopeManager *sm, Scope *scope, AstNode *node, int allow_block_scope);

static void collect_decls_list(ScopeManager *sm, Scope *scope, AstVec *vec) {
    if (!vec) return;
    for (size_t i = 0; i < vec->count; ++i) {
        collect_decls(sm, scope, vec->items[i], 1);
    }
}

static void collect_refs_list(ScopeManager *sm, Scope *scope, AstVec *vec) {
    if (!vec) return;
    for (size_t i = 0; i < vec->count; ++i) {
        collect_refs(sm, scope, vec->items[i], 1);
    }
}

static void collect_decls(ScopeManager *sm, Scope *scope, AstNode *node, int allow_block_scope) {
    if (!node || !scope) return;
    switch (node->type) {
        case AST_Program: {
            Program *pr = (Program *)node->data;
            collect_decls_list(sm, scope, &pr->body);
            break;
        }
        case AST_BlockStatement: {
            Scope *blk_scope = scope;
            if (allow_block_scope) {
                blk_scope = new_scope(sm, SCOPE_BLOCK, scope, node);
            }
            BlockStatement *bs = (BlockStatement *)node->data;
            collect_decls_list(sm, blk_scope, &bs->body);
            break;
        }
        case AST_VariableDeclaration: {
            VariableDeclaration *vd = (VariableDeclaration *)node->data;
            for (size_t i = 0; i < vd->declarations.count; ++i) {
                AstNode *decl = vd->declarations.items[i];
                if (!decl || decl->type != AST_VariableDeclarator) continue;
                VariableDeclarator *vdt = (VariableDeclarator *)decl->data;
                AstNode *id = vdt->id;
                const char *name = identifier_name(id);
                Scope *target = (vd->kind == VD_Var) ? find_var_scope(scope) : scope;
                add_binding(target, var_kind_to_binding(vd->kind), name, id, id ? id->start : (Position){0, 0});
                if (vdt->init) collect_decls(sm, scope, vdt->init, 1);
            }
            break;
        }
        case AST_FunctionDeclaration: {
            FunctionBody *fb = (FunctionBody *)node->data;
            Scope *target = find_var_scope(scope);
            if (fb && fb->name) {
                add_binding(target, BIND_FUNCTION, fb->name, node, node->start);
            }
            Scope *fn_scope = new_scope(sm, SCOPE_FUNCTION, scope, node);
            if (fb) {
                for (size_t i = 0; i < fb->params.count; ++i) {
                    AstNode *p = fb->params.items[i];
                    const char *pname = identifier_name(p);
                    add_binding(fn_scope, BIND_PARAM, pname, p, p ? p->start : (Position){0, 0});
                }
                if (fb->body) collect_decls(sm, fn_scope, fb->body, 0);
            }
            break;
        }
        case AST_FunctionExpression: {
            FunctionBody *fb = (FunctionBody *)node->data;
            Scope *fn_scope = new_scope(sm, SCOPE_FUNCTION, scope, node);
            if (fb && fb->name && fb->name[0]) {
                add_binding(fn_scope, BIND_FUNCTION, fb->name, node, node->start);
            }
            if (fb) {
                for (size_t i = 0; i < fb->params.count; ++i) {
                    AstNode *p = fb->params.items[i];
                    const char *pname = identifier_name(p);
                    add_binding(fn_scope, BIND_PARAM, pname, p, p ? p->start : (Position){0, 0});
                }
                if (fb->body) collect_decls(sm, fn_scope, fb->body, 0);
            }
            break;
        }
        case AST_ForStatement: {
            ForStatement *fs = (ForStatement *)node->data;
            Scope *loop_scope = scope;
            if (fs->init && fs->init->type == AST_VariableDeclaration) {
                VariableDeclaration *vd = (VariableDeclaration *)fs->init->data;
                if (vd->kind == VD_Let || vd->kind == VD_Const) {
                    loop_scope = new_scope(sm, SCOPE_FOR, scope, node);
                }
            }
            if (fs->init) collect_decls(sm, loop_scope, fs->init, 1);
            if (fs->test) collect_decls(sm, loop_scope, fs->test, 1);
            if (fs->update) collect_decls(sm, loop_scope, fs->update, 1);
            if (fs->body) collect_decls(sm, loop_scope, fs->body, 1);
            break;
        }
        case AST_SwitchStatement: {
            SwitchStatement *ss = (SwitchStatement *)node->data;
            Scope *sw_scope = new_scope(sm, SCOPE_BLOCK, scope, node);
            if (ss->discriminant) collect_decls(sm, sw_scope, ss->discriminant, 1);
            for (size_t i = 0; i < ss->cases.count; ++i) {
                collect_decls(sm, sw_scope, ss->cases.items[i], 1);
            }
            break;
        }
        case AST_SwitchCase: {
            SwitchCase *sc = (SwitchCase *)node->data;
            if (sc->test) collect_decls(sm, scope, sc->test, 1);
            collect_decls_list(sm, scope, &sc->consequent);
            break;
        }
        case AST_CatchClause: {
            CatchClause *cc = (CatchClause *)node->data;
            Scope *catch_scope = new_scope(sm, SCOPE_CATCH, scope, node);
            if (cc->param && is_identifier(cc->param)) {
                const char *name = identifier_name(cc->param);
                add_binding(catch_scope, BIND_CATCH, name, cc->param, cc->param->start);
            }
            if (cc->body) collect_decls(sm, catch_scope, cc->body, 0);
            break;
        }
        case AST_ImportDeclaration: {
            ImportDeclaration *id = (ImportDeclaration *)node->data;
            for (size_t i = 0; i < id->specifiers.count; ++i) {
                AstNode *spec = id->specifiers.items[i];
                if (!spec || spec->type != AST_ImportSpecifier) continue;
                ImportSpecifier *is = (ImportSpecifier *)spec->data;
                const char *local = identifier_name(is->local);
                add_binding(scope, BIND_IMPORT, local, is->local, is->local ? is->local->start : (Position){0, 0});
            }
            break;
        }
        case AST_ExportNamedDeclaration: {
            ExportNamedDeclaration *en = (ExportNamedDeclaration *)node->data;
            if (en->declaration) collect_decls(sm, scope, en->declaration, 1);
            break;
        }
        case AST_ExportDefaultDeclaration: {
            ExportDefaultDeclaration *ed = (ExportDefaultDeclaration *)node->data;
            if (ed->declaration) {
                collect_decls(sm, scope, ed->declaration, 1);
            } else if (ed->expression) {
                collect_decls(sm, scope, ed->expression, 1);
            }
            break;
        }
        default: {
            // Recurse into child nodes for declaration-bearing constructs
            switch (node->type) {
                case AST_ExpressionStatement: {
                    ExpressionStatement *es = (ExpressionStatement *)node->data;
                    collect_decls(sm, scope, es->expression, 1);
                    break;
                }
                case AST_UpdateExpression: {
                    UpdateExpression *ue = (UpdateExpression *)node->data;
                    collect_decls(sm, scope, ue->argument, 1);
                    break;
                }
                case AST_BinaryExpression: {
                    BinaryExpression *be = (BinaryExpression *)node->data;
                    collect_decls(sm, scope, be->left, 1);
                    collect_decls(sm, scope, be->right, 1);
                    break;
                }
                case AST_AssignmentExpression: {
                    AssignmentExpression *ae = (AssignmentExpression *)node->data;
                    collect_decls(sm, scope, ae->left, 1);
                    collect_decls(sm, scope, ae->right, 1);
                    break;
                }
                case AST_UnaryExpression: {
                    UnaryExpression *ue = (UnaryExpression *)node->data;
                    collect_decls(sm, scope, ue->argument, 1);
                    break;
                }
                case AST_ObjectExpression: {
                    ObjectExpression *oe = (ObjectExpression *)node->data;
                    for (size_t i = 0; i < oe->properties.count; ++i) {
                        collect_decls(sm, scope, oe->properties.items[i], 1);
                    }
                    break;
                }
                case AST_Property: {
                    Property *prop = (Property *)node->data;
                    if (prop->computed) collect_decls(sm, scope, prop->key, 1);
                    collect_decls(sm, scope, prop->value, 1);
                    break;
                }
                case AST_ArrayExpression: {
                    ArrayExpression *ae = (ArrayExpression *)node->data;
                    for (size_t i = 0; i < ae->elements.count; ++i) {
                        collect_decls(sm, scope, ae->elements.items[i], 1);
                    }
                    break;
                }
                case AST_MemberExpression: {
                    MemberExpression *me = (MemberExpression *)node->data;
                    collect_decls(sm, scope, me->object, 1);
                    if (me->computed) collect_decls(sm, scope, me->property, 1);
                    break;
                }
                case AST_CallExpression: {
                    CallExpression *ce = (CallExpression *)node->data;
                    collect_decls(sm, scope, ce->callee, 1);
                    for (size_t i = 0; i < ce->arguments.count; ++i) collect_decls(sm, scope, ce->arguments.items[i], 1);
                    break;
                }
                case AST_IfStatement: {
                    IfStatement *is = (IfStatement *)node->data;
                    collect_decls(sm, scope, is->test, 1);
                    collect_decls(sm, scope, is->consequent, 1);
                    collect_decls(sm, scope, is->alternate, 1);
                    break;
                }
                case AST_WhileStatement: {
                    WhileStatement *ws = (WhileStatement *)node->data;
                    collect_decls(sm, scope, ws->test, 1);
                    collect_decls(sm, scope, ws->body, 1);
                    break;
                }
                case AST_DoWhileStatement: {
                    DoWhileStatement *dw = (DoWhileStatement *)node->data;
                    collect_decls(sm, scope, dw->body, 1);
                    collect_decls(sm, scope, dw->test, 1);
                    break;
                }
                case AST_TryStatement: {
                    TryStatement *ts = (TryStatement *)node->data;
                    collect_decls(sm, scope, ts->block, 1);
                    for (size_t i = 0; i < ts->handlers.count; ++i) collect_decls(sm, scope, ts->handlers.items[i], 1);
                    collect_decls(sm, scope, ts->finalizer, 1);
                    break;
                }
                case AST_ThrowStatement: {
                    ThrowStatement *ts = (ThrowStatement *)node->data;
                    collect_decls(sm, scope, ts->argument, 1);
                    break;
                }
                case AST_ReturnStatement: {
                    ReturnStatement *rs = (ReturnStatement *)node->data;
                    collect_decls(sm, scope, rs->argument, 1);
                    break;
                }
                case AST_ImportSpecifier:
                case AST_ExportDefaultDeclaration:
                case AST_ExportNamedDeclaration:
                case AST_Identifier:
                case AST_Literal:
                case AST_BreakStatement:
                case AST_ContinueStatement:
                case AST_Error:
                    break;
                default:
                    break;
            }
            break;
        }
    }
}

static void maybe_mark_tdz(Reference *ref, Binding *b) {
    if (!ref || !b) return;
    if (b->kind == BIND_LET || b->kind == BIND_CONST || b->kind == BIND_CATCH || b->kind == BIND_IMPORT) {
        if (b->scope == ref->scope && pos_cmp(ref->loc, b->loc) < 0) {
            ref->in_tdz = 1;
        }
    }
}

static void note_identifier_ref(ScopeManager *sm, Scope *scope, AstNode *id_node, int is_write) {
    if (!id_node || id_node->type != AST_Identifier) return;
    const char *name = identifier_name(id_node);
    Reference *ref = add_reference(scope, name, is_write, id_node);
    if (!ref) return;
    ref->resolved = scope_resolve(scope, name);
    if (!ref->resolved && sm && sm->root && sm->root->type == SCOPE_GLOBAL) {
        Binding *imp = scope_lookup_local(sm->root, name);
        if (!imp) {
            imp = add_binding(sm->root, BIND_IMPLICIT, name, id_node, ref->loc);
        }
        ref->resolved = imp;
    }
    maybe_mark_tdz(ref, ref->resolved);
}

static void collect_refs(ScopeManager *sm, Scope *scope, AstNode *node, int allow_block_scope) {
    if (!node || !scope) return;
    Scope *scoped = map_lookup(sm, node);
    Scope *current = scoped ? scoped : scope;

    switch (node->type) {
        case AST_Program: {
            Program *pr = (Program *)node->data;
            collect_refs_list(sm, current, &pr->body);
            break;
        }
        case AST_BlockStatement: {
            BlockStatement *bs = (BlockStatement *)node->data;
            Scope *blk_scope = current;
            if (!scoped && allow_block_scope) {
                blk_scope = scope;
            }
            collect_refs_list(sm, scoped ? scoped : blk_scope, &bs->body);
            break;
        }
        case AST_VariableDeclaration: {
            VariableDeclaration *vd = (VariableDeclaration *)node->data;
            for (size_t i = 0; i < vd->declarations.count; ++i) {
                AstNode *decl = vd->declarations.items[i];
                if (!decl || decl->type != AST_VariableDeclarator) continue;
                VariableDeclarator *vdt = (VariableDeclarator *)decl->data;
                if (vdt->init) collect_refs(sm, current, vdt->init, 1);
            }
            break;
        }
        case AST_FunctionDeclaration:
        case AST_FunctionExpression: {
            FunctionBody *fb = (FunctionBody *)node->data;
            Scope *fn_scope = scoped ? scoped : current;
            if (fb && fb->body) collect_refs(sm, fn_scope, fb->body, 0);
            break;
        }
        case AST_ForStatement: {
            ForStatement *fs = (ForStatement *)node->data;
            Scope *loop_scope = scoped ? scoped : current;
            if (fs->init) collect_refs(sm, loop_scope, fs->init, 1);
            if (fs->test) collect_refs(sm, loop_scope, fs->test, 1);
            if (fs->update) collect_refs(sm, loop_scope, fs->update, 1);
            if (fs->body) collect_refs(sm, loop_scope, fs->body, 1);
            break;
        }
        case AST_SwitchStatement: {
            SwitchStatement *ss = (SwitchStatement *)node->data;
            Scope *sw_scope = scoped ? scoped : current;
            if (ss->discriminant) collect_refs(sm, sw_scope, ss->discriminant, 1);
            for (size_t i = 0; i < ss->cases.count; ++i) collect_refs(sm, sw_scope, ss->cases.items[i], 1);
            break;
        }
        case AST_SwitchCase: {
            SwitchCase *sc = (SwitchCase *)node->data;
            if (sc->test) collect_refs(sm, current, sc->test, 1);
            collect_refs_list(sm, current, &sc->consequent);
            break;
        }
        case AST_CatchClause: {
            CatchClause *cc = (CatchClause *)node->data;
            Scope *catch_scope = scoped ? scoped : current;
            if (cc->body) collect_refs(sm, catch_scope, cc->body, 0);
            break;
        }
        case AST_ExpressionStatement: {
            ExpressionStatement *es = (ExpressionStatement *)node->data;
            collect_refs(sm, current, es->expression, 1);
            break;
        }
        case AST_UpdateExpression: {
            UpdateExpression *ue = (UpdateExpression *)node->data;
            if (ue->argument && ue->argument->type == AST_Identifier) {
                note_identifier_ref(sm, current, ue->argument, 1);
            } else {
                collect_refs(sm, current, ue->argument, 1);
            }
            break;
        }
        case AST_AssignmentExpression: {
            AssignmentExpression *ae = (AssignmentExpression *)node->data;
            if (ae->left && ae->left->type == AST_Identifier) {
                note_identifier_ref(sm, current, ae->left, 1);
            } else {
                collect_refs(sm, current, ae->left, 1);
            }
            collect_refs(sm, current, ae->right, 1);
            break;
        }
        case AST_BinaryExpression: {
            BinaryExpression *be = (BinaryExpression *)node->data;
            collect_refs(sm, current, be->left, 1);
            collect_refs(sm, current, be->right, 1);
            break;
        }
        case AST_UnaryExpression: {
            UnaryExpression *ue = (UnaryExpression *)node->data;
            collect_refs(sm, current, ue->argument, 1);
            break;
        }
        case AST_ObjectExpression: {
            ObjectExpression *oe = (ObjectExpression *)node->data;
            for (size_t i = 0; i < oe->properties.count; ++i) collect_refs(sm, current, oe->properties.items[i], 1);
            break;
        }
        case AST_Property: {
            Property *prop = (Property *)node->data;
            if (prop->computed) collect_refs(sm, current, prop->key, 1);
            collect_refs(sm, current, prop->value, 1);
            break;
        }
        case AST_ArrayExpression: {
            ArrayExpression *ae = (ArrayExpression *)node->data;
            for (size_t i = 0; i < ae->elements.count; ++i) collect_refs(sm, current, ae->elements.items[i], 1);
            break;
        }
        case AST_MemberExpression: {
            MemberExpression *me = (MemberExpression *)node->data;
            collect_refs(sm, current, me->object, 1);
            if (me->computed) collect_refs(sm, current, me->property, 1);
            break;
        }
        case AST_CallExpression: {
            CallExpression *ce = (CallExpression *)node->data;
            collect_refs(sm, current, ce->callee, 1);
            for (size_t i = 0; i < ce->arguments.count; ++i) collect_refs(sm, current, ce->arguments.items[i], 1);
            break;
        }
        case AST_IfStatement: {
            IfStatement *is = (IfStatement *)node->data;
            collect_refs(sm, current, is->test, 1);
            collect_refs(sm, current, is->consequent, 1);
            collect_refs(sm, current, is->alternate, 1);
            break;
        }
        case AST_WhileStatement: {
            WhileStatement *ws = (WhileStatement *)node->data;
            collect_refs(sm, current, ws->test, 1);
            collect_refs(sm, current, ws->body, 1);
            break;
        }
        case AST_DoWhileStatement: {
            DoWhileStatement *dw = (DoWhileStatement *)node->data;
            collect_refs(sm, current, dw->body, 1);
            collect_refs(sm, current, dw->test, 1);
            break;
        }
        case AST_TryStatement: {
            TryStatement *ts = (TryStatement *)node->data;
            collect_refs(sm, current, ts->block, 1);
            for (size_t i = 0; i < ts->handlers.count; ++i) collect_refs(sm, current, ts->handlers.items[i], 1);
            collect_refs(sm, current, ts->finalizer, 1);
            break;
        }
        case AST_ThrowStatement: {
            ThrowStatement *ts = (ThrowStatement *)node->data;
            collect_refs(sm, current, ts->argument, 1);
            break;
        }
        case AST_ReturnStatement: {
            ReturnStatement *rs = (ReturnStatement *)node->data;
            collect_refs(sm, current, rs->argument, 1);
            break;
        }
        case AST_ImportDeclaration: {
            break;
        }
        case AST_ExportNamedDeclaration: {
            ExportNamedDeclaration *en = (ExportNamedDeclaration *)node->data;
            if (en->declaration) collect_refs(sm, current, en->declaration, 1);
            for (size_t i = 0; i < en->specifiers.count; ++i) {
                AstNode *idn = en->specifiers.items[i];
                if (idn && idn->type == AST_Identifier) {
                    note_identifier_ref(sm, current, idn, 0);
                }
            }
            break;
        }
        case AST_ExportDefaultDeclaration: {
            ExportDefaultDeclaration *ed = (ExportDefaultDeclaration *)node->data;
            if (ed->declaration) collect_refs(sm, current, ed->declaration, 1);
            if (ed->expression) collect_refs(sm, current, ed->expression, 1);
            break;
        }
        case AST_Identifier: {
            note_identifier_ref(sm, current, node, 0);
            break;
        }
        default:
            break;
    }
}

int scope_analyze(ScopeManager *sm, AstNode *root, int is_module) {
    if (!sm || !root) return -1;
    scope_manager_free(sm);
    scope_manager_init(sm);
    sm->root = new_scope(sm, is_module ? SCOPE_MODULE : SCOPE_GLOBAL, NULL, root);
    collect_decls(sm, sm->root, root, 1);
    collect_refs(sm, sm->root, root, 1);
    return 0;
}

static const char *scope_name(ScopeType t) {
    switch (t) {
        case SCOPE_GLOBAL: return "Global";
        case SCOPE_MODULE: return "Module";
        case SCOPE_FUNCTION: return "Function";
        case SCOPE_BLOCK: return "Block";
        case SCOPE_CATCH: return "Catch";
        case SCOPE_FOR: return "For";
        default: return "Unknown";
    }
}

static const char *binding_name(BindingKind k) {
    switch (k) {
        case BIND_VAR: return "var";
        case BIND_LET: return "let";
        case BIND_CONST: return "const";
        case BIND_FUNCTION: return "function";
        case BIND_PARAM: return "param";
        case BIND_CATCH: return "catch";
        case BIND_IMPORT: return "import";
        case BIND_IMPLICIT: return "implicit";
        default: return "binding";
    }
}

static void indent_print(int n) {
    for (int i = 0; i < n; ++i) putchar(' ');
}

static void dump_scope(const Scope *s, int indent) {
    if (!s) return;
    indent_print(indent);
    printf("Scope(%s)\n", scope_name(s->type));

    indent_print(indent + 2);
    printf("Bindings:\n");
    for (size_t i = 0; i < s->bindings.count; ++i) {
        Binding *b = s->bindings.items[i];
        indent_print(indent + 4);
        printf("%s [%s] @%d:%d\n", b->name ? b->name : "<anon>", binding_name(b->kind), b->loc.line, b->loc.column);
    }

    indent_print(indent + 2);
    printf("References:\n");
    for (size_t i = 0; i < s->references.count; ++i) {
        Reference *r = s->references.items[i];
        indent_print(indent + 4);
        printf("%s %s%s -> %s\n",
               r->name ? r->name : "<anon>",
               r->is_write ? "write" : "read",
               r->in_tdz ? " (TDZ)" : "",
               r->resolved && r->resolved->name ? r->resolved->name : "<unresolved>");
    }

    for (size_t i = 0; i < s->children.count; ++i) {
        dump_scope(s->children.items[i], indent + 2);
    }
}

void scope_dump(const Scope *scope, int indent) {
    dump_scope(scope, indent);
}

static void print_json_string(const char *s) {
    if (!s) { printf("null"); return; }
    putchar('"');
    for (const char *p = s; *p; ++p) {
        if (*p == '"' || *p == '\\') putchar('\\');
        putchar(*p);
    }
    putchar('"');
}

static void print_pos_json(Position p) {
    printf("{\"line\":%d,\"column\":%d}", p.line, p.column);
}

static void dump_scope_json_rec(const Scope *s) {
    if (!s) { printf("null"); return; }
    printf("{\"type\":\"");
    printf("%s\"", scope_name(s->type));
    printf(",\"bindings\":[");
    for (size_t i = 0; i < s->bindings.count; ++i) {
        if (i) printf(",");
        Binding *b = s->bindings.items[i];
        printf("{\"name\":"); print_json_string(b ? b->name : NULL);
        printf(",\"kind\":\""); printf("%s\"", b ? binding_name(b->kind) : "binding");
        printf(",\"loc\":"); print_pos_json(b ? b->loc : (Position){0,0});
        printf(",\"shadowed\":");
        if (b && b->shadowed && b->shadowed->name) print_json_string(b->shadowed->name); else printf("null");
        printf("}");
    }
    printf("],\"references\":[");
    for (size_t i = 0; i < s->references.count; ++i) {
        if (i) printf(",");
        Reference *r = s->references.items[i];
        printf("{\"name\":"); print_json_string(r ? r->name : NULL);
        printf(",\"write\":%s", (r && r->is_write) ? "true" : "false");
        printf(",\"tdz\":%s", (r && r->in_tdz) ? "true" : "false");
        printf(",\"loc\":"); print_pos_json(r ? r->loc : (Position){0,0});
        printf(",\"resolved\":");
        if (r && r->resolved && r->resolved->name) print_json_string(r->resolved->name); else printf("null");
        printf("}");
    }
    printf("],\"children\":[");
    for (size_t i = 0; i < s->children.count; ++i) {
        if (i) printf(",");
        dump_scope_json_rec(s->children.items[i]);
    }
    printf("]}");
}

void scope_dump_json(const Scope *scope) {
    dump_scope_json_rec(scope);
    printf("\n");
}
