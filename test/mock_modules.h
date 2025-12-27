/**
 * @file mock_modules.h
 * @brief Mock implementations for testing module boundaries in isolation
 * 
 * Provides stub implementations of parser, codegen, and scope analysis
 * for testing individual components without full dependencies.
 */

#ifndef MOCK_MODULES_H
#define MOCK_MODULES_H

#include "quickjsflow/lexer.h"
#include "quickjsflow/parser.h"
#include "quickjsflow/ast.h"
#include "quickjsflow/scope.h"
#include "quickjsflow/codegen.h"

/* ============================================================================
 * Mock Lexer - Returns predefined token sequences
 * ============================================================================ */

typedef struct {
    Token *tokens;
    size_t count;
    size_t pos;
} MockLexer;

/**
 * Create mock lexer with predefined tokens
 */
MockLexer *mock_lexer_create(Token *tokens, size_t count);

Token mock_lexer_next(MockLexer *ml);
Token mock_lexer_peek(MockLexer *ml);
void mock_lexer_free(MockLexer *ml);

/**
 * Create a simple token for testing
 */
static inline Token token_create(TokenType type, const char *lexeme, 
                                 int line, int col) {
    Token t = {
        .type = type,
        .start_line = line,
        .start_col = col,
        .end_line = line,
        .end_col = col + (lexeme ? strlen(lexeme) : 0),
        .lexeme = (char *)lexeme,
        .error = 0,
        .error_kind = NULL
    };
    return t;
}

/* ============================================================================
 * Mock Parser - Returns synthetic AST structures
 * ============================================================================ */

/**
 * Create a simple Program AST for testing
 * @param statement_count: Number of statements to include
 * @return: Program node
 */
AstNode *mock_parser_create_program(int statement_count);

/**
 * Create a VariableDeclaration node
 */
AstNode *mock_parser_create_var_decl(const char *var_name, VarKind kind);

/**
 * Create an Identifier node
 */
AstNode *mock_parser_create_identifier(const char *name, int line, int col);

/**
 * Create a Literal node
 */
AstNode *mock_parser_create_literal(const char *raw, LiteralKind kind);

/**
 * Create ExpressionStatement wrapping an expression
 */
AstNode *mock_parser_create_expr_stmt(AstNode *expr);

/* ============================================================================
 * Mock Codegen - Returns fixed output for deterministic testing
 * ============================================================================ */

typedef enum {
    CODEGEN_MOCK_IDENTITY,    // Output equals input (for testing parsing)
    CODEGEN_MOCK_MINIFIED,    // Output is minified version
    CODEGEN_MOCK_FORMATTED,   // Output is prettified version
    CODEGEN_MOCK_FAILURE      // Simulate codegen failure
} CodegenMockMode;

/**
 * Set mock codegen behavior
 */
void mock_codegen_set_mode(CodegenMockMode mode);

/**
 * Get mock code for testing (must be freed by caller)
 */
char *mock_codegen_get_output(const AstNode *root);

/* ============================================================================
 * Mock ScopeManager - Returns predefined scope information
 * ============================================================================ */

typedef struct {
    const char **names;
    BindingKind *kinds;
    size_t count;
} MockScopeBindings;

/**
 * Create mock scope with predefined bindings
 */
Scope *mock_scope_create(ScopeType type, Scope *parent,
                         MockScopeBindings *bindings);

/**
 * Create mock scope manager with predefined scope tree
 */
ScopeManager *mock_scope_manager_create(Scope *root);

/**
 * Setup a simple global scope for testing
 */
ScopeManager *mock_scope_manager_simple(void);

/* ============================================================================
 * Test Assertion Helpers for Module Boundaries
 * ============================================================================ */

/**
 * Assert that AST node has expected type
 */
int assert_ast_type(const AstNode *node, AstNodeType expected_type);

/**
 * Assert that two token sequences are identical
 */
int assert_token_sequence_equal(Token *t1, size_t count1,
                               Token *t2, size_t count2);

/**
 * Assert that scope has expected bindings
 */
int assert_scope_bindings(Scope *scope, const char **names, int count);

/**
 * Assert that generated code compiles (basic check)
 */
int assert_code_valid_syntax(const char *code);

#endif // MOCK_MODULES_H
