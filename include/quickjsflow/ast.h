#ifndef QUICKJSFLOW_AST_H
#define QUICKJSFLOW_AST_H

#include <stddef.h>

typedef enum {
    // Phase 1: Essential Features
    AST_Program = 1,
    AST_VariableDeclaration,
    AST_VariableDeclarator,
    AST_Identifier,
    AST_Literal,
    AST_ExpressionStatement,
    AST_UpdateExpression,
    AST_BinaryExpression,
    AST_AssignmentExpression,
    AST_UnaryExpression,
    AST_ObjectExpression,
    AST_Property,
    AST_ArrayExpression,
    AST_MemberExpression,
    AST_CallExpression,
    AST_FunctionDeclaration,
    AST_FunctionExpression,
    AST_BlockStatement,
    AST_IfStatement,
    AST_WhileStatement,
    AST_DoWhileStatement,
    AST_ForStatement,
    AST_SwitchStatement,
    AST_SwitchCase,
    AST_TryStatement,
    AST_CatchClause,
    AST_ThrowStatement,
    AST_ReturnStatement,
    AST_BreakStatement,
    AST_ContinueStatement,
    AST_ImportDeclaration,
    AST_ImportSpecifier,
    AST_ExportNamedDeclaration,
    AST_ExportDefaultDeclaration,
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
    int refcount; // reference count for structural sharing
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
    char *operator; // "=" or "+=" etc
    AstNode *left;
    AstNode *right;
} AssignmentExpression;

typedef struct {
    char *operator; // "-", "!", "typeof", etc.
    int prefix;      // 1 if prefix, 0 if postfix (rare)
    AstNode *argument;
} UnaryExpression;

typedef struct {
    AstNode *key;    // Identifier or Literal
    AstNode *value;  // Expression
    int computed;    // 0 for property.key, 1 for property[key]
} Property;

typedef struct {
    AstVec properties; // Property*
} ObjectExpression;

typedef struct {
    AstVec elements; // Expression* (can be NULL for holes)
} ArrayExpression;

typedef struct {
    AstNode *object;    // Expression
    AstNode *property;  // Identifier or Expression
    int computed;       // 0 for obj.prop, 1 for obj[prop]
} MemberExpression;

typedef struct {
    AstNode *callee;    // Expression
    AstVec arguments;   // Expression*
} CallExpression;

typedef struct {
    AstVec params;      // Identifier* or Pattern*
    AstNode *body;      // BlockStatement
    char *name;         // for FunctionDeclaration/Expression
} FunctionBody;

typedef struct {
    AstVec body; // statements
} BlockStatement;

typedef struct {
    AstNode *test;       // Expression
    AstNode *consequent; // Statement
    AstNode *alternate;  // Statement or NULL
} IfStatement;

typedef struct {
    AstNode *test;   // Expression
    AstNode *body;   // Statement
} WhileStatement;

typedef struct {
    AstNode *body; // Statement
    AstNode *test; // Expression
} DoWhileStatement;

typedef struct {
    AstNode *init;       // Expression or VariableDeclaration or NULL
    AstNode *test;       // Expression or NULL
    AstNode *update;     // Expression or NULL
    AstNode *body;       // Statement
} ForStatement;

typedef struct {
    AstNode *discriminant; // Expression
    AstVec cases;          // SwitchCase*
} SwitchStatement;

typedef struct {
    AstNode *test;       // Expression or NULL (default case)
    AstVec consequent;   // Statement*
} SwitchCase;

typedef struct {
    AstNode *block;        // BlockStatement
    AstVec handlers;       // CatchClause*
    AstNode *finalizer;    // BlockStatement or NULL
} TryStatement;

typedef struct {
    AstNode *param;  // Identifier or Pattern
    AstNode *body;   // BlockStatement
} CatchClause;

typedef struct {
    AstNode *argument; // Expression
} ThrowStatement;

typedef struct {
    AstNode *argument; // Expression or NULL
} ReturnStatement;

typedef struct {
    char *label; // NULL for now
} BreakStatement;

typedef struct {
    char *label; // NULL for now
} ContinueStatement;

typedef struct {
    AstVec specifiers; // ImportSpecifier*
    char *source;      // module path
} ImportDeclaration;

typedef struct {
    AstNode *imported;  // Identifier
    AstNode *local;     // Identifier
} ImportSpecifier;

typedef struct {
    AstVec specifiers;   // Identifier* (exported names)
    char *source;        // module path or NULL
    AstNode *declaration; // VariableDeclaration, FunctionDeclaration, etc.
} ExportNamedDeclaration;

typedef struct {
    AstNode *declaration; // FunctionDeclaration, ClassDeclaration, etc.
    AstNode *expression;  // Expression
} ExportDefaultDeclaration;

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
AstNode *ast_assignment_expression(const char *op, AstNode *left, AstNode *right, Position s, Position e);
AstNode *ast_unary_expression(const char *op, int prefix, AstNode *arg, Position s, Position e);
AstNode *ast_object_expression(Position s, Position e);
AstNode *ast_property(AstNode *key, AstNode *value, int computed);
AstNode *ast_array_expression(Position s, Position e);
AstNode *ast_member_expression(AstNode *obj, AstNode *prop, int computed, Position s, Position e);
AstNode *ast_call_expression(AstNode *callee, Position s, Position e);
AstNode *ast_function_declaration(const char *name, Position s, Position e);
AstNode *ast_function_expression(const char *name, Position s, Position e);
AstNode *ast_block_statement(Position s, Position e);
AstNode *ast_if_statement(AstNode *test, AstNode *cons, AstNode *alt, Position s, Position e);
AstNode *ast_while_statement(AstNode *test, AstNode *body, Position s, Position e);
AstNode *ast_do_while_statement(AstNode *body, AstNode *test, Position s, Position e);
AstNode *ast_for_statement(AstNode *init, AstNode *test, AstNode *update, AstNode *body, Position s, Position e);
AstNode *ast_switch_statement(AstNode *discriminant, Position s, Position e);
AstNode *ast_switch_case(AstNode *test);
AstNode *ast_try_statement(AstNode *block, Position s, Position e);
AstNode *ast_catch_clause(AstNode *param, AstNode *body);
AstNode *ast_throw_statement(AstNode *argument, Position s, Position e);
AstNode *ast_return_statement(AstNode *argument, Position s, Position e);
AstNode *ast_break_statement(Position s, Position e);
AstNode *ast_continue_statement(Position s, Position e);
AstNode *ast_import_declaration(const char *source, Position s, Position e);
AstNode *ast_import_specifier(AstNode *imported, AstNode *local);
AstNode *ast_export_named_declaration(const char *source, Position s, Position e);
AstNode *ast_export_default_declaration(Position s, Position e);
AstNode *ast_error(const char *msg, Position s, Position e);

// JSON printer
void ast_print_json(const AstNode *node);
void ast_free(AstNode *node);
void ast_retain(AstNode *node);
void ast_release(AstNode *node);
AstNode *ast_clone(const AstNode *node);

#endif
