#include <stdlib.h>
#include <string.h>
#include "quickjsflow/edit.h"

// --- utilities ------------------------------------------------------------

typedef AstNode *(*PreRewriteFn)(const AstNode *orig, void *ctx, int *handled);
typedef AstNode *(*PostRewriteFn)(AstNode *node, void *ctx);

typedef struct {
    PreRewriteFn pre;
    void *pre_ctx;
    PostRewriteFn post;
    void *post_ctx;
    const AstNode *insert_parent;
    size_t insert_index;
    AstNode *insert_node;
    int *inserted;
} RewriteOptions;

typedef struct {
    const AstNode *target;
    AstNode *replacement;
    int applied;
} ReplaceCtx;

typedef struct {
    const AstNode *target;
    int applied;
} RemoveCtx;

typedef struct {
    const AstNode **nodes;
    size_t count;
    const char *new_name;
} RenameCtx;

typedef struct {
    Reference **items;
    size_t count;
    size_t capacity;
} RefVec;

typedef struct {
    const AstNode **items;
    size_t count;
    size_t capacity;
} NodePtrVec;

typedef struct {
    EditVisitor visitor;
    void *userdata;
} TransformCtx;

static EditStatus status_ok(void) {
    EditStatus st; st.code = 0; st.message[0] = '\0'; return st;
}

static EditStatus status_err(const char *msg) {
    EditStatus st; st.code = -1; st.message[0] = '\0';
    if (msg) {
        strncpy(st.message, msg, sizeof(st.message) - 1);
        st.message[sizeof(st.message) - 1] = '\0';
    }
    return st;
}

static char *dup_cstr(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *d = (char *)malloc(len + 1);
    if (!d) return NULL;
    memcpy(d, s, len + 1);
    return d;
}

static AstNode *copy_base(const AstNode *orig) {
    if (!orig) return NULL;
    AstNode *n = (AstNode *)calloc(1, sizeof(AstNode));
    if (!n) return NULL;
    n->type = orig->type;
    n->start = orig->start;
    n->end = orig->end;
    n->refcount = 1;
    return n;
}

static void refvec_push(RefVec *v, Reference *r) {
    if (!v) return;
    if (v->count + 1 > v->capacity) {
        size_t cap = v->capacity ? v->capacity * 2 : 8;
        Reference **items = (Reference **)realloc(v->items, cap * sizeof(Reference *));
        if (!items) return;
        v->items = items;
        v->capacity = cap;
    }
    v->items[v->count++] = r;
}

static void nodeptrvec_push(NodePtrVec *v, const AstNode *n) {
    if (!v) return;
    if (v->count + 1 > v->capacity) {
        size_t cap = v->capacity ? v->capacity * 2 : 8;
        const AstNode **items = (const AstNode **)realloc(v->items, cap * sizeof(const AstNode *));
        if (!items) return;
        v->items = items;
        v->capacity = cap;
    }
    v->items[v->count++] = n;
}

// forward declarations
static int contains_node(const AstNode *root, const AstNode *needle);
static AstNode *rewrite_tree(const AstNode *orig, const RewriteOptions *opt);

// --- rewriting helpers ----------------------------------------------------

static void maybe_insert(AstVec *dst, const AstNode *owner, size_t current_index, const RewriteOptions *opt) {
    if (!opt || !dst) return;
    if (owner == opt->insert_parent && opt->insert_node && opt->insert_index == current_index) {
        AstNode *clone = ast_clone(opt->insert_node);
        astvec_push(dst, clone);
        if (opt->inserted) *(opt->inserted) = 1;
    }
}

static void maybe_insert_after_all(AstVec *dst, const AstNode *owner, const AstVec *src, const RewriteOptions *opt) {
    if (!opt || !dst) return;
    if (owner == opt->insert_parent && opt->insert_node && opt->insert_index >= (src ? src->count : 0)) {
        AstNode *clone = ast_clone(opt->insert_node);
        astvec_push(dst, clone);
        if (opt->inserted) *(opt->inserted) = 1;
    }
}

static AstNode *rewrite_tree(const AstNode *orig, const RewriteOptions *opt) {
    if (!orig) return NULL;

    int handled = 0;
    AstNode *pre_out = NULL;
    if (opt && opt->pre) {
        pre_out = opt->pre(orig, opt->pre_ctx, &handled);
    }
    if (handled) {
        if (opt && opt->post && pre_out) {
            AstNode *post_out = opt->post(pre_out, opt->post_ctx);
            if (post_out != pre_out) ast_release(pre_out);
            return post_out;
        }
        return pre_out;
    }

    AstNode *n = copy_base(orig);
    if (!n) return NULL;

    switch (orig->type) {
        case AST_Program: {
            Program *op = (Program *)orig->data;
            Program *np = (Program *)calloc(1, sizeof(Program));
            astvec_init(&np->body);
            if (op) {
                for (size_t i = 0; i < op->body.count; ++i) {
                    maybe_insert(&np->body, orig, i, opt);
                    AstNode *child = rewrite_tree(op->body.items[i], opt);
                    if (child) astvec_push(&np->body, child);
                }
                maybe_insert_after_all(&np->body, orig, &op->body, opt);
            }
            n->data = np;
            break;
        }
        case AST_VariableDeclaration: {
            VariableDeclaration *ovd = (VariableDeclaration *)orig->data;
            VariableDeclaration *nvd = (VariableDeclaration *)calloc(1, sizeof(VariableDeclaration));
            if (ovd) {
                nvd->kind = ovd->kind;
                astvec_init(&nvd->declarations);
                for (size_t i = 0; i < ovd->declarations.count; ++i) {
                    maybe_insert(&nvd->declarations, orig, i, opt);
                    AstNode *child = rewrite_tree(ovd->declarations.items[i], opt);
                    if (child) astvec_push(&nvd->declarations, child);
                }
                maybe_insert_after_all(&nvd->declarations, orig, &ovd->declarations, opt);
            }
            n->data = nvd;
            break;
        }
        case AST_VariableDeclarator: {
            VariableDeclarator *ovd = (VariableDeclarator *)orig->data;
            VariableDeclarator *nvd = (VariableDeclarator *)calloc(1, sizeof(VariableDeclarator));
            if (ovd) {
                nvd->id = rewrite_tree(ovd->id, opt);
                nvd->init = rewrite_tree(ovd->init, opt);
            }
            n->data = nvd;
            break;
        }
        case AST_Identifier: {
            Identifier *oid = (Identifier *)orig->data;
            Identifier *nid = (Identifier *)calloc(1, sizeof(Identifier));
            if (oid && oid->name) nid->name = dup_cstr(oid->name);
            n->data = nid;
            break;
        }
        case AST_Literal: {
            Literal *olit = (Literal *)orig->data;
            Literal *nlit = (Literal *)calloc(1, sizeof(Literal));
            if (olit) {
                nlit->kind = olit->kind;
                if (olit->raw) nlit->raw = dup_cstr(olit->raw);
            }
            n->data = nlit;
            break;
        }
        case AST_ExpressionStatement: {
            ExpressionStatement *oes = (ExpressionStatement *)orig->data;
            ExpressionStatement *nes = (ExpressionStatement *)calloc(1, sizeof(ExpressionStatement));
            if (oes) nes->expression = rewrite_tree(oes->expression, opt);
            n->data = nes;
            break;
        }
        case AST_UpdateExpression: {
            UpdateExpression *oue = (UpdateExpression *)orig->data;
            UpdateExpression *nue = (UpdateExpression *)calloc(1, sizeof(UpdateExpression));
            if (oue) {
                nue->prefix = oue->prefix;
                if (oue->operator) nue->operator = dup_cstr(oue->operator);
                nue->argument = rewrite_tree(oue->argument, opt);
            }
            n->data = nue;
            break;
        }
        case AST_BinaryExpression: {
            BinaryExpression *obe = (BinaryExpression *)orig->data;
            BinaryExpression *nbe = (BinaryExpression *)calloc(1, sizeof(BinaryExpression));
            if (obe) {
                if (obe->operator) nbe->operator = dup_cstr(obe->operator);
                nbe->left = rewrite_tree(obe->left, opt);
                nbe->right = rewrite_tree(obe->right, opt);
            }
            n->data = nbe;
            break;
        }
        case AST_AssignmentExpression: {
            AssignmentExpression *oae = (AssignmentExpression *)orig->data;
            AssignmentExpression *nae = (AssignmentExpression *)calloc(1, sizeof(AssignmentExpression));
            if (oae) {
                if (oae->operator) nae->operator = dup_cstr(oae->operator);
                nae->left = rewrite_tree(oae->left, opt);
                nae->right = rewrite_tree(oae->right, opt);
            }
            n->data = nae;
            break;
        }
        case AST_UnaryExpression: {
            UnaryExpression *oue = (UnaryExpression *)orig->data;
            UnaryExpression *nue = (UnaryExpression *)calloc(1, sizeof(UnaryExpression));
            if (oue) {
                if (oue->operator) nue->operator = dup_cstr(oue->operator);
                nue->prefix = oue->prefix;
                nue->argument = rewrite_tree(oue->argument, opt);
            }
            n->data = nue;
            break;
        }
        case AST_ObjectExpression: {
            ObjectExpression *ooe = (ObjectExpression *)orig->data;
            ObjectExpression *noe = (ObjectExpression *)calloc(1, sizeof(ObjectExpression));
            astvec_init(&noe->properties);
            if (ooe) {
                for (size_t i = 0; i < ooe->properties.count; ++i) {
                    maybe_insert(&noe->properties, orig, i, opt);
                    AstNode *child = rewrite_tree(ooe->properties.items[i], opt);
                    if (child) astvec_push(&noe->properties, child);
                }
                maybe_insert_after_all(&noe->properties, orig, &ooe->properties, opt);
            }
            n->data = noe;
            break;
        }
        case AST_Property: {
            Property *op = (Property *)orig->data;
            Property *np = (Property *)calloc(1, sizeof(Property));
            if (op) {
                np->computed = op->computed;
                np->key = rewrite_tree(op->key, opt);
                np->value = rewrite_tree(op->value, opt);
            }
            n->data = np;
            break;
        }
        case AST_ArrayExpression: {
            ArrayExpression *oae = (ArrayExpression *)orig->data;
            ArrayExpression *nae = (ArrayExpression *)calloc(1, sizeof(ArrayExpression));
            astvec_init(&nae->elements);
            if (oae) {
                for (size_t i = 0; i < oae->elements.count; ++i) {
                    maybe_insert(&nae->elements, orig, i, opt);
                    AstNode *child = rewrite_tree(oae->elements.items[i], opt);
                    if (child) astvec_push(&nae->elements, child);
                }
                maybe_insert_after_all(&nae->elements, orig, &oae->elements, opt);
            }
            n->data = nae;
            break;
        }
        case AST_MemberExpression: {
            MemberExpression *ome = (MemberExpression *)orig->data;
            MemberExpression *nme = (MemberExpression *)calloc(1, sizeof(MemberExpression));
            if (ome) {
                nme->computed = ome->computed;
                nme->object = rewrite_tree(ome->object, opt);
                nme->property = rewrite_tree(ome->property, opt);
            }
            n->data = nme;
            break;
        }
        case AST_CallExpression: {
            CallExpression *oce = (CallExpression *)orig->data;
            CallExpression *nce = (CallExpression *)calloc(1, sizeof(CallExpression));
            astvec_init(&nce->arguments);
            if (oce) {
                nce->callee = rewrite_tree(oce->callee, opt);
                for (size_t i = 0; i < oce->arguments.count; ++i) {
                    maybe_insert(&nce->arguments, orig, i, opt);
                    AstNode *child = rewrite_tree(oce->arguments.items[i], opt);
                    if (child) astvec_push(&nce->arguments, child);
                }
                maybe_insert_after_all(&nce->arguments, orig, &oce->arguments, opt);
            }
            n->data = nce;
            break;
        }
        case AST_FunctionDeclaration:
        case AST_FunctionExpression: {
            FunctionBody *ofb = (FunctionBody *)orig->data;
            FunctionBody *nfb = (FunctionBody *)calloc(1, sizeof(FunctionBody));
            if (ofb) {
                if (ofb->name) nfb->name = dup_cstr(ofb->name);
                astvec_init(&nfb->params);
                for (size_t i = 0; i < ofb->params.count; ++i) {
                    maybe_insert(&nfb->params, orig, i, opt);
                    AstNode *child = rewrite_tree(ofb->params.items[i], opt);
                    if (child) astvec_push(&nfb->params, child);
                }
                maybe_insert_after_all(&nfb->params, orig, &ofb->params, opt);
                nfb->body = rewrite_tree(ofb->body, opt);
            }
            n->data = nfb;
            break;
        }
        case AST_BlockStatement: {
            BlockStatement *obs = (BlockStatement *)orig->data;
            BlockStatement *nbs = (BlockStatement *)calloc(1, sizeof(BlockStatement));
            astvec_init(&nbs->body);
            if (obs) {
                for (size_t i = 0; i < obs->body.count; ++i) {
                    maybe_insert(&nbs->body, orig, i, opt);
                    AstNode *child = rewrite_tree(obs->body.items[i], opt);
                    if (child) astvec_push(&nbs->body, child);
                }
                maybe_insert_after_all(&nbs->body, orig, &obs->body, opt);
            }
            n->data = nbs;
            break;
        }
        case AST_IfStatement: {
            IfStatement *ois = (IfStatement *)orig->data;
            IfStatement *nis = (IfStatement *)calloc(1, sizeof(IfStatement));
            if (ois) {
                nis->test = rewrite_tree(ois->test, opt);
                nis->consequent = rewrite_tree(ois->consequent, opt);
                nis->alternate = rewrite_tree(ois->alternate, opt);
            }
            n->data = nis;
            break;
        }
        case AST_WhileStatement: {
            WhileStatement *ows = (WhileStatement *)orig->data;
            WhileStatement *nws = (WhileStatement *)calloc(1, sizeof(WhileStatement));
            if (ows) {
                nws->test = rewrite_tree(ows->test, opt);
                nws->body = rewrite_tree(ows->body, opt);
            }
            n->data = nws;
            break;
        }
        case AST_DoWhileStatement: {
            DoWhileStatement *odw = (DoWhileStatement *)orig->data;
            DoWhileStatement *ndw = (DoWhileStatement *)calloc(1, sizeof(DoWhileStatement));
            if (odw) {
                ndw->body = rewrite_tree(odw->body, opt);
                ndw->test = rewrite_tree(odw->test, opt);
            }
            n->data = ndw;
            break;
        }
        case AST_ForStatement: {
            ForStatement *ofs = (ForStatement *)orig->data;
            ForStatement *nfs = (ForStatement *)calloc(1, sizeof(ForStatement));
            if (ofs) {
                nfs->init = rewrite_tree(ofs->init, opt);
                nfs->test = rewrite_tree(ofs->test, opt);
                nfs->update = rewrite_tree(ofs->update, opt);
                nfs->body = rewrite_tree(ofs->body, opt);
            }
            n->data = nfs;
            break;
        }
        case AST_SwitchStatement: {
            SwitchStatement *oss = (SwitchStatement *)orig->data;
            SwitchStatement *nss = (SwitchStatement *)calloc(1, sizeof(SwitchStatement));
            astvec_init(&nss->cases);
            if (oss) {
                nss->discriminant = rewrite_tree(oss->discriminant, opt);
                for (size_t i = 0; i < oss->cases.count; ++i) {
                    maybe_insert(&nss->cases, orig, i, opt);
                    AstNode *child = rewrite_tree(oss->cases.items[i], opt);
                    if (child) astvec_push(&nss->cases, child);
                }
                maybe_insert_after_all(&nss->cases, orig, &oss->cases, opt);
            }
            n->data = nss;
            break;
        }
        case AST_SwitchCase: {
            SwitchCase *osc = (SwitchCase *)orig->data;
            SwitchCase *nsc = (SwitchCase *)calloc(1, sizeof(SwitchCase));
            astvec_init(&nsc->consequent);
            if (osc) {
                nsc->test = rewrite_tree(osc->test, opt);
                for (size_t i = 0; i < osc->consequent.count; ++i) {
                    maybe_insert(&nsc->consequent, orig, i, opt);
                    AstNode *child = rewrite_tree(osc->consequent.items[i], opt);
                    if (child) astvec_push(&nsc->consequent, child);
                }
                maybe_insert_after_all(&nsc->consequent, orig, &osc->consequent, opt);
            }
            n->data = nsc;
            break;
        }
        case AST_TryStatement: {
            TryStatement *ots = (TryStatement *)orig->data;
            TryStatement *nts = (TryStatement *)calloc(1, sizeof(TryStatement));
            astvec_init(&nts->handlers);
            if (ots) {
                nts->block = rewrite_tree(ots->block, opt);
                for (size_t i = 0; i < ots->handlers.count; ++i) {
                    maybe_insert(&nts->handlers, orig, i, opt);
                    AstNode *child = rewrite_tree(ots->handlers.items[i], opt);
                    if (child) astvec_push(&nts->handlers, child);
                }
                maybe_insert_after_all(&nts->handlers, orig, &ots->handlers, opt);
                nts->finalizer = rewrite_tree(ots->finalizer, opt);
            }
            n->data = nts;
            break;
        }
        case AST_CatchClause: {
            CatchClause *occ = (CatchClause *)orig->data;
            CatchClause *ncc = (CatchClause *)calloc(1, sizeof(CatchClause));
            if (occ) {
                ncc->param = rewrite_tree(occ->param, opt);
                ncc->body = rewrite_tree(occ->body, opt);
            }
            n->data = ncc;
            break;
        }
        case AST_ThrowStatement: {
            ThrowStatement *ots = (ThrowStatement *)orig->data;
            ThrowStatement *nts = (ThrowStatement *)calloc(1, sizeof(ThrowStatement));
            if (ots) nts->argument = rewrite_tree(ots->argument, opt);
            n->data = nts;
            break;
        }
        case AST_ReturnStatement: {
            ReturnStatement *ors = (ReturnStatement *)orig->data;
            ReturnStatement *nrs = (ReturnStatement *)calloc(1, sizeof(ReturnStatement));
            if (ors) nrs->argument = rewrite_tree(ors->argument, opt);
            n->data = nrs;
            break;
        }
        case AST_BreakStatement: {
            BreakStatement *obs = (BreakStatement *)orig->data;
            BreakStatement *nbs = (BreakStatement *)calloc(1, sizeof(BreakStatement));
            if (obs && obs->label) nbs->label = dup_cstr(obs->label);
            n->data = nbs;
            break;
        }
        case AST_ContinueStatement: {
            ContinueStatement *ocs = (ContinueStatement *)orig->data;
            ContinueStatement *ncs = (ContinueStatement *)calloc(1, sizeof(ContinueStatement));
            if (ocs && ocs->label) ncs->label = dup_cstr(ocs->label);
            n->data = ncs;
            break;
        }
        case AST_ImportDeclaration: {
            ImportDeclaration *oid = (ImportDeclaration *)orig->data;
            ImportDeclaration *nid = (ImportDeclaration *)calloc(1, sizeof(ImportDeclaration));
            astvec_init(&nid->specifiers);
            if (oid) {
                if (oid->source) nid->source = dup_cstr(oid->source);
                for (size_t i = 0; i < oid->specifiers.count; ++i) {
                    maybe_insert(&nid->specifiers, orig, i, opt);
                    AstNode *child = rewrite_tree(oid->specifiers.items[i], opt);
                    if (child) astvec_push(&nid->specifiers, child);
                }
                maybe_insert_after_all(&nid->specifiers, orig, &oid->specifiers, opt);
            }
            n->data = nid;
            break;
        }
        case AST_ImportSpecifier: {
            ImportSpecifier *ois = (ImportSpecifier *)orig->data;
            ImportSpecifier *nis = (ImportSpecifier *)calloc(1, sizeof(ImportSpecifier));
            if (ois) {
                nis->imported = rewrite_tree(ois->imported, opt);
                nis->local = rewrite_tree(ois->local, opt);
            }
            n->data = nis;
            break;
        }
        case AST_ExportNamedDeclaration: {
            ExportNamedDeclaration *oen = (ExportNamedDeclaration *)orig->data;
            ExportNamedDeclaration *nen = (ExportNamedDeclaration *)calloc(1, sizeof(ExportNamedDeclaration));
            astvec_init(&nen->specifiers);
            if (oen) {
                if (oen->source) nen->source = dup_cstr(oen->source);
                for (size_t i = 0; i < oen->specifiers.count; ++i) {
                    maybe_insert(&nen->specifiers, orig, i, opt);
                    AstNode *child = rewrite_tree(oen->specifiers.items[i], opt);
                    if (child) astvec_push(&nen->specifiers, child);
                }
                maybe_insert_after_all(&nen->specifiers, orig, &oen->specifiers, opt);
                nen->declaration = rewrite_tree(oen->declaration, opt);
            }
            n->data = nen;
            break;
        }
        case AST_ExportDefaultDeclaration: {
            ExportDefaultDeclaration *oed = (ExportDefaultDeclaration *)orig->data;
            ExportDefaultDeclaration *ned = (ExportDefaultDeclaration *)calloc(1, sizeof(ExportDefaultDeclaration));
            if (oed) {
                ned->declaration = rewrite_tree(oed->declaration, opt);
                ned->expression = rewrite_tree(oed->expression, opt);
            }
            n->data = ned;
            break;
        }
        case AST_Error: {
            ErrorNode *oe = (ErrorNode *)orig->data;
            ErrorNode *ne = (ErrorNode *)calloc(1, sizeof(ErrorNode));
            if (oe && oe->message) ne->message = dup_cstr(oe->message);
            n->data = ne;
            break;
        }
        default:
            break;
    }

    if (opt && opt->post && n) {
        AstNode *post_out = opt->post(n, opt->post_ctx);
        if (post_out != n) ast_release(n);
        return post_out;
    }

    return n;
}

// --- callbacks for basic edits -------------------------------------------

static AstNode *replace_cb(const AstNode *orig, void *ctx, int *handled) {
    ReplaceCtx *r = (ReplaceCtx *)ctx;
    if (r && orig == r->target) {
        r->applied = 1;
        if (handled) *handled = 1;
        return ast_clone(r->replacement);
    }
    return NULL;
}

static AstNode *remove_cb(const AstNode *orig, void *ctx, int *handled) {
    RemoveCtx *r = (RemoveCtx *)ctx;
    if (r && orig == r->target) {
        r->applied = 1;
        if (handled) *handled = 1;
        return NULL;
    }
    return NULL;
}

static int should_rename(const RenameCtx *rc, const AstNode *orig) {
    if (!rc || !orig) return 0;
    for (size_t i = 0; i < rc->count; ++i) {
        if (rc->nodes[i] == orig) return 1;
    }
    return 0;
}

static AstNode *rename_cb(const AstNode *orig, void *ctx, int *handled) {
    RenameCtx *rc = (RenameCtx *)ctx;
    if (should_rename(rc, orig) && orig->type == AST_Identifier) {
        if (handled) *handled = 1;
        Identifier *oid = (Identifier *)orig->data;
        Position s = orig->start;
        Position e = orig->end;
        (void)oid;
        return ast_identifier(rc->new_name, s, e);
    }
    return NULL;
}

// --- subtree membership helpers -----------------------------------------

static int contains_node(const AstNode *root, const AstNode *needle) {
    if (!root || !needle) return 0;
    if (root == needle) return 1;
    switch (root->type) {
        case AST_Program: {
            Program *p = (Program *)root->data;
            for (size_t i = 0; i < p->body.count; ++i) {
                if (contains_node(p->body.items[i], needle)) return 1;
            }
            break;
        }
        case AST_VariableDeclaration: {
            VariableDeclaration *vd = (VariableDeclaration *)root->data;
            for (size_t i = 0; i < vd->declarations.count; ++i) {
                if (contains_node(vd->declarations.items[i], needle)) return 1;
            }
            break;
        }
        case AST_VariableDeclarator: {
            VariableDeclarator *vd = (VariableDeclarator *)root->data;
            if (contains_node(vd->id, needle) || contains_node(vd->init, needle)) return 1;
            break;
        }
        case AST_ExpressionStatement: {
            ExpressionStatement *es = (ExpressionStatement *)root->data;
            return contains_node(es->expression, needle);
        }
        case AST_UpdateExpression: {
            UpdateExpression *ue = (UpdateExpression *)root->data;
            return contains_node(ue->argument, needle);
        }
        case AST_BinaryExpression: {
            BinaryExpression *be = (BinaryExpression *)root->data;
            return contains_node(be->left, needle) || contains_node(be->right, needle);
        }
        case AST_AssignmentExpression: {
            AssignmentExpression *ae = (AssignmentExpression *)root->data;
            return contains_node(ae->left, needle) || contains_node(ae->right, needle);
        }
        case AST_UnaryExpression: {
            UnaryExpression *ue = (UnaryExpression *)root->data;
            return contains_node(ue->argument, needle);
        }
        case AST_ObjectExpression: {
            ObjectExpression *oe = (ObjectExpression *)root->data;
            for (size_t i = 0; i < oe->properties.count; ++i) if (contains_node(oe->properties.items[i], needle)) return 1;
            break;
        }
        case AST_Property: {
            Property *prop = (Property *)root->data;
            return contains_node(prop->key, needle) || contains_node(prop->value, needle);
        }
        case AST_ArrayExpression: {
            ArrayExpression *ae = (ArrayExpression *)root->data;
            for (size_t i = 0; i < ae->elements.count; ++i) if (contains_node(ae->elements.items[i], needle)) return 1;
            break;
        }
        case AST_MemberExpression: {
            MemberExpression *me = (MemberExpression *)root->data;
            return contains_node(me->object, needle) || contains_node(me->property, needle);
        }
        case AST_CallExpression: {
            CallExpression *ce = (CallExpression *)root->data;
            if (contains_node(ce->callee, needle)) return 1;
            for (size_t i = 0; i < ce->arguments.count; ++i) if (contains_node(ce->arguments.items[i], needle)) return 1;
            break;
        }
        case AST_FunctionDeclaration:
        case AST_FunctionExpression: {
            FunctionBody *fb = (FunctionBody *)root->data;
            for (size_t i = 0; i < fb->params.count; ++i) if (contains_node(fb->params.items[i], needle)) return 1;
            if (contains_node(fb->body, needle)) return 1;
            break;
        }
        case AST_BlockStatement: {
            BlockStatement *bs = (BlockStatement *)root->data;
            for (size_t i = 0; i < bs->body.count; ++i) if (contains_node(bs->body.items[i], needle)) return 1;
            break;
        }
        case AST_IfStatement: {
            IfStatement *is = (IfStatement *)root->data;
            return contains_node(is->test, needle) || contains_node(is->consequent, needle) || contains_node(is->alternate, needle);
        }
        case AST_WhileStatement: {
            WhileStatement *ws = (WhileStatement *)root->data;
            return contains_node(ws->test, needle) || contains_node(ws->body, needle);
        }
        case AST_DoWhileStatement: {
            DoWhileStatement *dw = (DoWhileStatement *)root->data;
            return contains_node(dw->body, needle) || contains_node(dw->test, needle);
        }
        case AST_ForStatement: {
            ForStatement *fs = (ForStatement *)root->data;
            return contains_node(fs->init, needle) || contains_node(fs->test, needle) || contains_node(fs->update, needle) || contains_node(fs->body, needle);
        }
        case AST_SwitchStatement: {
            SwitchStatement *ss = (SwitchStatement *)root->data;
            if (contains_node(ss->discriminant, needle)) return 1;
            for (size_t i = 0; i < ss->cases.count; ++i) if (contains_node(ss->cases.items[i], needle)) return 1;
            break;
        }
        case AST_SwitchCase: {
            SwitchCase *sc = (SwitchCase *)root->data;
            if (contains_node(sc->test, needle)) return 1;
            for (size_t i = 0; i < sc->consequent.count; ++i) if (contains_node(sc->consequent.items[i], needle)) return 1;
            break;
        }
        case AST_TryStatement: {
            TryStatement *ts = (TryStatement *)root->data;
            if (contains_node(ts->block, needle) || contains_node(ts->finalizer, needle)) return 1;
            for (size_t i = 0; i < ts->handlers.count; ++i) if (contains_node(ts->handlers.items[i], needle)) return 1;
            break;
        }
        case AST_CatchClause: {
            CatchClause *cc = (CatchClause *)root->data;
            return contains_node(cc->param, needle) || contains_node(cc->body, needle);
        }
        case AST_ThrowStatement: {
            ThrowStatement *ts = (ThrowStatement *)root->data;
            return contains_node(ts->argument, needle);
        }
        case AST_ReturnStatement: {
            ReturnStatement *rs = (ReturnStatement *)root->data;
            return contains_node(rs->argument, needle);
        }
        case AST_ImportDeclaration: {
            ImportDeclaration *id = (ImportDeclaration *)root->data;
            for (size_t i = 0; i < id->specifiers.count; ++i) if (contains_node(id->specifiers.items[i], needle)) return 1;
            break;
        }
        case AST_ImportSpecifier: {
            ImportSpecifier *is = (ImportSpecifier *)root->data;
            return contains_node(is->imported, needle) || contains_node(is->local, needle);
        }
        case AST_ExportNamedDeclaration: {
            ExportNamedDeclaration *en = (ExportNamedDeclaration *)root->data;
            if (contains_node(en->declaration, needle)) return 1;
            for (size_t i = 0; i < en->specifiers.count; ++i) if (contains_node(en->specifiers.items[i], needle)) return 1;
            break;
        }
        case AST_ExportDefaultDeclaration: {
            ExportDefaultDeclaration *ed = (ExportDefaultDeclaration *)root->data;
            return contains_node(ed->declaration, needle) || contains_node(ed->expression, needle);
        }
        default:
            break;
    }
    return 0;
}

// --- scope helpers -------------------------------------------------------

static Binding *find_binding_by_node(Scope *scope, const AstNode *node) {
    if (!scope || !node) return NULL;
    for (size_t i = 0; i < scope->bindings.count; ++i) {
        Binding *b = scope->bindings.items[i];
        if (b && b->node == node) return b;
    }
    for (size_t i = 0; i < scope->children.count; ++i) {
        Binding *found = find_binding_by_node(scope->children.items[i], node);
        if (found) return found;
    }
    return NULL;
}

static void collect_refs_for_binding(Scope *scope, const Binding *b, RefVec *out) {
    if (!scope || !b || !out) return;
    for (size_t i = 0; i < scope->references.count; ++i) {
        Reference *r = scope->references.items[i];
        if (r && r->resolved == b) refvec_push(out, r);
    }
    for (size_t i = 0; i < scope->children.count; ++i) {
        collect_refs_for_binding(scope->children.items[i], b, out);
    }
}

static int has_intervening_binding(Scope *from, Scope *stop_at, const char *name) {
    Scope *s = from;
    while (s && s != stop_at) {
        if (scope_lookup_local(s, name)) return 1;
        s = s->parent;
    }
    return 0;
}

static int binding_in_subtree(const AstNode *subtree, const Binding *b) {
    return subtree && b && contains_node(subtree, b->node);
}

static void collect_refs_in_subtree(const Scope *scope, const AstNode *subtree, RefVec *out) {
    if (!scope || !subtree || !out) return;
    for (size_t i = 0; i < scope->references.count; ++i) {
        Reference *r = scope->references.items[i];
        if (r && contains_node(subtree, r->node)) refvec_push(out, r);
    }
    for (size_t i = 0; i < scope->children.count; ++i) {
        collect_refs_in_subtree(scope->children.items[i], subtree, out);
    }
}

// --- public API ----------------------------------------------------------

EditStatus edit_replace(AstNode *root, const AstNode *target, AstNode *replacement, AstNode **out_root) {
    if (!root || !target || !replacement || !out_root) return status_err("invalid arguments");
    ReplaceCtx ctx = { target, replacement, 0 };
    RewriteOptions opt = { replace_cb, &ctx, NULL, NULL, NULL, 0, NULL, NULL };
    AstNode *nr = rewrite_tree(root, &opt);
    if (!ctx.applied) { ast_release(nr); return status_err("target not found"); }
    *out_root = nr;
    return status_ok();
}

EditStatus edit_remove(AstNode *root, const AstNode *target, AstNode **out_root) {
    if (!root || !target || !out_root) return status_err("invalid arguments");
    RemoveCtx ctx = { target, 0 };
    RewriteOptions opt = { remove_cb, &ctx, NULL, NULL, NULL, 0, NULL, NULL };
    AstNode *nr = rewrite_tree(root, &opt);
    if (!ctx.applied) { ast_release(nr); return status_err("target not found"); }
    *out_root = nr;
    return status_ok();
}

EditStatus edit_insert(AstNode *root, const AstNode *parent, size_t index, AstNode *node, AstNode **out_root) {
    if (!root || !parent || !node || !out_root) return status_err("invalid arguments");
    int inserted = 0;
    RewriteOptions opt = { NULL, NULL, NULL, NULL, parent, index, node, &inserted };
    AstNode *nr = rewrite_tree(root, &opt);
    if (!inserted) { ast_release(nr); return status_err("parent not found"); }
    *out_root = nr;
    return status_ok();
}

EditStatus edit_move(ScopeManager *sm, AstNode *root, const AstNode *target, const AstNode *new_parent, size_t index, AstNode **out_root) {
    if (!sm || !root || !target || !new_parent || !out_root) return status_err("invalid arguments");
    if (contains_node(target, new_parent)) return status_err("cannot move into itself");

    Scope *insert_scope = scope_of_node(sm, new_parent);
    if (!insert_scope) insert_scope = sm->root;

    // Safety: ensure external bindings remain visible
    RefVec refs = {0};
    collect_refs_in_subtree(sm->root, target, &refs);
    for (size_t i = 0; i < refs.count; ++i) {
        Reference *r = refs.items[i];
        Binding *b = r ? r->resolved : NULL;
        if (!b) continue;
        if (binding_in_subtree(target, b)) continue; // moves together
        Binding *at_new = scope_resolve(insert_scope, b->name);
        if (at_new != b) {
            free(refs.items);
            return status_err("move would change resolution");
        }
    }
    free(refs.items);

    // perform move: remove target and insert clone at new location
    RemoveCtx rm = { target, 0 };
    int inserted = 0;
    RewriteOptions opt = { remove_cb, &rm, NULL, NULL, new_parent, index, (AstNode *)target, &inserted };
    AstNode *nr = rewrite_tree(root, &opt);
    if (!rm.applied || !inserted) {
        ast_release(nr);
        return status_err("move failed (target or parent not found)");
    }
    *out_root = nr;
    return status_ok();
}

EditStatus edit_rename(ScopeManager *sm, AstNode *root, const AstNode *binding_identifier, const char *new_name, AstNode **out_root) {
    if (!sm || !root || !binding_identifier || !new_name || !out_root) return status_err("invalid arguments");
    if (!new_name[0]) return status_err("empty name");

    Binding *b = find_binding_by_node(sm->root, binding_identifier);
    if (!b) return status_err("binding not found");

    // conflict in same scope
    Binding *local = scope_lookup_local(b->scope, new_name);
    if (local && local != b) return status_err("name already bound in scope");

    RefVec refs = {0};
    collect_refs_for_binding(sm->root, b, &refs);
    for (size_t i = 0; i < refs.count; ++i) {
        Reference *r = refs.items[i];
        if (!r) continue;
        if (has_intervening_binding(r->scope, b->scope, new_name)) {
            free(refs.items);
            return status_err("rename would be captured by inner binding");
        }
    }

    NodePtrVec nodes = {0};
    nodeptrvec_push(&nodes, binding_identifier);
    for (size_t i = 0; i < refs.count; ++i) {
        Reference *r = refs.items[i];
        if (r && r->node) nodeptrvec_push(&nodes, r->node);
    }
    free(refs.items);

    RenameCtx rc = { nodes.items, nodes.count, new_name };
    RewriteOptions opt = { rename_cb, &rc, NULL, NULL, NULL, 0, NULL, NULL };
    AstNode *nr = rewrite_tree(root, &opt);
    free(nodes.items);
    if (!nr) return status_err("rename failed");
    *out_root = nr;
    return status_ok();
}

static AstNode *transform_post(AstNode *node, void *ctx) {
    TransformCtx *tc = (TransformCtx *)ctx;
    if (!tc || !tc->visitor || !node) return node;
    AstNode *next = tc->visitor(node, tc->userdata);
    if (!next) {
        ast_release(node);
        return NULL;
    }
    if (next == node) return node;
    ast_release(node);
    return next;
}

AstNode *edit_transform(AstNode *root, EditVisitor visitor, void *userdata) {
    if (!root || !visitor) return NULL;
    TransformCtx tc = { visitor, userdata };
    RewriteOptions opt = { NULL, NULL, transform_post, &tc, NULL, 0, NULL, NULL };
    return rewrite_tree(root, &opt);
}
