#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quickjsflow/ast.h"

static char *dupstr(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *d = (char *)malloc(len + 1);
    if (!d) return NULL;
    memcpy(d, s, len);
    d[len] = '\0';
    return d;
}

static void print_escaped(const char *s) {
    if (!s) return;
    for (const char *p = s; *p; ++p) {
        switch (*p) {
            case '"': printf("\\\""); break;
            case '\\': printf("\\\\"); break;
            case '\n': printf("\\n"); break;
            case '\r': printf("\\r"); break;
            case '\t': printf("\\t"); break;
            case '\b': printf("\\b"); break;
            case '\f': printf("\\f"); break;
            default:
                if (*p < 32 || *p == 127) {
                    printf("\\u%04x", (unsigned char)*p);
                } else {
                    putchar(*p);
                }
                break;
        }
    }
}

void astvec_init(AstVec *v) {
    v->items = NULL;
    v->count = 0;
    v->capacity = 0;
}

void astvec_push(AstVec *v, AstNode *n) {
    if (v->count + 1 > v->capacity) {
        size_t cap = v->capacity ? v->capacity * 2 : 4;
        AstNode **items = (AstNode **)realloc(v->items, cap * sizeof(AstNode *));
        if (!items) return;
        v->items = items;
        v->capacity = cap;
    }
    v->items[v->count++] = n;
}

void commentvec_push(Program *p, Comment *c) {
    if (!p || !c) return;
    if (p->comment_count + 1 > p->comment_capacity) {
        size_t cap = p->comment_capacity ? p->comment_capacity * 2 : 4;
        Comment **items = (Comment **)realloc(p->comments, cap * sizeof(Comment *));
        if (!items) return;
        p->comments = items;
        p->comment_capacity = cap;
    }
    p->comments[p->comment_count++] = c;
}

Comment *comment_clone(const Comment *c) {
    if (!c) return NULL;
    Comment *nc = (Comment *)calloc(1, sizeof(Comment));
    if (!nc) return NULL;
    nc->is_block = c->is_block;
    if (c->text) {
        size_t len = strlen(c->text);
        nc->text = (char *)malloc(len + 1);
        if (nc->text) memcpy(nc->text, c->text, len + 1);
    }
    nc->start = c->start;
    nc->end = c->end;
    return nc;
}

static AstNode *new_node(AstNodeType t) {
    AstNode *n = (AstNode *)calloc(1, sizeof(AstNode));
    if (n) {
        n->type = t;
        n->refcount = 1;
    }
    return n;
}

AstNode *ast_program(void) {
    AstNode *n = new_node(AST_Program);
    if (!n) return NULL;
    Program *p = (Program *)calloc(1, sizeof(Program));
    astvec_init(&p->body);
    p->comments = NULL;
    p->comment_count = 0;
    p->comment_capacity = 0;
    n->data = p;
    return n;
}

AstNode *ast_identifier(const char *name, Position s, Position e) {
    AstNode *n = new_node(AST_Identifier);
    if (!n) return NULL;
    Identifier *id = (Identifier *)calloc(1, sizeof(Identifier));
    id->name = dupstr(name);
    n->data = id;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_literal(LiteralKind kind, const char *raw, Position s, Position e) {
    AstNode *n = new_node(AST_Literal);
    Literal *lit = (Literal *)calloc(1, sizeof(Literal));
    lit->kind = kind;
    lit->raw = dupstr(raw);
    n->data = lit;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_variable_declaration(VarKind kind) {
    AstNode *n = new_node(AST_VariableDeclaration);
    VariableDeclaration *vd = (VariableDeclaration *)calloc(1, sizeof(VariableDeclaration));
    vd->kind = kind;
    astvec_init(&vd->declarations);
    n->data = vd;
    return n;
}

AstNode *ast_variable_declarator(AstNode *id, AstNode *init) {
    AstNode *n = new_node(AST_VariableDeclarator);
    VariableDeclarator *vd = (VariableDeclarator *)calloc(1, sizeof(VariableDeclarator));
    vd->id = id;
    vd->init = init;
    n->data = vd;
    return n;
}

AstNode *ast_expression_statement(AstNode *expr, Position s, Position e) {
    AstNode *n = new_node(AST_ExpressionStatement);
    ExpressionStatement *es = (ExpressionStatement *)calloc(1, sizeof(ExpressionStatement));
    es->expression = expr;
    n->data = es;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_update_expression(const char *op, int prefix, AstNode *arg, Position s, Position e) {
    AstNode *n = new_node(AST_UpdateExpression);
    UpdateExpression *ue = (UpdateExpression *)calloc(1, sizeof(UpdateExpression));
    ue->operator = dupstr(op);
    ue->prefix = prefix;
    ue->argument = arg;
    n->data = ue;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_binary_expression(const char *op, AstNode *left, AstNode *right, Position s, Position e) {
    AstNode *n = new_node(AST_BinaryExpression);
    BinaryExpression *be = (BinaryExpression *)calloc(1, sizeof(BinaryExpression));
    be->operator = dupstr(op);
    be->left = left; be->right = right;
    n->data = be;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_assignment_expression(const char *op, AstNode *left, AstNode *right, Position s, Position e) {
    AstNode *n = new_node(AST_AssignmentExpression);
    AssignmentExpression *ae = (AssignmentExpression *)calloc(1, sizeof(AssignmentExpression));
    ae->operator = dupstr(op);
    ae->left = left;
    ae->right = right;
    n->data = ae;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_unary_expression(const char *op, int prefix, AstNode *arg, Position s, Position e) {
    AstNode *n = new_node(AST_UnaryExpression);
    UnaryExpression *ue = (UnaryExpression *)calloc(1, sizeof(UnaryExpression));
    ue->operator = dupstr(op);
    ue->prefix = prefix;
    ue->argument = arg;
    n->data = ue;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_object_expression(Position s, Position e) {
    AstNode *n = new_node(AST_ObjectExpression);
    ObjectExpression *obj = (ObjectExpression *)calloc(1, sizeof(ObjectExpression));
    astvec_init(&obj->properties);
    n->data = obj;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_property(AstNode *key, AstNode *value, int computed) {
    AstNode *n = new_node(AST_Property);
    Property *prop = (Property *)calloc(1, sizeof(Property));
    prop->key = key;
    prop->value = value;
    prop->computed = computed;
    n->data = prop;
    return n;
}

AstNode *ast_array_expression(Position s, Position e) {
    AstNode *n = new_node(AST_ArrayExpression);
    ArrayExpression *arr = (ArrayExpression *)calloc(1, sizeof(ArrayExpression));
    astvec_init(&arr->elements);
    n->data = arr;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_member_expression(AstNode *obj, AstNode *prop, int computed, Position s, Position e) {
    AstNode *n = new_node(AST_MemberExpression);
    MemberExpression *me = (MemberExpression *)calloc(1, sizeof(MemberExpression));
    me->object = obj;
    me->property = prop;
    me->computed = computed;
    n->data = me;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_call_expression(AstNode *callee, Position s, Position e) {
    AstNode *n = new_node(AST_CallExpression);
    CallExpression *ce = (CallExpression *)calloc(1, sizeof(CallExpression));
    ce->callee = callee;
    astvec_init(&ce->arguments);
    n->data = ce;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_function_declaration(const char *name, Position s, Position e) {
    AstNode *n = new_node(AST_FunctionDeclaration);
    FunctionBody *fb = (FunctionBody *)calloc(1, sizeof(FunctionBody));
    fb->name = dupstr(name);
    astvec_init(&fb->params);
    n->data = fb;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_function_expression(const char *name, Position s, Position e) {
    AstNode *n = new_node(AST_FunctionExpression);
    FunctionBody *fb = (FunctionBody *)calloc(1, sizeof(FunctionBody));
    fb->name = dupstr(name);
    astvec_init(&fb->params);
    n->data = fb;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_block_statement(Position s, Position e) {
    AstNode *n = new_node(AST_BlockStatement);
    BlockStatement *bs = (BlockStatement *)calloc(1, sizeof(BlockStatement));
    astvec_init(&bs->body);
    n->data = bs;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_if_statement(AstNode *test, AstNode *cons, AstNode *alt, Position s, Position e) {
    AstNode *n = new_node(AST_IfStatement);
    IfStatement *is = (IfStatement *)calloc(1, sizeof(IfStatement));
    is->test = test;
    is->consequent = cons;
    is->alternate = alt;
    n->data = is;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_while_statement(AstNode *test, AstNode *body, Position s, Position e) {
    AstNode *n = new_node(AST_WhileStatement);
    WhileStatement *ws = (WhileStatement *)calloc(1, sizeof(WhileStatement));
    ws->test = test;
    ws->body = body;
    n->data = ws;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_do_while_statement(AstNode *body, AstNode *test, Position s, Position e) {
    AstNode *n = new_node(AST_DoWhileStatement);
    DoWhileStatement *dws = (DoWhileStatement *)calloc(1, sizeof(DoWhileStatement));
    dws->body = body;
    dws->test = test;
    n->data = dws;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_for_statement(AstNode *init, AstNode *test, AstNode *update, AstNode *body, Position s, Position e) {
    AstNode *n = new_node(AST_ForStatement);
    ForStatement *fs = (ForStatement *)calloc(1, sizeof(ForStatement));
    fs->init = init;
    fs->test = test;
    fs->update = update;
    fs->body = body;
    n->data = fs;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_switch_statement(AstNode *discriminant, Position s, Position e) {
    AstNode *n = new_node(AST_SwitchStatement);
    SwitchStatement *ss = (SwitchStatement *)calloc(1, sizeof(SwitchStatement));
    ss->discriminant = discriminant;
    astvec_init(&ss->cases);
    n->data = ss;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_switch_case(AstNode *test) {
    AstNode *n = new_node(AST_SwitchCase);
    SwitchCase *sc = (SwitchCase *)calloc(1, sizeof(SwitchCase));
    sc->test = test;  // NULL for default case
    astvec_init(&sc->consequent);
    n->data = sc;
    return n;
}

AstNode *ast_try_statement(AstNode *block, Position s, Position e) {
    AstNode *n = new_node(AST_TryStatement);
    TryStatement *ts = (TryStatement *)calloc(1, sizeof(TryStatement));
    ts->block = block;
    astvec_init(&ts->handlers);
    n->data = ts;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_catch_clause(AstNode *param, AstNode *body) {
    AstNode *n = new_node(AST_CatchClause);
    CatchClause *cc = (CatchClause *)calloc(1, sizeof(CatchClause));
    cc->param = param;
    cc->body = body;
    n->data = cc;
    return n;
}

AstNode *ast_throw_statement(AstNode *argument, Position s, Position e) {
    AstNode *n = new_node(AST_ThrowStatement);
    ThrowStatement *ts = (ThrowStatement *)calloc(1, sizeof(ThrowStatement));
    ts->argument = argument;
    n->data = ts;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_return_statement(AstNode *argument, Position s, Position e) {
    AstNode *n = new_node(AST_ReturnStatement);
    ReturnStatement *rs = (ReturnStatement *)calloc(1, sizeof(ReturnStatement));
    rs->argument = argument;
    n->data = rs;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_break_statement(Position s, Position e) {
    AstNode *n = new_node(AST_BreakStatement);
    BreakStatement *bs = (BreakStatement *)calloc(1, sizeof(BreakStatement));
    bs->label = NULL;
    n->data = bs;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_continue_statement(Position s, Position e) {
    AstNode *n = new_node(AST_ContinueStatement);
    ContinueStatement *cs = (ContinueStatement *)calloc(1, sizeof(ContinueStatement));
    cs->label = NULL;
    n->data = cs;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_import_declaration(const char *source, Position s, Position e) {
    AstNode *n = new_node(AST_ImportDeclaration);
    ImportDeclaration *id = (ImportDeclaration *)calloc(1, sizeof(ImportDeclaration));
    id->source = dupstr(source);
    astvec_init(&id->specifiers);
    n->data = id;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_import_specifier(AstNode *imported, AstNode *local) {
    AstNode *n = new_node(AST_ImportSpecifier);
    ImportSpecifier *is = (ImportSpecifier *)calloc(1, sizeof(ImportSpecifier));
    is->imported = imported;
    is->local = local;
    n->data = is;
    return n;
}

AstNode *ast_import_default_specifier(AstNode *local, Position s, Position e) {
    AstNode *n = new_node(AST_ImportDefaultSpecifier);
    ImportDefaultSpecifier *ids = (ImportDefaultSpecifier *)calloc(1, sizeof(ImportDefaultSpecifier));
    ids->local = local;
    n->data = ids;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_import_namespace_specifier(AstNode *local, Position s, Position e) {
    AstNode *n = new_node(AST_ImportNamespaceSpecifier);
    ImportNamespaceSpecifier *ins = (ImportNamespaceSpecifier *)calloc(1, sizeof(ImportNamespaceSpecifier));
    ins->local = local;
    n->data = ins;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_export_named_declaration(const char *source, Position s, Position e) {
    AstNode *n = new_node(AST_ExportNamedDeclaration);
    ExportNamedDeclaration *end = (ExportNamedDeclaration *)calloc(1, sizeof(ExportNamedDeclaration));
    end->source = dupstr(source);
    astvec_init(&end->specifiers);
    n->data = end;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_export_default_declaration(Position s, Position e) {
    AstNode *n = new_node(AST_ExportDefaultDeclaration);
    ExportDefaultDeclaration *edd = (ExportDefaultDeclaration *)calloc(1, sizeof(ExportDefaultDeclaration));
    n->data = edd;
    n->start = s; n->end = e;
    return n;
}

// Phase 2: Modern Features (ES6+)

AstNode *ast_arrow_function_expression(int is_async, Position s, Position e) {
    AstNode *n = new_node(AST_ArrowFunctionExpression);
    ArrowFunctionExpression *afe = (ArrowFunctionExpression *)calloc(1, sizeof(ArrowFunctionExpression));
    astvec_init(&afe->params);
    afe->body = NULL;
    afe->is_async = is_async;
    n->data = afe;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_template_literal(Position s, Position e) {
    AstNode *n = new_node(AST_TemplateLiteral);
    TemplateLiteral *tl = (TemplateLiteral *)calloc(1, sizeof(TemplateLiteral));
    astvec_init(&tl->quasis);
    astvec_init(&tl->expressions);
    n->data = tl;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_template_element(const char *value, int tail, Position s, Position e) {
    AstNode *n = new_node(AST_TemplateElement);
    TemplateElement *te = (TemplateElement *)calloc(1, sizeof(TemplateElement));
    te->value = dupstr(value);
    te->tail = tail;
    n->data = te;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_spread_element(AstNode *argument, Position s, Position e) {
    AstNode *n = new_node(AST_SpreadElement);
    SpreadElement *se = (SpreadElement *)calloc(1, sizeof(SpreadElement));
    se->argument = argument;
    n->data = se;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_object_pattern(Position s, Position e) {
    AstNode *n = new_node(AST_ObjectPattern);
    ObjectPattern *op = (ObjectPattern *)calloc(1, sizeof(ObjectPattern));
    astvec_init(&op->properties);
    n->data = op;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_array_pattern(Position s, Position e) {
    AstNode *n = new_node(AST_ArrayPattern);
    ArrayPattern *ap = (ArrayPattern *)calloc(1, sizeof(ArrayPattern));
    astvec_init(&ap->elements);
    n->data = ap;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_assignment_pattern(AstNode *left, AstNode *right, Position s, Position e) {
    AstNode *n = new_node(AST_AssignmentPattern);
    AssignmentPattern *ap = (AssignmentPattern *)calloc(1, sizeof(AssignmentPattern));
    ap->left = left;
    ap->right = right;
    n->data = ap;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_rest_element(AstNode *argument, Position s, Position e) {
    AstNode *n = new_node(AST_RestElement);
    RestElement *re = (RestElement *)calloc(1, sizeof(RestElement));
    re->argument = argument;
    n->data = re;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_for_of_statement(AstNode *left, AstNode *right, AstNode *body, Position s, Position e) {
    AstNode *n = new_node(AST_ForOfStatement);
    ForOfStatement *fos = (ForOfStatement *)calloc(1, sizeof(ForOfStatement));
    fos->left = left;
    fos->right = right;
    fos->body = body;
    n->data = fos;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_for_in_statement(AstNode *left, AstNode *right, AstNode *body, Position s, Position e) {
    AstNode *n = new_node(AST_ForInStatement);
    ForInStatement *fis = (ForInStatement *)calloc(1, sizeof(ForInStatement));
    fis->left = left;
    fis->right = right;
    fis->body = body;
    n->data = fis;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_class_declaration(AstNode *id, AstNode *superClass, Position s, Position e) {
    AstNode *n = new_node(AST_ClassDeclaration);
    ClassDeclaration *cd = (ClassDeclaration *)calloc(1, sizeof(ClassDeclaration));
    cd->id = id;
    cd->superClass = superClass;
    astvec_init(&cd->body);
    n->data = cd;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_class_expression(AstNode *id, AstNode *superClass, Position s, Position e) {
    AstNode *n = new_node(AST_ClassExpression);
    ClassExpression *ce = (ClassExpression *)calloc(1, sizeof(ClassExpression));
    ce->id = id;
    ce->superClass = superClass;
    astvec_init(&ce->body);
    n->data = ce;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_method_definition(AstNode *key, AstNode *value, const char *kind, int is_static, Position s, Position e) {
    AstNode *n = new_node(AST_MethodDefinition);
    MethodDefinition *md = (MethodDefinition *)calloc(1, sizeof(MethodDefinition));
    md->key = key;
    md->value = value;
    md->kind = dupstr(kind);
    md->is_static = is_static;
    n->data = md;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_await_expression(AstNode *argument, Position s, Position e) {
    AstNode *n = new_node(AST_AwaitExpression);
    AwaitExpression *ae = (AwaitExpression *)calloc(1, sizeof(AwaitExpression));
    ae->argument = argument;
    n->data = ae;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_yield_expression(AstNode *argument, int delegate, Position s, Position e) {
    AstNode *n = new_node(AST_YieldExpression);
    YieldExpression *ye = (YieldExpression *)calloc(1, sizeof(YieldExpression));
    ye->argument = argument;
    ye->delegate = delegate;
    n->data = ye;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_super(Position s, Position e) {
    AstNode *n = new_node(AST_Super);
    Super *sup = (Super *)calloc(1, sizeof(Super));
    n->data = sup;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_this_expression(Position s, Position e) {
    AstNode *n = new_node(AST_ThisExpression);
    ThisExpression *te = (ThisExpression *)calloc(1, sizeof(ThisExpression));
    n->data = te;
    n->start = s; n->end = e;
    return n;
}

AstNode *ast_error(const char *msg, Position s, Position e) {
    AstNode *n = new_node(AST_Error);
    ErrorNode *er = (ErrorNode *)calloc(1, sizeof(ErrorNode));
    er->message = dupstr(msg);
    n->data = er;
    n->start = s; n->end = e;
    return n;
}

static void print_pos(const char *key, Position p) {
    printf("\"%s\":{\"line\":%d,\"column\":%d}", key, p.line, p.column);
}

static void print_node(const AstNode *n);

static void print_program(const Program *p) {
    printf("\"body\":[");
    for (size_t i = 0; i < p->body.count; ++i) {
        if (i) printf(",");
        print_node(p->body.items[i]);
    }
    printf("]");
}

static void print_identifier(const Identifier *id) {
    printf("\"name\":\"");
    print_escaped(id->name);
    printf("\"");
}

static void print_literal(const Literal *lit) {
    printf("\"raw\":\"");
    print_escaped(lit->raw);
    printf("\"");
}

static void print_variable_declaration(const VariableDeclaration *vd) {
    const char *kind = vd->kind == VD_Var ? "var" : (vd->kind == VD_Let ? "let" : "const");
    printf("\"kind\":\"%s\",\"declarations\":[", kind);
    for (size_t i = 0; i < vd->declarations.count; ++i) {
        if (i) printf(",");
        print_node(vd->declarations.items[i]);
    }
    printf("]");
}

static void print_variable_declarator(const VariableDeclarator *vd) {
    printf("\"id\":");
    print_node(vd->id);
    printf(",\"init\":");
    if (vd->init) print_node(vd->init); else printf("null");
}

static void print_expression_statement(const ExpressionStatement *es) {
    printf("\"expression\":");
    print_node(es->expression);
}

static void print_update_expression(const UpdateExpression *ue) {
    printf("\"operator\":\""); print_escaped(ue->operator); printf("\",");
    printf("\"prefix\":%d,\"argument\":", ue->prefix);
    print_node(ue->argument);
}

static void print_binary_expression(const BinaryExpression *be) {
    printf("\"operator\":\""); print_escaped(be->operator); printf("\",");
    printf("\"left\":"); print_node(be->left);
    printf(",\"right\":"); print_node(be->right);
}

static void print_assignment_expression(const AssignmentExpression *ae) {
    printf("\"operator\":\""); print_escaped(ae->operator); printf("\",");
    printf("\"left\":"); print_node(ae->left);
    printf(",\"right\":"); print_node(ae->right);
}

static void print_unary_expression(const UnaryExpression *ue) {
    printf("\"operator\":\""); print_escaped(ue->operator); printf("\",");
    printf("\"prefix\":%d,\"argument\":", ue->prefix);
    print_node(ue->argument);
}

static void print_object_expression(const ObjectExpression *obj) {
    printf("\"properties\":[");
    for (size_t i = 0; i < obj->properties.count; ++i) {
        if (i) printf(",");
        print_node(obj->properties.items[i]);
    }
    printf("]");
}

static void print_property(const Property *prop) {
    printf("\"key\":"); print_node(prop->key);
    printf(",\"value\":"); print_node(prop->value);
    printf(",\"computed\":%d", prop->computed);
}

static void print_array_expression(const ArrayExpression *arr) {
    printf("\"elements\":[");
    for (size_t i = 0; i < arr->elements.count; ++i) {
        if (i) printf(",");
        if (arr->elements.items[i]) print_node(arr->elements.items[i]);
        else printf("null");
    }
    printf("]");
}

static void print_member_expression(const MemberExpression *me) {
    printf("\"object\":"); print_node(me->object);
    printf(",\"property\":"); print_node(me->property);
    printf(",\"computed\":%d", me->computed);
}

static void print_call_expression(const CallExpression *ce) {
    printf("\"callee\":"); print_node(ce->callee);
    printf(",\"arguments\":[");
    for (size_t i = 0; i < ce->arguments.count; ++i) {
        if (i) printf(",");
        print_node(ce->arguments.items[i]);
    }
    printf("]");
}

static void print_function_body(const FunctionBody *fb) {
    if (fb->name) { printf("\"id\":{\"type\":\"Identifier\",\"name\":\""); print_escaped(fb->name); printf("\"},"); }
    else { printf("\"id\":null,"); }
    printf("\"params\":[");
    for (size_t i = 0; i < fb->params.count; ++i) {
        if (i) printf(",");
        print_node(fb->params.items[i]);
    }
    printf("],\"body\":");
    if (fb->body) print_node(fb->body); else printf("null");
}

static void print_block_statement(const BlockStatement *bs) {
    printf("\"body\":[");
    for (size_t i = 0; i < bs->body.count; ++i) {
        if (i) printf(",");
        print_node(bs->body.items[i]);
    }
    printf("]");
}

static void print_if_statement(const IfStatement *is) {
    printf("\"test\":"); print_node(is->test);
    printf(",\"consequent\":"); print_node(is->consequent);
    printf(",\"alternate\":"); if (is->alternate) print_node(is->alternate); else printf("null");
}

static void print_while_statement(const WhileStatement *ws) {
    printf("\"test\":"); print_node(ws->test);
    printf(",\"body\":"); print_node(ws->body);
}

static void print_do_while_statement(const DoWhileStatement *dws) {
    printf("\"body\":"); print_node(dws->body);
    printf(",\"test\":"); print_node(dws->test);
}

static void print_for_statement(const ForStatement *fs) {
    printf("\"init\":"); if (fs->init) print_node(fs->init); else printf("null");
    printf(",\"test\":"); if (fs->test) print_node(fs->test); else printf("null");
    printf(",\"update\":"); if (fs->update) print_node(fs->update); else printf("null");
    printf(",\"body\":"); print_node(fs->body);
}

static void print_switch_statement(const SwitchStatement *ss) {
    printf("\"discriminant\":"); print_node(ss->discriminant);
    printf(",\"cases\":[");
    for (size_t i = 0; i < ss->cases.count; ++i) {
        if (i) printf(",");
        print_node(ss->cases.items[i]);
    }
    printf("]");
}

static void print_switch_case(const SwitchCase *sc) {
    printf("\"test\":"); if (sc->test) print_node(sc->test); else printf("null");
    printf(",\"consequent\":[");
    for (size_t i = 0; i < sc->consequent.count; ++i) {
        if (i) printf(",");
        print_node(sc->consequent.items[i]);
    }
    printf("]");
}

static void print_try_statement(const TryStatement *ts) {
    printf("\"block\":"); print_node(ts->block);
    printf(",\"handlers\":[");
    for (size_t i = 0; i < ts->handlers.count; ++i) {
        if (i) printf(",");
        print_node(ts->handlers.items[i]);
    }
    printf("],\"finalizer\":"); if (ts->finalizer) print_node(ts->finalizer); else printf("null");
}

static void print_catch_clause(const CatchClause *cc) {
    printf("\"param\":"); if (cc->param) print_node(cc->param); else printf("null");
    printf(",\"body\":"); print_node(cc->body);
}

static void print_throw_statement(const ThrowStatement *ts) {
    printf("\"argument\":"); print_node(ts->argument);
}

static void print_return_statement(const ReturnStatement *rs) {
    printf("\"argument\":"); if (rs->argument) print_node(rs->argument); else printf("null");
}

static void print_break_statement(const BreakStatement *bs) {
    (void)bs;
    printf("\"label\":null");
}

static void print_continue_statement(const ContinueStatement *cs) {
    (void)cs;
    printf("\"label\":null");
}

static void print_import_declaration(const ImportDeclaration *id) {
    printf("\"specifiers\":[");
    for (size_t i = 0; i < id->specifiers.count; ++i) {
        if (i) printf(",");
        print_node(id->specifiers.items[i]);
    }
    printf("],\"source\":\""); print_escaped(id->source); printf("\"");
}

static void print_import_specifier(const ImportSpecifier *is) {
    printf("\"imported\":"); print_node(is->imported);
    printf(",\"local\":"); print_node(is->local);
}

static void print_import_default_specifier(const ImportDefaultSpecifier *ids) {
    printf("\"local\":"); print_node(ids->local);
}

static void print_import_namespace_specifier(const ImportNamespaceSpecifier *ins) {
    printf("\"local\":"); print_node(ins->local);
}

static void print_export_named_declaration(const ExportNamedDeclaration *end) {
    printf("\"specifiers\":[");
    for (size_t i = 0; i < end->specifiers.count; ++i) {
        if (i) printf(",");
        print_node(end->specifiers.items[i]);
    }
    printf("],\"source\":"); if (end->source) { printf("\""); print_escaped(end->source); printf("\""); } else printf("null");
    printf(",\"declaration\":"); if (end->declaration) print_node(end->declaration); else printf("null");
}

static void print_export_default_declaration(const ExportDefaultDeclaration *edd) {
    printf("\"declaration\":"); if (edd->declaration) print_node(edd->declaration); else printf("null");
    printf(",\"expression\":"); if (edd->expression) print_node(edd->expression); else printf("null");
}

// Phase 2: Modern Feature Print Functions
static void print_arrow_function_expression(const ArrowFunctionExpression *afe) {
    printf("\"async\":%s", afe->is_async ? "true" : "false");
    printf(",\"params\":[");
    for (size_t i = 0; i < afe->params.count; ++i) {
        if (i) printf(",");
        print_node(afe->params.items[i]);
    }
    printf("],\"body\":");
    if (afe->body) print_node(afe->body); else printf("null");
}

static void print_template_literal(const TemplateLiteral *tl) {
    printf("\"quasis\":[");
    for (size_t i = 0; i < tl->quasis.count; ++i) {
        if (i) printf(",");
        print_node(tl->quasis.items[i]);
    }
    printf("],\"expressions\":[");
    for (size_t i = 0; i < tl->expressions.count; ++i) {
        if (i) printf(",");
        print_node(tl->expressions.items[i]);
    }
    printf("]");
}

static void print_template_element(const TemplateElement *te) {
    printf("\"value\":{\"raw\":\""); print_escaped(te->value); printf("\"}");
    printf(",\"tail\":%s", te->tail ? "true" : "false");
}

static void print_spread_element(const SpreadElement *se) {
    printf("\"argument\":"); if (se->argument) print_node(se->argument); else printf("null");
}

static void print_rest_element(const RestElement *re) {
    printf("\"argument\":"); if (re->argument) print_node(re->argument); else printf("null");
}

static void print_for_of_statement(const ForOfStatement *fos) {
    printf("\"left\":"); if (fos->left) print_node(fos->left); else printf("null");
    printf(",\"right\":"); if (fos->right) print_node(fos->right); else printf("null");
    printf(",\"body\":"); if (fos->body) print_node(fos->body); else printf("null");
    printf(",\"await\":false"); // TODO: Add await support if needed
}

static void print_for_in_statement(const ForInStatement *fis) {
    printf("\"left\":"); if (fis->left) print_node(fis->left); else printf("null");
    printf(",\"right\":"); if (fis->right) print_node(fis->right); else printf("null");
    printf(",\"body\":"); if (fis->body) print_node(fis->body); else printf("null");
}

static void print_class_declaration(const ClassDeclaration *cd) {
    printf("\"id\":"); if (cd->id) print_node(cd->id); else printf("null");
    printf(",\"superClass\":"); if (cd->superClass) print_node(cd->superClass); else printf("null");
    printf(",\"body\":{\"type\":\"ClassBody\",\"body\":[");
    for (size_t i = 0; i < cd->body.count; ++i) {
        if (i) printf(",");
        print_node(cd->body.items[i]);
    }
    printf("]}");
}

static void print_class_expression(const ClassExpression *ce) {
    printf("\"id\":"); if (ce->id) print_node(ce->id); else printf("null");
    printf(",\"superClass\":"); if (ce->superClass) print_node(ce->superClass); else printf("null");
    printf(",\"body\":{\"type\":\"ClassBody\",\"body\":[");
    for (size_t i = 0; i < ce->body.count; ++i) {
        if (i) printf(",");
        print_node(ce->body.items[i]);
    }
    printf("]}");
}

static void print_method_definition(const MethodDefinition *md) {
    printf("\"kind\":\"");
    if (md->kind) {
        print_escaped(md->kind);
    } else {
        printf("method");
    }
    printf("\",\"key\":"); if (md->key) print_node(md->key); else printf("null");
    printf(",\"value\":"); if (md->value) print_node(md->value); else printf("null");
    printf(",\"static\":%s", md->is_static ? "true" : "false");
}

static void print_error(const ErrorNode *er) {
    printf("\"message\":\""); print_escaped(er->message); printf("\"");
}

static void print_node(const AstNode *n) {
    if (!n) { printf("null"); return; }
    printf("{");
    const char *type = "Unknown";
    switch (n->type) {
        case AST_Program: type = "Program"; break;
        case AST_VariableDeclaration: type = "VariableDeclaration"; break;
        case AST_VariableDeclarator: type = "VariableDeclarator"; break;
        case AST_Identifier: type = "Identifier"; break;
        case AST_Literal: type = "Literal"; break;
        case AST_ExpressionStatement: type = "ExpressionStatement"; break;
        case AST_UpdateExpression: type = "UpdateExpression"; break;
        case AST_BinaryExpression: type = "BinaryExpression"; break;
        case AST_AssignmentExpression: type = "AssignmentExpression"; break;
        case AST_UnaryExpression: type = "UnaryExpression"; break;
        case AST_ObjectExpression: type = "ObjectExpression"; break;
        case AST_Property: type = "Property"; break;
        case AST_ArrayExpression: type = "ArrayExpression"; break;
        case AST_MemberExpression: type = "MemberExpression"; break;
        case AST_CallExpression: type = "CallExpression"; break;
        case AST_FunctionDeclaration: type = "FunctionDeclaration"; break;
        case AST_FunctionExpression: type = "FunctionExpression"; break;
        case AST_BlockStatement: type = "BlockStatement"; break;
        case AST_IfStatement: type = "IfStatement"; break;
        case AST_WhileStatement: type = "WhileStatement"; break;
        case AST_DoWhileStatement: type = "DoWhileStatement"; break;
        case AST_ForStatement: type = "ForStatement"; break;
        case AST_SwitchStatement: type = "SwitchStatement"; break;
        case AST_SwitchCase: type = "SwitchCase"; break;
        case AST_TryStatement: type = "TryStatement"; break;
        case AST_CatchClause: type = "CatchClause"; break;
        case AST_ThrowStatement: type = "ThrowStatement"; break;
        case AST_ReturnStatement: type = "ReturnStatement"; break;
        case AST_BreakStatement: type = "BreakStatement"; break;
        case AST_ContinueStatement: type = "ContinueStatement"; break;
        case AST_ImportDeclaration: type = "ImportDeclaration"; break;
        case AST_ImportSpecifier: type = "ImportSpecifier"; break;
        case AST_ImportDefaultSpecifier: type = "ImportDefaultSpecifier"; break;
        case AST_ImportNamespaceSpecifier: type = "ImportNamespaceSpecifier"; break;
        case AST_ExportNamedDeclaration: type = "ExportNamedDeclaration"; break;
        case AST_ExportDefaultDeclaration: type = "ExportDefaultDeclaration"; break;
        case AST_Error: type = "Error"; break;
        // Phase 2: Modern Features
        case AST_ArrowFunctionExpression: type = "ArrowFunctionExpression"; break;
        case AST_TemplateLiteral: type = "TemplateLiteral"; break;
        case AST_TemplateElement: type = "TemplateElement"; break;
        case AST_SpreadElement: type = "SpreadElement"; break;
        case AST_ObjectPattern: type = "ObjectPattern"; break;
        case AST_ArrayPattern: type = "ArrayPattern"; break;
        case AST_AssignmentPattern: type = "AssignmentPattern"; break;
        case AST_RestElement: type = "RestElement"; break;
        case AST_ForOfStatement: type = "ForOfStatement"; break;
        case AST_ForInStatement: type = "ForInStatement"; break;
        case AST_ClassDeclaration: type = "ClassDeclaration"; break;
        case AST_ClassExpression: type = "ClassExpression"; break;
        case AST_MethodDefinition: type = "MethodDefinition"; break;
        case AST_AwaitExpression: type = "AwaitExpression"; break;
        case AST_YieldExpression: type = "YieldExpression"; break;
        case AST_Super: type = "Super"; break;
        case AST_ThisExpression: type = "ThisExpression"; break;
        default: break;
    }
    printf("\"type\":\"%s\",", type);
    print_pos("start", n->start);
    printf(",");
    print_pos("end", n->end);
    printf(",");
    switch (n->type) {
        case AST_Program: print_program((const Program *)n->data); break;
        case AST_VariableDeclaration: print_variable_declaration((const VariableDeclaration *)n->data); break;
        case AST_VariableDeclarator: print_variable_declarator((const VariableDeclarator *)n->data); break;
        case AST_Identifier: print_identifier((const Identifier *)n->data); break;
        case AST_Literal: print_literal((const Literal *)n->data); break;
        case AST_ExpressionStatement: print_expression_statement((const ExpressionStatement *)n->data); break;
        case AST_UpdateExpression: print_update_expression((const UpdateExpression *)n->data); break;
        case AST_BinaryExpression: print_binary_expression((const BinaryExpression *)n->data); break;
        case AST_AssignmentExpression: print_assignment_expression((const AssignmentExpression *)n->data); break;
        case AST_UnaryExpression: print_unary_expression((const UnaryExpression *)n->data); break;
        case AST_ObjectExpression: print_object_expression((const ObjectExpression *)n->data); break;
        case AST_Property: print_property((const Property *)n->data); break;
        case AST_ArrayExpression: print_array_expression((const ArrayExpression *)n->data); break;
        case AST_MemberExpression: print_member_expression((const MemberExpression *)n->data); break;
        case AST_CallExpression: print_call_expression((const CallExpression *)n->data); break;
        case AST_FunctionDeclaration: print_function_body((const FunctionBody *)n->data); break;
        case AST_FunctionExpression: print_function_body((const FunctionBody *)n->data); break;
        case AST_BlockStatement: print_block_statement((const BlockStatement *)n->data); break;
        case AST_IfStatement: print_if_statement((const IfStatement *)n->data); break;
        case AST_WhileStatement: print_while_statement((const WhileStatement *)n->data); break;
        case AST_DoWhileStatement: print_do_while_statement((const DoWhileStatement *)n->data); break;
        case AST_ForStatement: print_for_statement((const ForStatement *)n->data); break;
        case AST_SwitchStatement: print_switch_statement((const SwitchStatement *)n->data); break;
        case AST_SwitchCase: print_switch_case((const SwitchCase *)n->data); break;
        case AST_TryStatement: print_try_statement((const TryStatement *)n->data); break;
        case AST_CatchClause: print_catch_clause((const CatchClause *)n->data); break;
        case AST_ThrowStatement: print_throw_statement((const ThrowStatement *)n->data); break;
        case AST_ReturnStatement: print_return_statement((const ReturnStatement *)n->data); break;
        case AST_BreakStatement: print_break_statement((const BreakStatement *)n->data); break;
        case AST_ContinueStatement: print_continue_statement((const ContinueStatement *)n->data); break;
        case AST_ImportDeclaration: print_import_declaration((const ImportDeclaration *)n->data); break;
        case AST_ImportSpecifier: print_import_specifier((const ImportSpecifier *)n->data); break;
        case AST_ImportDefaultSpecifier: print_import_default_specifier((const ImportDefaultSpecifier *)n->data); break;
        case AST_ImportNamespaceSpecifier: print_import_namespace_specifier((const ImportNamespaceSpecifier *)n->data); break;
        case AST_ExportNamedDeclaration: print_export_named_declaration((const ExportNamedDeclaration *)n->data); break;
        case AST_ExportDefaultDeclaration: print_export_default_declaration((const ExportDefaultDeclaration *)n->data); break;
        case AST_Error: print_error((const ErrorNode *)n->data); break;
        // Phase 2: Modern Features
        case AST_ArrowFunctionExpression: print_arrow_function_expression((const ArrowFunctionExpression *)n->data); break;
        case AST_TemplateLiteral: print_template_literal((const TemplateLiteral *)n->data); break;
        case AST_TemplateElement: print_template_element((const TemplateElement *)n->data); break;
        case AST_SpreadElement: print_spread_element((const SpreadElement *)n->data); break;
        case AST_RestElement: print_rest_element((const RestElement *)n->data); break;
        case AST_ForOfStatement: print_for_of_statement((const ForOfStatement *)n->data); break;
        case AST_ForInStatement: print_for_in_statement((const ForInStatement *)n->data); break;
        case AST_ClassDeclaration: print_class_declaration((const ClassDeclaration *)n->data); break;
        case AST_ClassExpression: print_class_expression((const ClassExpression *)n->data); break;
        case AST_MethodDefinition: print_method_definition((const MethodDefinition *)n->data); break;
        case AST_AwaitExpression:
        case AST_YieldExpression: printf("\"argument\":null"); break;
        case AST_Super:
        case AST_ThisExpression: break; // No additional fields
        case AST_ObjectPattern:
        case AST_ArrayPattern:
        case AST_AssignmentPattern: break; // TODO: Implement if needed
        default: break;
    }
    printf("}");
}

void ast_print_json(const AstNode *node) {
    print_node(node);
    printf("\n");
}

static void free_node(AstNode *n);

void ast_retain(AstNode *node) {
    if (!node) return;
    node->refcount++;
}

void ast_release(AstNode *node) {
    if (!node) return;
    if (--node->refcount > 0) return;
    free_node(node);
}

void ast_free(AstNode *node) {
    ast_release(node);
}

static AstNode *clone_node(const AstNode *node);

AstNode *ast_clone(const AstNode *node) {
    return clone_node(node);
}

// --- implementation helpers ---

static AstNode *clone_node(const AstNode *n) {
    if (!n) return NULL;
    AstNode *c = new_node(n->type);
    if (!c) return NULL;
    c->start = n->start;
    c->end = n->end;
    switch (n->type) {
        case AST_Program: {
            Program *orig = (Program *)n->data;
            Program *cp = (Program *)calloc(1, sizeof(Program));
            astvec_init(&cp->body);
            for (size_t i = 0; orig && i < orig->body.count; ++i) {
                astvec_push(&cp->body, clone_node(orig->body.items[i]));
            }
            if (orig) {
                for (size_t i = 0; i < orig->comment_count; ++i) {
                    Comment *cc = comment_clone(orig->comments[i]);
                    if (cc) commentvec_push(cp, cc);
                }
            }
            c->data = cp;
            break;
        }
        case AST_VariableDeclaration: {
            VariableDeclaration *vd = (VariableDeclaration *)n->data;
            VariableDeclaration *cvd = (VariableDeclaration *)calloc(1, sizeof(VariableDeclaration));
            if (vd) {
                cvd->kind = vd->kind;
                astvec_init(&cvd->declarations);
                for (size_t i = 0; i < vd->declarations.count; ++i) {
                    astvec_push(&cvd->declarations, clone_node(vd->declarations.items[i]));
                }
            }
            c->data = cvd;
            break;
        }
        case AST_VariableDeclarator: {
            VariableDeclarator *vd = (VariableDeclarator *)n->data;
            VariableDeclarator *cvd = (VariableDeclarator *)calloc(1, sizeof(VariableDeclarator));
            if (vd) {
                cvd->id = clone_node(vd->id);
                cvd->init = clone_node(vd->init);
            }
            c->data = cvd;
            break;
        }
        case AST_Identifier: {
            Identifier *id = (Identifier *)n->data;
            Identifier *cid = (Identifier *)calloc(1, sizeof(Identifier));
            if (id && id->name) cid->name = dupstr(id->name);
            c->data = cid;
            break;
        }
        case AST_Literal: {
            Literal *lit = (Literal *)n->data;
            Literal *clit = (Literal *)calloc(1, sizeof(Literal));
            if (lit) {
                clit->kind = lit->kind;
                clit->raw = dupstr(lit->raw);
            }
            c->data = clit;
            break;
        }
        case AST_ExpressionStatement: {
            ExpressionStatement *es = (ExpressionStatement *)n->data;
            ExpressionStatement *ces = (ExpressionStatement *)calloc(1, sizeof(ExpressionStatement));
            if (es) ces->expression = clone_node(es->expression);
            c->data = ces;
            break;
        }
        case AST_UpdateExpression: {
            UpdateExpression *ue = (UpdateExpression *)n->data;
            UpdateExpression *cue = (UpdateExpression *)calloc(1, sizeof(UpdateExpression));
            if (ue) {
                cue->prefix = ue->prefix;
                cue->operator = dupstr(ue->operator);
                cue->argument = clone_node(ue->argument);
            }
            c->data = cue;
            break;
        }
        case AST_BinaryExpression: {
            BinaryExpression *be = (BinaryExpression *)n->data;
            BinaryExpression *cbe = (BinaryExpression *)calloc(1, sizeof(BinaryExpression));
            if (be) {
                cbe->operator = dupstr(be->operator);
                cbe->left = clone_node(be->left);
                cbe->right = clone_node(be->right);
            }
            c->data = cbe;
            break;
        }
        case AST_AssignmentExpression: {
            AssignmentExpression *ae = (AssignmentExpression *)n->data;
            AssignmentExpression *cae = (AssignmentExpression *)calloc(1, sizeof(AssignmentExpression));
            if (ae) {
                cae->operator = dupstr(ae->operator);
                cae->left = clone_node(ae->left);
                cae->right = clone_node(ae->right);
            }
            c->data = cae;
            break;
        }
        case AST_UnaryExpression: {
            UnaryExpression *ue = (UnaryExpression *)n->data;
            UnaryExpression *cue = (UnaryExpression *)calloc(1, sizeof(UnaryExpression));
            if (ue) {
                cue->operator = dupstr(ue->operator);
                cue->prefix = ue->prefix;
                cue->argument = clone_node(ue->argument);
            }
            c->data = cue;
            break;
        }
        case AST_ObjectExpression: {
            ObjectExpression *oe = (ObjectExpression *)n->data;
            ObjectExpression *coe = (ObjectExpression *)calloc(1, sizeof(ObjectExpression));
            astvec_init(&coe->properties);
            if (oe) {
                for (size_t i = 0; i < oe->properties.count; ++i) {
                    astvec_push(&coe->properties, clone_node(oe->properties.items[i]));
                }
            }
            c->data = coe;
            break;
        }
        case AST_Property: {
            Property *prop = (Property *)n->data;
            Property *cprop = (Property *)calloc(1, sizeof(Property));
            if (prop) {
                cprop->computed = prop->computed;
                cprop->key = clone_node(prop->key);
                cprop->value = clone_node(prop->value);
            }
            c->data = cprop;
            break;
        }
        case AST_ArrayExpression: {
            ArrayExpression *ae = (ArrayExpression *)n->data;
            ArrayExpression *cae = (ArrayExpression *)calloc(1, sizeof(ArrayExpression));
            astvec_init(&cae->elements);
            if (ae) {
                for (size_t i = 0; i < ae->elements.count; ++i) {
                    astvec_push(&cae->elements, clone_node(ae->elements.items[i]));
                }
            }
            c->data = cae;
            break;
        }
        case AST_MemberExpression: {
            MemberExpression *me = (MemberExpression *)n->data;
            MemberExpression *cme = (MemberExpression *)calloc(1, sizeof(MemberExpression));
            if (me) {
                cme->computed = me->computed;
                cme->object = clone_node(me->object);
                cme->property = clone_node(me->property);
            }
            c->data = cme;
            break;
        }
        case AST_CallExpression: {
            CallExpression *ce = (CallExpression *)n->data;
            CallExpression *cce = (CallExpression *)calloc(1, sizeof(CallExpression));
            astvec_init(&cce->arguments);
            if (ce) {
                cce->callee = clone_node(ce->callee);
                for (size_t i = 0; i < ce->arguments.count; ++i) {
                    astvec_push(&cce->arguments, clone_node(ce->arguments.items[i]));
                }
            }
            c->data = cce;
            break;
        }
        case AST_FunctionDeclaration:
        case AST_FunctionExpression: {
            FunctionBody *fb = (FunctionBody *)n->data;
            FunctionBody *cfb = (FunctionBody *)calloc(1, sizeof(FunctionBody));
            if (fb) {
                cfb->name = dupstr(fb->name);
                astvec_init(&cfb->params);
                for (size_t i = 0; i < fb->params.count; ++i) {
                    astvec_push(&cfb->params, clone_node(fb->params.items[i]));
                }
                cfb->body = clone_node(fb->body);
            }
            c->data = cfb;
            break;
        }
        case AST_BlockStatement: {
            BlockStatement *bs = (BlockStatement *)n->data;
            BlockStatement *cbs = (BlockStatement *)calloc(1, sizeof(BlockStatement));
            astvec_init(&cbs->body);
            if (bs) {
                for (size_t i = 0; i < bs->body.count; ++i) {
                    astvec_push(&cbs->body, clone_node(bs->body.items[i]));
                }
            }
            c->data = cbs;
            break;
        }
        case AST_IfStatement: {
            IfStatement *is = (IfStatement *)n->data;
            IfStatement *cis = (IfStatement *)calloc(1, sizeof(IfStatement));
            if (is) {
                cis->test = clone_node(is->test);
                cis->consequent = clone_node(is->consequent);
                cis->alternate = clone_node(is->alternate);
            }
            c->data = cis;
            break;
        }
        case AST_WhileStatement: {
            WhileStatement *ws = (WhileStatement *)n->data;
            WhileStatement *cws = (WhileStatement *)calloc(1, sizeof(WhileStatement));
            if (ws) {
                cws->test = clone_node(ws->test);
                cws->body = clone_node(ws->body);
            }
            c->data = cws;
            break;
        }
        case AST_DoWhileStatement: {
            DoWhileStatement *dw = (DoWhileStatement *)n->data;
            DoWhileStatement *cdw = (DoWhileStatement *)calloc(1, sizeof(DoWhileStatement));
            if (dw) {
                cdw->body = clone_node(dw->body);
                cdw->test = clone_node(dw->test);
            }
            c->data = cdw;
            break;
        }
        case AST_ForStatement: {
            ForStatement *fs = (ForStatement *)n->data;
            ForStatement *cfs = (ForStatement *)calloc(1, sizeof(ForStatement));
            if (fs) {
                cfs->init = clone_node(fs->init);
                cfs->test = clone_node(fs->test);
                cfs->update = clone_node(fs->update);
                cfs->body = clone_node(fs->body);
            }
            c->data = cfs;
            break;
        }
        case AST_SwitchStatement: {
            SwitchStatement *ss = (SwitchStatement *)n->data;
            SwitchStatement *css = (SwitchStatement *)calloc(1, sizeof(SwitchStatement));
            astvec_init(&css->cases);
            if (ss) {
                css->discriminant = clone_node(ss->discriminant);
                for (size_t i = 0; i < ss->cases.count; ++i) {
                    astvec_push(&css->cases, clone_node(ss->cases.items[i]));
                }
            }
            c->data = css;
            break;
        }
        case AST_SwitchCase: {
            SwitchCase *sc = (SwitchCase *)n->data;
            SwitchCase *csc = (SwitchCase *)calloc(1, sizeof(SwitchCase));
            astvec_init(&csc->consequent);
            if (sc) {
                csc->test = clone_node(sc->test);
                for (size_t i = 0; i < sc->consequent.count; ++i) {
                    astvec_push(&csc->consequent, clone_node(sc->consequent.items[i]));
                }
            }
            c->data = csc;
            break;
        }
        case AST_TryStatement: {
            TryStatement *ts = (TryStatement *)n->data;
            TryStatement *cts = (TryStatement *)calloc(1, sizeof(TryStatement));
            astvec_init(&cts->handlers);
            if (ts) {
                cts->block = clone_node(ts->block);
                for (size_t i = 0; i < ts->handlers.count; ++i) {
                    astvec_push(&cts->handlers, clone_node(ts->handlers.items[i]));
                }
                cts->finalizer = clone_node(ts->finalizer);
            }
            c->data = cts;
            break;
        }
        case AST_CatchClause: {
            CatchClause *cc = (CatchClause *)n->data;
            CatchClause *ccc = (CatchClause *)calloc(1, sizeof(CatchClause));
            if (cc) {
                ccc->param = clone_node(cc->param);
                ccc->body = clone_node(cc->body);
            }
            c->data = ccc;
            break;
        }
        case AST_ThrowStatement: {
            ThrowStatement *ts = (ThrowStatement *)n->data;
            ThrowStatement *cts = (ThrowStatement *)calloc(1, sizeof(ThrowStatement));
            if (ts) cts->argument = clone_node(ts->argument);
            c->data = cts;
            break;
        }
        case AST_ReturnStatement: {
            ReturnStatement *rs = (ReturnStatement *)n->data;
            ReturnStatement *crs = (ReturnStatement *)calloc(1, sizeof(ReturnStatement));
            if (rs) crs->argument = clone_node(rs->argument);
            c->data = crs;
            break;
        }
        case AST_BreakStatement: {
            BreakStatement *bs = (BreakStatement *)n->data;
            BreakStatement *cbs = (BreakStatement *)calloc(1, sizeof(BreakStatement));
            if (bs && bs->label) cbs->label = dupstr(bs->label);
            c->data = cbs;
            break;
        }
        case AST_ContinueStatement: {
            ContinueStatement *cs = (ContinueStatement *)n->data;
            ContinueStatement *ccs = (ContinueStatement *)calloc(1, sizeof(ContinueStatement));
            if (cs && cs->label) ccs->label = dupstr(cs->label);
            c->data = ccs;
            break;
        }
        case AST_ImportDeclaration: {
            ImportDeclaration *id = (ImportDeclaration *)n->data;
            ImportDeclaration *cid = (ImportDeclaration *)calloc(1, sizeof(ImportDeclaration));
            astvec_init(&cid->specifiers);
            if (id) {
                cid->source = dupstr(id->source);
                for (size_t i = 0; i < id->specifiers.count; ++i) {
                    astvec_push(&cid->specifiers, clone_node(id->specifiers.items[i]));
                }
            }
            c->data = cid;
            break;
        }
        case AST_ImportSpecifier: {
            ImportSpecifier *is = (ImportSpecifier *)n->data;
            ImportSpecifier *cis = (ImportSpecifier *)calloc(1, sizeof(ImportSpecifier));
            if (is) {
                cis->imported = clone_node(is->imported);
                cis->local = clone_node(is->local);
            }
            c->data = cis;
            break;
        }
        case AST_ImportDefaultSpecifier: {
            ImportDefaultSpecifier *ids = (ImportDefaultSpecifier *)n->data;
            ImportDefaultSpecifier *cids = (ImportDefaultSpecifier *)calloc(1, sizeof(ImportDefaultSpecifier));
            if (ids) {
                cids->local = clone_node(ids->local);
            }
            c->data = cids;
            break;
        }
        case AST_ImportNamespaceSpecifier: {
            ImportNamespaceSpecifier *ins = (ImportNamespaceSpecifier *)n->data;
            ImportNamespaceSpecifier *cins = (ImportNamespaceSpecifier *)calloc(1, sizeof(ImportNamespaceSpecifier));
            if (ins) {
                cins->local = clone_node(ins->local);
            }
            c->data = cins;
            break;
        }
        case AST_ExportNamedDeclaration: {
            ExportNamedDeclaration *en = (ExportNamedDeclaration *)n->data;
            ExportNamedDeclaration *cen = (ExportNamedDeclaration *)calloc(1, sizeof(ExportNamedDeclaration));
            astvec_init(&cen->specifiers);
            if (en) {
                cen->source = dupstr(en->source);
                for (size_t i = 0; i < en->specifiers.count; ++i) {
                    astvec_push(&cen->specifiers, clone_node(en->specifiers.items[i]));
                }
                cen->declaration = clone_node(en->declaration);
            }
            c->data = cen;
            break;
        }
        case AST_ExportDefaultDeclaration: {
            ExportDefaultDeclaration *ed = (ExportDefaultDeclaration *)n->data;
            ExportDefaultDeclaration *ced = (ExportDefaultDeclaration *)calloc(1, sizeof(ExportDefaultDeclaration));
            if (ed) {
                ced->declaration = clone_node(ed->declaration);
                ced->expression = clone_node(ed->expression);
            }
            c->data = ced;
            break;
        }
        // Phase 2: Modern Features
        case AST_ArrowFunctionExpression: {
            ArrowFunctionExpression *afe = (ArrowFunctionExpression *)n->data;
            ArrowFunctionExpression *cafe = (ArrowFunctionExpression *)calloc(1, sizeof(ArrowFunctionExpression));
            astvec_init(&cafe->params);
            if (afe) {
                cafe->is_async = afe->is_async;
                for (size_t i = 0; i < afe->params.count; ++i) {
                    astvec_push(&cafe->params, clone_node(afe->params.items[i]));
                }
                cafe->body = clone_node(afe->body);
            }
            c->data = cafe;
            break;
        }
        case AST_TemplateLiteral: {
            TemplateLiteral *tl = (TemplateLiteral *)n->data;
            TemplateLiteral *ctl = (TemplateLiteral *)calloc(1, sizeof(TemplateLiteral));
            astvec_init(&ctl->quasis);
            astvec_init(&ctl->expressions);
            if (tl) {
                for (size_t i = 0; i < tl->quasis.count; ++i) {
                    astvec_push(&ctl->quasis, clone_node(tl->quasis.items[i]));
                }
                for (size_t i = 0; i < tl->expressions.count; ++i) {
                    astvec_push(&ctl->expressions, clone_node(tl->expressions.items[i]));
                }
            }
            c->data = ctl;
            break;
        }
        case AST_TemplateElement: {
            TemplateElement *te = (TemplateElement *)n->data;
            TemplateElement *cte = (TemplateElement *)calloc(1, sizeof(TemplateElement));
            if (te) {
                cte->value = dupstr(te->value);
                cte->tail = te->tail;
            }
            c->data = cte;
            break;
        }
        case AST_SpreadElement: {
            SpreadElement *se = (SpreadElement *)n->data;
            SpreadElement *cse = (SpreadElement *)calloc(1, sizeof(SpreadElement));
            if (se) cse->argument = clone_node(se->argument);
            c->data = cse;
            break;
        }
        case AST_ObjectPattern: {
            ObjectPattern *op = (ObjectPattern *)n->data;
            ObjectPattern *cop = (ObjectPattern *)calloc(1, sizeof(ObjectPattern));
            astvec_init(&cop->properties);
            if (op) {
                for (size_t i = 0; i < op->properties.count; ++i) {
                    astvec_push(&cop->properties, clone_node(op->properties.items[i]));
                }
            }
            c->data = cop;
            break;
        }
        case AST_ArrayPattern: {
            ArrayPattern *ap = (ArrayPattern *)n->data;
            ArrayPattern *cap = (ArrayPattern *)calloc(1, sizeof(ArrayPattern));
            astvec_init(&cap->elements);
            if (ap) {
                for (size_t i = 0; i < ap->elements.count; ++i) {
                    astvec_push(&cap->elements, clone_node(ap->elements.items[i]));
                }
            }
            c->data = cap;
            break;
        }
        case AST_AssignmentPattern: {
            AssignmentPattern *ap = (AssignmentPattern *)n->data;
            AssignmentPattern *cap = (AssignmentPattern *)calloc(1, sizeof(AssignmentPattern));
            if (ap) {
                cap->left = clone_node(ap->left);
                cap->right = clone_node(ap->right);
            }
            c->data = cap;
            break;
        }
        case AST_RestElement: {
            RestElement *re = (RestElement *)n->data;
            RestElement *cre = (RestElement *)calloc(1, sizeof(RestElement));
            if (re) cre->argument = clone_node(re->argument);
            c->data = cre;
            break;
        }
        case AST_ForOfStatement: {
            ForOfStatement *fos = (ForOfStatement *)n->data;
            ForOfStatement *cfos = (ForOfStatement *)calloc(1, sizeof(ForOfStatement));
            if (fos) {
                cfos->left = clone_node(fos->left);
                cfos->right = clone_node(fos->right);
                cfos->body = clone_node(fos->body);
            }
            c->data = cfos;
            break;
        }
        case AST_ForInStatement: {
            ForInStatement *fis = (ForInStatement *)n->data;
            ForInStatement *cfis = (ForInStatement *)calloc(1, sizeof(ForInStatement));
            if (fis) {
                cfis->left = clone_node(fis->left);
                cfis->right = clone_node(fis->right);
                cfis->body = clone_node(fis->body);
            }
            c->data = cfis;
            break;
        }
        case AST_ClassDeclaration: {
            ClassDeclaration *cd = (ClassDeclaration *)n->data;
            ClassDeclaration *ccd = (ClassDeclaration *)calloc(1, sizeof(ClassDeclaration));
            astvec_init(&ccd->body);
            if (cd) {
                ccd->id = clone_node(cd->id);
                ccd->superClass = clone_node(cd->superClass);
                for (size_t i = 0; i < cd->body.count; ++i) {
                    astvec_push(&ccd->body, clone_node(cd->body.items[i]));
                }
            }
            c->data = ccd;
            break;
        }
        case AST_ClassExpression: {
            ClassExpression *ce = (ClassExpression *)n->data;
            ClassExpression *cce = (ClassExpression *)calloc(1, sizeof(ClassExpression));
            astvec_init(&cce->body);
            if (ce) {
                cce->id = clone_node(ce->id);
                cce->superClass = clone_node(ce->superClass);
                for (size_t i = 0; i < ce->body.count; ++i) {
                    astvec_push(&cce->body, clone_node(ce->body.items[i]));
                }
            }
            c->data = cce;
            break;
        }
        case AST_MethodDefinition: {
            MethodDefinition *md = (MethodDefinition *)n->data;
            MethodDefinition *cmd = (MethodDefinition *)calloc(1, sizeof(MethodDefinition));
            if (md) {
                cmd->key = clone_node(md->key);
                cmd->value = clone_node(md->value);
                cmd->kind = dupstr(md->kind);
                cmd->is_static = md->is_static;
            }
            c->data = cmd;
            break;
        }
        case AST_AwaitExpression: {
            AwaitExpression *ae = (AwaitExpression *)n->data;
            AwaitExpression *cae = (AwaitExpression *)calloc(1, sizeof(AwaitExpression));
            if (ae) cae->argument = clone_node(ae->argument);
            c->data = cae;
            break;
        }
        case AST_YieldExpression: {
            YieldExpression *ye = (YieldExpression *)n->data;
            YieldExpression *cye = (YieldExpression *)calloc(1, sizeof(YieldExpression));
            if (ye) {
                cye->argument = clone_node(ye->argument);
                cye->delegate = ye->delegate;
            }
            c->data = cye;
            break;
        }
        case AST_Super: {
            Super *sup = (Super *)n->data;
            Super *csup = (Super *)calloc(1, sizeof(Super));
            if (sup) csup->unused = sup->unused;
            c->data = csup;
            break;
        }
        case AST_ThisExpression: {
            ThisExpression *te = (ThisExpression *)n->data;
            ThisExpression *cte = (ThisExpression *)calloc(1, sizeof(ThisExpression));
            if (te) cte->unused = te->unused;
            c->data = cte;
            break;
        }
        case AST_Error: {
            ErrorNode *er = (ErrorNode *)n->data;
            ErrorNode *cer = (ErrorNode *)calloc(1, sizeof(ErrorNode));
            if (er && er->message) cer->message = dupstr(er->message);
            c->data = cer;
            break;
        }
        default:
            break;
    }
    return c;
}

static void free_program(Program *p) {
    for (size_t i = 0; i < p->body.count; ++i) ast_release(p->body.items[i]);
    free(p->body.items);
    for (size_t i = 0; i < p->comment_count; ++i) {
        Comment *c = p->comments[i];
        if (c) { free(c->text); free(c); }
    }
    free(p->comments);
    free(p);
}

static void free_identifier(Identifier *id) { free(id->name); free(id); }
static void free_literal(Literal *lit) { free(lit->raw); free(lit); }
static void free_vardecl(VariableDeclaration *vd) {
    for (size_t i = 0; i < vd->declarations.count; ++i) ast_release(vd->declarations.items[i]);
    free(vd->declarations.items); free(vd);
}
static void free_vardeclarator(VariableDeclarator *vd) { ast_release(vd->id); ast_release(vd->init); free(vd); }
static void free_exprstmt(ExpressionStatement *es) { ast_release(es->expression); free(es); }
static void free_update(UpdateExpression *ue) { free(ue->operator); ast_release(ue->argument); free(ue); }
static void free_binary(BinaryExpression *be) { free(be->operator); ast_release(be->left); ast_release(be->right); free(be); }
static void free_assignment(AssignmentExpression *ae) { free(ae->operator); ast_release(ae->left); ast_release(ae->right); free(ae); }
static void free_unary(UnaryExpression *ue) { free(ue->operator); ast_release(ue->argument); free(ue); }
static void free_object_expr(ObjectExpression *obj) {
    for (size_t i = 0; i < obj->properties.count; ++i) ast_release(obj->properties.items[i]);
    free(obj->properties.items); free(obj);
}
static void free_property(Property *prop) { ast_release(prop->key); ast_release(prop->value); free(prop); }
static void free_array_expr(ArrayExpression *arr) {
    for (size_t i = 0; i < arr->elements.count; ++i) ast_release(arr->elements.items[i]);
    free(arr->elements.items); free(arr);
}
static void free_member_expr(MemberExpression *me) { ast_release(me->object); ast_release(me->property); free(me); }
static void free_call_expr(CallExpression *ce) {
    ast_release(ce->callee);
    for (size_t i = 0; i < ce->arguments.count; ++i) ast_release(ce->arguments.items[i]);
    free(ce->arguments.items); free(ce);
}
static void free_function_body(FunctionBody *fb) {
    free(fb->name);
    for (size_t i = 0; i < fb->params.count; ++i) ast_release(fb->params.items[i]);
    free(fb->params.items);
    ast_release(fb->body);
    free(fb);
}
static void free_block_stmt(BlockStatement *bs) {
    for (size_t i = 0; i < bs->body.count; ++i) ast_release(bs->body.items[i]);
    free(bs->body.items); free(bs);
}
static void free_if_stmt(IfStatement *is) { ast_release(is->test); ast_release(is->consequent); ast_release(is->alternate); free(is); }
static void free_while_stmt(WhileStatement *ws) { ast_release(ws->test); ast_release(ws->body); free(ws); }
static void free_do_while_stmt(DoWhileStatement *dws) { ast_release(dws->body); ast_release(dws->test); free(dws); }
static void free_for_stmt(ForStatement *fs) { ast_release(fs->init); ast_release(fs->test); ast_release(fs->update); ast_release(fs->body); free(fs); }
static void free_switch_stmt(SwitchStatement *ss) {
    ast_release(ss->discriminant);
    for (size_t i = 0; i < ss->cases.count; ++i) ast_release(ss->cases.items[i]);
    free(ss->cases.items); free(ss);
}
static void free_switch_case(SwitchCase *sc) {
    ast_release(sc->test);
    for (size_t i = 0; i < sc->consequent.count; ++i) ast_release(sc->consequent.items[i]);
    free(sc->consequent.items); free(sc);
}
static void free_try_stmt(TryStatement *ts) {
    ast_release(ts->block);
    for (size_t i = 0; i < ts->handlers.count; ++i) ast_release(ts->handlers.items[i]);
    free(ts->handlers.items);
    ast_release(ts->finalizer); free(ts);
}
static void free_catch_clause(CatchClause *cc) { ast_release(cc->param); ast_release(cc->body); free(cc); }
static void free_throw_stmt(ThrowStatement *ts) { ast_release(ts->argument); free(ts); }
static void free_return_stmt(ReturnStatement *rs) { ast_release(rs->argument); free(rs); }
static void free_break_stmt(BreakStatement *bs) { free(bs->label); free(bs); }
static void free_continue_stmt(ContinueStatement *cs) { free(cs->label); free(cs); }
static void free_import_decl(ImportDeclaration *id) {
    for (size_t i = 0; i < id->specifiers.count; ++i) ast_release(id->specifiers.items[i]);
    free(id->specifiers.items);
    free(id->source); free(id);
}
static void free_import_spec(ImportSpecifier *is) { ast_release(is->imported); ast_release(is->local); free(is); }
static void free_import_default_spec(ImportDefaultSpecifier *ids) { ast_release(ids->local); free(ids); }
static void free_import_namespace_spec(ImportNamespaceSpecifier *ins) { ast_release(ins->local); free(ins); }
static void free_export_named(ExportNamedDeclaration *end) {
    for (size_t i = 0; i < end->specifiers.count; ++i) ast_release(end->specifiers.items[i]);
    free(end->specifiers.items);
    free(end->source);
    ast_release(end->declaration); free(end);
}
static void free_export_default(ExportDefaultDeclaration *edd) { ast_release(edd->declaration); ast_release(edd->expression); free(edd); }
// Phase 2: Modern Features
static void free_arrow_func(ArrowFunctionExpression *afe) {
    for (size_t i = 0; i < afe->params.count; ++i) ast_release(afe->params.items[i]);
    free(afe->params.items);
    ast_release(afe->body); free(afe);
}
static void free_template_lit(TemplateLiteral *tl) {
    for (size_t i = 0; i < tl->quasis.count; ++i) ast_release(tl->quasis.items[i]);
    free(tl->quasis.items);
    for (size_t i = 0; i < tl->expressions.count; ++i) ast_release(tl->expressions.items[i]);
    free(tl->expressions.items);
    free(tl);
}
static void free_template_elem(TemplateElement *te) { free(te->value); free(te); }
static void free_spread_elem(SpreadElement *se) { ast_release(se->argument); free(se); }
static void free_object_pattern(ObjectPattern *op) {
    for (size_t i = 0; i < op->properties.count; ++i) ast_release(op->properties.items[i]);
    free(op->properties.items); free(op);
}
static void free_array_pattern(ArrayPattern *ap) {
    for (size_t i = 0; i < ap->elements.count; ++i) ast_release(ap->elements.items[i]);
    free(ap->elements.items); free(ap);
}
static void free_assign_pattern(AssignmentPattern *ap) { ast_release(ap->left); ast_release(ap->right); free(ap); }
static void free_rest_elem(RestElement *re) { ast_release(re->argument); free(re); }
static void free_for_of_stmt(ForOfStatement *fos) { ast_release(fos->left); ast_release(fos->right); ast_release(fos->body); free(fos); }
static void free_for_in_stmt(ForInStatement *fis) { ast_release(fis->left); ast_release(fis->right); ast_release(fis->body); free(fis); }
static void free_class_decl(ClassDeclaration *cd) {
    ast_release(cd->id); ast_release(cd->superClass);
    for (size_t i = 0; i < cd->body.count; ++i) ast_release(cd->body.items[i]);
    free(cd->body.items); free(cd);
}
static void free_class_expr(ClassExpression *ce) {
    ast_release(ce->id); ast_release(ce->superClass);
    for (size_t i = 0; i < ce->body.count; ++i) ast_release(ce->body.items[i]);
    free(ce->body.items); free(ce);
}
static void free_method_def(MethodDefinition *md) { ast_release(md->key); ast_release(md->value); free(md->kind); free(md); }
static void free_await_expr(AwaitExpression *ae) { ast_release(ae->argument); free(ae); }
static void free_yield_expr(YieldExpression *ye) { ast_release(ye->argument); free(ye); }
static void free_super(Super *sup) { free(sup); }
static void free_this_expr(ThisExpression *te) { free(te); }
static void free_error(ErrorNode *er) { /*free(er->message);*/ free(er); }

static void free_node(AstNode *n) {
    if (!n) return;
    switch (n->type) {
        case AST_Program: free_program((Program *)n->data); break;
        case AST_VariableDeclaration: free_vardecl((VariableDeclaration *)n->data); break;
        case AST_VariableDeclarator: free_vardeclarator((VariableDeclarator *)n->data); break;
        case AST_Identifier: free_identifier((Identifier *)n->data); break;
        case AST_Literal: free_literal((Literal *)n->data); break;
        case AST_ExpressionStatement: free_exprstmt((ExpressionStatement *)n->data); break;
        case AST_UpdateExpression: free_update((UpdateExpression *)n->data); break;
        case AST_BinaryExpression: free_binary((BinaryExpression *)n->data); break;
        case AST_AssignmentExpression: free_assignment((AssignmentExpression *)n->data); break;
            case AST_UnaryExpression: free_unary((UnaryExpression *)n->data); break;
            case AST_ObjectExpression: free_object_expr((ObjectExpression *)n->data); break;
            case AST_Property: free_property((Property *)n->data); break;
            case AST_ArrayExpression: free_array_expr((ArrayExpression *)n->data); break;
            case AST_MemberExpression: free_member_expr((MemberExpression *)n->data); break;
            case AST_CallExpression: free_call_expr((CallExpression *)n->data); break;
            case AST_FunctionDeclaration: free_function_body((FunctionBody *)n->data); break;
            case AST_FunctionExpression: free_function_body((FunctionBody *)n->data); break;
            case AST_BlockStatement: free_block_stmt((BlockStatement *)n->data); break;
            case AST_IfStatement: free_if_stmt((IfStatement *)n->data); break;
            case AST_WhileStatement: free_while_stmt((WhileStatement *)n->data); break;
            case AST_DoWhileStatement: free_do_while_stmt((DoWhileStatement *)n->data); break;
            case AST_ForStatement: free_for_stmt((ForStatement *)n->data); break;
            case AST_SwitchStatement: free_switch_stmt((SwitchStatement *)n->data); break;
            case AST_SwitchCase: free_switch_case((SwitchCase *)n->data); break;
            case AST_TryStatement: free_try_stmt((TryStatement *)n->data); break;
            case AST_CatchClause: free_catch_clause((CatchClause *)n->data); break;
            case AST_ThrowStatement: free_throw_stmt((ThrowStatement *)n->data); break;
            case AST_ReturnStatement: free_return_stmt((ReturnStatement *)n->data); break;
            case AST_BreakStatement: free_break_stmt((BreakStatement *)n->data); break;
            case AST_ContinueStatement: free_continue_stmt((ContinueStatement *)n->data); break;
            case AST_ImportDeclaration: free_import_decl((ImportDeclaration *)n->data); break;
            case AST_ImportSpecifier: free_import_spec((ImportSpecifier *)n->data); break;
            case AST_ImportDefaultSpecifier: free_import_default_spec((ImportDefaultSpecifier *)n->data); break;
            case AST_ImportNamespaceSpecifier: free_import_namespace_spec((ImportNamespaceSpecifier *)n->data); break;
            case AST_ExportNamedDeclaration: free_export_named((ExportNamedDeclaration *)n->data); break;
            case AST_ExportDefaultDeclaration: free_export_default((ExportDefaultDeclaration *)n->data); break;
            case AST_ArrowFunctionExpression: free_arrow_func((ArrowFunctionExpression *)n->data); break;
            case AST_TemplateLiteral: free_template_lit((TemplateLiteral *)n->data); break;
            case AST_TemplateElement: free_template_elem((TemplateElement *)n->data); break;
            case AST_SpreadElement: free_spread_elem((SpreadElement *)n->data); break;
            case AST_ObjectPattern: free_object_pattern((ObjectPattern *)n->data); break;
            case AST_ArrayPattern: free_array_pattern((ArrayPattern *)n->data); break;
            case AST_AssignmentPattern: free_assign_pattern((AssignmentPattern *)n->data); break;
            case AST_RestElement: free_rest_elem((RestElement *)n->data); break;
            case AST_ForOfStatement: free_for_of_stmt((ForOfStatement *)n->data); break;
            case AST_ForInStatement: free_for_in_stmt((ForInStatement *)n->data); break;
            case AST_ClassDeclaration: free_class_decl((ClassDeclaration *)n->data); break;
            case AST_ClassExpression: free_class_expr((ClassExpression *)n->data); break;
            case AST_MethodDefinition: free_method_def((MethodDefinition *)n->data); break;
            case AST_AwaitExpression: free_await_expr((AwaitExpression *)n->data); break;
            case AST_YieldExpression: free_yield_expr((YieldExpression *)n->data); break;
            case AST_Super: free_super((Super *)n->data); break;
            case AST_ThisExpression: free_this_expr((ThisExpression *)n->data); break;
        case AST_Error: free_error((ErrorNode *)n->data); break;
    }
    free(n);
}
