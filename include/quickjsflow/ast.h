#ifndef QUICKJSFLOW_AST_H
#define QUICKJSFLOW_AST_H

#include <stddef.h>

typedef enum {
    AST_Program = 1,
    AST_VariableDeclaration,
    AST_VariableDeclarator,
    AST_Identifier,
    AST_Literal,
    AST_ExpressionStatement,
    AST_UpdateExpression,
    AST_BinaryExpression,
    AST_Error
} AstNodeType;

typedef struct {
    int line;
    int column;
} Position;

typedef struct AstNode AstNode;

struct AstNode {
    AstNodeType type;
    Position start;
    Position end;
    void *data; // type-specific payload
};

typedef struct {
    AstNode **items;
    size_t count;
    size_t capacity;
} AstVec;

typedef struct {
    AstVec body; // statements
} Program;

typedef enum { VD_Var = 1, VD_Let, VD_Const } VarKind;

typedef struct {
    AstNode *id;      // Identifier
    AstNode *init;    // Expression or NULL
} VariableDeclarator;

typedef struct {
    VarKind kind;
    AstVec declarations; // VariableDeclarator*
} VariableDeclaration;

typedef struct {
    char *name;
} Identifier;

typedef enum { LIT_Number = 1, LIT_String } LiteralKind;

typedef struct {
    LiteralKind kind;
    char *raw; // raw lexeme
} Literal;

typedef struct {
    AstNode *expression;
} ExpressionStatement;

typedef struct {
    int prefix;
    char *operator; // "++" or "--"
    AstNode *argument; // Identifier
} UpdateExpression;

typedef struct {
    char *operator; // "+" etc (limited for MVP)
    AstNode *left;
    AstNode *right;
} BinaryExpression;

typedef struct {
    char *message;
} ErrorNode;

// vector helpers
void astvec_init(AstVec *v);
void astvec_push(AstVec *v, AstNode *n);

// constructors
AstNode *ast_program(void);
AstNode *ast_identifier(const char *name, Position s, Position e);
AstNode *ast_literal(LiteralKind kind, const char *raw, Position s, Position e);
AstNode *ast_variable_declaration(VarKind kind);
AstNode *ast_variable_declarator(AstNode *id, AstNode *init);
AstNode *ast_expression_statement(AstNode *expr, Position s, Position e);
AstNode *ast_update_expression(const char *op, int prefix, AstNode *arg, Position s, Position e);
AstNode *ast_binary_expression(const char *op, AstNode *left, AstNode *right, Position s, Position e);
AstNode *ast_error(const char *msg, Position s, Position e);

// JSON printer
void ast_print_json(const AstNode *node);
void ast_free(AstNode *node);

#endif
