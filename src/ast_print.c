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
        default: break;
    }
    free(n);
}
