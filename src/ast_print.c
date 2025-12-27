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
        if (*p == '"' || *p == '\\') putchar('\\');
        putchar(*p);
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

static AstNode *new_node(AstNodeType t) {
    AstNode *n = (AstNode *)calloc(1, sizeof(AstNode));
    if (n) n->type = t;
    return n;
}

AstNode *ast_program(void) {
    AstNode *n = new_node(AST_Program);
    if (!n) return NULL;
    Program *p = (Program *)calloc(1, sizeof(Program));
    astvec_init(&p->body);
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
        case AST_ExportNamedDeclaration: type = "ExportNamedDeclaration"; break;
        case AST_ExportDefaultDeclaration: type = "ExportDefaultDeclaration"; break;
        case AST_Error: type = "Error"; break;
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
        case AST_ExportNamedDeclaration: print_export_named_declaration((const ExportNamedDeclaration *)n->data); break;
        case AST_ExportDefaultDeclaration: print_export_default_declaration((const ExportDefaultDeclaration *)n->data); break;
        case AST_Error: print_error((const ErrorNode *)n->data); break;
        default: break;
    }
    printf("}");
}

void ast_print_json(const AstNode *node) {
    print_node(node);
    printf("\n");
}

static void free_node(AstNode *n);

void ast_free(AstNode *n) {
    free_node(n);
}

static void free_program(Program *p) {
    for (size_t i = 0; i < p->body.count; ++i) free_node(p->body.items[i]);
    free(p->body.items);
    free(p);
}

static void free_identifier(Identifier *id) { free(id->name); free(id); }
static void free_literal(Literal *lit) { free(lit->raw); free(lit); }
static void free_vardecl(VariableDeclaration *vd) {
    for (size_t i = 0; i < vd->declarations.count; ++i) free_node(vd->declarations.items[i]);
    free(vd->declarations.items); free(vd);
}
static void free_vardeclarator(VariableDeclarator *vd) { free_node(vd->id); free_node(vd->init); free(vd); }
static void free_exprstmt(ExpressionStatement *es) { free_node(es->expression); free(es); }
static void free_update(UpdateExpression *ue) { free(ue->operator); free_node(ue->argument); free(ue); }
static void free_binary(BinaryExpression *be) { free(be->operator); free_node(be->left); free_node(be->right); free(be); }
static void free_assignment(AssignmentExpression *ae) { free(ae->operator); free_node(ae->left); free_node(ae->right); free(ae); }
static void free_unary(UnaryExpression *ue) { free(ue->operator); free_node(ue->argument); free(ue); }
static void free_object_expr(ObjectExpression *obj) {
    for (size_t i = 0; i < obj->properties.count; ++i) free_node(obj->properties.items[i]);
    free(obj->properties.items); free(obj);
}
static void free_property(Property *prop) { free_node(prop->key); free_node(prop->value); free(prop); }
static void free_array_expr(ArrayExpression *arr) {
    for (size_t i = 0; i < arr->elements.count; ++i) free_node(arr->elements.items[i]);
    free(arr->elements.items); free(arr);
}
static void free_member_expr(MemberExpression *me) { free_node(me->object); free_node(me->property); free(me); }
static void free_call_expr(CallExpression *ce) {
    free_node(ce->callee);
    for (size_t i = 0; i < ce->arguments.count; ++i) free_node(ce->arguments.items[i]);
    free(ce->arguments.items); free(ce);
}
static void free_function_body(FunctionBody *fb) {
    free(fb->name);
    for (size_t i = 0; i < fb->params.count; ++i) free_node(fb->params.items[i]);
    free(fb->params.items);
    free_node(fb->body);
    free(fb);
}
static void free_block_stmt(BlockStatement *bs) {
    for (size_t i = 0; i < bs->body.count; ++i) free_node(bs->body.items[i]);
    free(bs->body.items); free(bs);
}
static void free_if_stmt(IfStatement *is) { free_node(is->test); free_node(is->consequent); free_node(is->alternate); free(is); }
static void free_while_stmt(WhileStatement *ws) { free_node(ws->test); free_node(ws->body); free(ws); }
static void free_do_while_stmt(DoWhileStatement *dws) { free_node(dws->body); free_node(dws->test); free(dws); }
static void free_for_stmt(ForStatement *fs) { free_node(fs->init); free_node(fs->test); free_node(fs->update); free_node(fs->body); free(fs); }
static void free_switch_stmt(SwitchStatement *ss) {
    free_node(ss->discriminant);
    for (size_t i = 0; i < ss->cases.count; ++i) free_node(ss->cases.items[i]);
    free(ss->cases.items); free(ss);
}
static void free_switch_case(SwitchCase *sc) {
    free_node(sc->test);
    for (size_t i = 0; i < sc->consequent.count; ++i) free_node(sc->consequent.items[i]);
    free(sc->consequent.items); free(sc);
}
static void free_try_stmt(TryStatement *ts) {
    free_node(ts->block);
    for (size_t i = 0; i < ts->handlers.count; ++i) free_node(ts->handlers.items[i]);
    free(ts->handlers.items);
    free_node(ts->finalizer); free(ts);
}
static void free_catch_clause(CatchClause *cc) { free_node(cc->param); free_node(cc->body); free(cc); }
static void free_throw_stmt(ThrowStatement *ts) { free_node(ts->argument); free(ts); }
static void free_return_stmt(ReturnStatement *rs) { free_node(rs->argument); free(rs); }
static void free_break_stmt(BreakStatement *bs) { free(bs->label); free(bs); }
static void free_continue_stmt(ContinueStatement *cs) { free(cs->label); free(cs); }
static void free_import_decl(ImportDeclaration *id) {
    for (size_t i = 0; i < id->specifiers.count; ++i) free_node(id->specifiers.items[i]);
    free(id->specifiers.items);
    free(id->source); free(id);
}
static void free_import_spec(ImportSpecifier *is) { free_node(is->imported); free_node(is->local); free(is); }
static void free_export_named(ExportNamedDeclaration *end) {
    for (size_t i = 0; i < end->specifiers.count; ++i) free_node(end->specifiers.items[i]);
    free(end->specifiers.items);
    free(end->source);
    free_node(end->declaration); free(end);
}
static void free_export_default(ExportDefaultDeclaration *edd) { free_node(edd->declaration); free_node(edd->expression); free(edd); }
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
        case AST_Error: free_error((ErrorNode *)n->data); break;
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
            case AST_ExportNamedDeclaration: free_export_named((ExportNamedDeclaration *)n->data); break;
            case AST_ExportDefaultDeclaration: free_export_default((ExportDefaultDeclaration *)n->data); break;
        default: break;
    }
    free(n);
}
