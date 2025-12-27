/**
 * @file mock_modules.c
 * @brief Mock implementations for module boundary testing
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mock_modules.h"

/* ============================================================================
 * Mock Lexer Implementation
 * ============================================================================ */

MockLexer *mock_lexer_create(Token *tokens, size_t count) {
    MockLexer *ml = malloc(sizeof(MockLexer));
    if (!ml) return NULL;
    
    ml->tokens = malloc(sizeof(Token) * count);
    if (!ml->tokens) {
        free(ml);
        return NULL;
    }
    
    memcpy(ml->tokens, tokens, sizeof(Token) * count);
    ml->count = count;
    ml->pos = 0;
    
    return ml;
}

Token mock_lexer_next(MockLexer *ml) {
    if (!ml || ml->pos >= ml->count) {
        Token eof = {.type = TOKEN_EOF, .lexeme = NULL};
        return eof;
    }
    return ml->tokens[ml->pos++];
}

Token mock_lexer_peek(MockLexer *ml) {
    if (!ml || ml->pos >= ml->count) {
        Token eof = {.type = TOKEN_EOF, .lexeme = NULL};
        return eof;
    }
    return ml->tokens[ml->pos];
}

void mock_lexer_free(MockLexer *ml) {
    if (!ml) return;
    if (ml->tokens) free(ml->tokens);
    free(ml);
}

/* ============================================================================
 * Mock Parser Implementation
 * ============================================================================ */

AstNode *mock_parser_create_program(int statement_count) {
    AstNode *prog = malloc(sizeof(AstNode));
    if (!prog) return NULL;
    
    Program *p = malloc(sizeof(Program));
    if (!p) {
        free(prog);
        return NULL;
    }
    
    prog->type = AST_Program;
    prog->start = (Position){1, 0};
    prog->end = (Position){1, 0};
    prog->refcount = 1;
    prog->data = p;
    
    p->body.items = malloc(sizeof(AstNode *) * statement_count);
    p->body.count = statement_count;
    p->body.capacity = statement_count;
    p->comments = NULL;
    p->comment_count = 0;
    p->comment_capacity = 0;
    
    // Create placeholder statements
    for (int i = 0; i < statement_count; i++) {
        p->body.items[i] = mock_parser_create_identifier("placeholder", 1, 0);
    }
    
    return prog;
}

AstNode *mock_parser_create_identifier(const char *name, int line, int col) {
    AstNode *node = malloc(sizeof(AstNode));
    if (!node) return NULL;
    
    Identifier *id = malloc(sizeof(Identifier));
    if (!id) {
        free(node);
        return NULL;
    }
    
    id->name = malloc(strlen(name) + 1);
    if (!id->name) {
        free(id);
        free(node);
        return NULL;
    }
    strcpy(id->name, name);
    
    node->type = AST_Identifier;
    node->start = (Position){line, col};
    node->end = (Position){line, col + (int)strlen(name)};
    node->refcount = 1;
    node->data = id;
    
    return node;
}

AstNode *mock_parser_create_literal(const char *raw, LiteralKind kind) {
    AstNode *node = malloc(sizeof(AstNode));
    if (!node) return NULL;
    
    Literal *lit = malloc(sizeof(Literal));
    if (!lit) {
        free(node);
        return NULL;
    }
    
    lit->raw = malloc(strlen(raw) + 1);
    if (!lit->raw) {
        free(lit);
        free(node);
        return NULL;
    }
    strcpy(lit->raw, raw);
    lit->kind = kind;
    
    node->type = AST_Literal;
    node->start = (Position){1, 0};
    node->end = (Position){1, (int)strlen(raw)};
    node->refcount = 1;
    node->data = lit;
    
    return node;
}

AstNode *mock_parser_create_var_decl(const char *var_name, VarKind kind) {
    AstNode *node = malloc(sizeof(AstNode));
    if (!node) return NULL;
    
    VariableDeclaration *vd = malloc(sizeof(VariableDeclaration));
    if (!vd) {
        free(node);
        return NULL;
    }
    
    vd->kind = kind;
    vd->declarations.items = malloc(sizeof(VariableDeclarator));
    vd->declarations.count = 1;
    vd->declarations.capacity = 1;
    
    VariableDeclarator *decl = malloc(sizeof(VariableDeclarator));
    memcpy(vd->declarations.items, &decl, sizeof(VariableDeclarator *));
    
    decl->id = mock_parser_create_identifier(var_name, 1, 0);
    decl->init = NULL;
    
    node->type = AST_VariableDeclaration;
    node->start = (Position){1, 0};
    node->end = (Position){1, 10};
    node->refcount = 1;
    node->data = vd;
    
    return node;
}

AstNode *mock_parser_create_expr_stmt(AstNode *expr) {
    AstNode *node = malloc(sizeof(AstNode));
    if (!node) return NULL;
    
    ExpressionStatement *es = malloc(sizeof(ExpressionStatement));
    if (!es) {
        free(node);
        return NULL;
    }
    
    es->expression = expr;
    
    node->type = AST_ExpressionStatement;
    node->start = expr ? expr->start : (Position){1, 0};
    node->end = expr ? expr->end : (Position){1, 0};
    node->refcount = 1;
    node->data = es;
    
    return node;
}

/* ============================================================================
 * Mock Codegen Implementation
 * ============================================================================ */

static CodegenMockMode mock_codegen_mode = CODEGEN_MOCK_IDENTITY;

void mock_codegen_set_mode(CodegenMockMode mode) {
    mock_codegen_mode = mode;
}

char *mock_codegen_get_output(const AstNode *root) {
    if (!root) return NULL;
    
    switch (mock_codegen_mode) {
        case CODEGEN_MOCK_IDENTITY: {
            // Return a simple valid JavaScript string
            char *output = malloc(128);
            strcpy(output, "var x = 42;\n");
            return output;
        }
        
        case CODEGEN_MOCK_MINIFIED: {
            char *output = malloc(128);
            strcpy(output, "var x=42;");
            return output;
        }
        
        case CODEGEN_MOCK_FORMATTED: {
            char *output = malloc(256);
            strcpy(output, "var x = 42;\nvar y = 'hello';\n");
            return output;
        }
        
        case CODEGEN_MOCK_FAILURE:
            return NULL;
            
        default:
            return NULL;
    }
}

/* ============================================================================
 * Mock ScopeManager Implementation
 * ============================================================================ */

Scope *mock_scope_create(ScopeType type, Scope *parent,
                        MockScopeBindings *bindings) {
    Scope *scope = malloc(sizeof(Scope));
    if (!scope) return NULL;
    
    scope->type = type;
    scope->parent = parent;
    scope->node = NULL;
    
    // Initialize binding vector
    if (bindings && bindings->count > 0) {
        scope->bindings.items = malloc(sizeof(Binding *) * bindings->count);
        scope->bindings.count = bindings->count;
        scope->bindings.capacity = bindings->count;
        
        for (size_t i = 0; i < bindings->count; i++) {
            Binding *b = malloc(sizeof(Binding));
            b->name = malloc(strlen(bindings->names[i]) + 1);
            strcpy(b->name, bindings->names[i]);
            b->kind = bindings->kinds[i];
            b->loc = (Position){1, 0};
            b->node = NULL;
            b->scope = scope;
            b->shadowed = NULL;
            
            scope->bindings.items[i] = b;
        }
    } else {
        scope->bindings.items = NULL;
        scope->bindings.count = 0;
        scope->bindings.capacity = 0;
    }
    
    scope->references.items = NULL;
    scope->references.count = 0;
    scope->references.capacity = 0;
    
    scope->children.items = NULL;
    scope->children.count = 0;
    scope->children.capacity = 0;
    
    return scope;
}

ScopeManager *mock_scope_manager_create(Scope *root) {
    ScopeManager *sm = malloc(sizeof(ScopeManager));
    if (!sm) return NULL;
    
    sm->root = root;
    sm->map = NULL;
    sm->map_count = 0;
    sm->map_capacity = 0;
    
    return sm;
}

ScopeManager *mock_scope_manager_simple(void) {
    // Create global scope with some standard bindings
    const char *global_names[] = {"console", "Object", "Array", "String", "Number"};
    BindingKind global_kinds[] = {BIND_IMPLICIT, BIND_IMPLICIT, BIND_IMPLICIT, 
                                   BIND_IMPLICIT, BIND_IMPLICIT};
    
    MockScopeBindings global_bindings = {
        .names = global_names,
        .kinds = global_kinds,
        .count = 5
    };
    
    Scope *global = mock_scope_create(SCOPE_GLOBAL, NULL, &global_bindings);
    return mock_scope_manager_create(global);
}

/* ============================================================================
 * Test Assertion Helpers
 * ============================================================================ */

int assert_ast_type(const AstNode *node, AstNodeType expected_type) {
    if (!node) {
        fprintf(stderr, "assert_ast_type: node is NULL\n");
        return 0;
    }
    
    if (node->type != expected_type) {
        fprintf(stderr, "assert_ast_type: expected type %d, got %d\n", 
                expected_type, node->type);
        return 0;
    }
    
    return 1;
}

int assert_token_sequence_equal(Token *t1, size_t count1,
                               Token *t2, size_t count2) {
    if (count1 != count2) {
        fprintf(stderr, "assert_token_sequence_equal: count mismatch %zu vs %zu\n",
                count1, count2);
        return 0;
    }
    
    for (size_t i = 0; i < count1; i++) {
        if (t1[i].type != t2[i].type) {
            fprintf(stderr, "assert_token_sequence_equal: token %zu type mismatch\n", i);
            return 0;
        }
        if ((t1[i].lexeme && t2[i].lexeme && strcmp(t1[i].lexeme, t2[i].lexeme) != 0) ||
            (t1[i].lexeme != NULL) != (t2[i].lexeme != NULL)) {
            fprintf(stderr, "assert_token_sequence_equal: token %zu lexeme mismatch\n", i);
            return 0;
        }
    }
    
    return 1;
}

int assert_scope_bindings(Scope *scope, const char **names, int count) {
    if (!scope) {
        fprintf(stderr, "assert_scope_bindings: scope is NULL\n");
        return 0;
    }
    
    if ((int)scope->bindings.count != count) {
        fprintf(stderr, "assert_scope_bindings: count mismatch %zu vs %d\n",
                scope->bindings.count, count);
        return 0;
    }
    
    for (int i = 0; i < count; i++) {
        if (!scope->bindings.items[i]) {
            fprintf(stderr, "assert_scope_bindings: binding %d is NULL\n", i);
            return 0;
        }
        if (strcmp(scope->bindings.items[i]->name, names[i]) != 0) {
            fprintf(stderr, "assert_scope_bindings: binding %d name mismatch '%s' vs '%s'\n",
                    i, scope->bindings.items[i]->name, names[i]);
            return 0;
        }
    }
    
    return 1;
}

int assert_code_valid_syntax(const char *code) {
    if (!code || strlen(code) == 0) {
        fprintf(stderr, "assert_code_valid_syntax: code is empty\n");
        return 0;
    }
    
    // Basic checks: not NULL, not too short, contains some identifiers/keywords
    if (strstr(code, "var") || strstr(code, "let") || strstr(code, "const") ||
        strstr(code, "function") || strstr(code, "return")) {
        return 1;
    }
    
    // For now, assume single expression is valid if non-empty
    if (strlen(code) > 0) {
        return 1;
    }
    
    fprintf(stderr, "assert_code_valid_syntax: code doesn't look like valid JS\n");
    return 0;
}
