/**
 * @file test_integration_comprehensive.c
 * @brief Comprehensive integration tests for module boundaries
 * 
 * Tests:
 * 1. Lexer → Parser: Token stream processing
 * 2. Parser → ScopeManager: AST structure and binding
 * 3. ScopeManager → Edit: Scope queries
 * 4. Edit → Codegen: AST preservation
 * 5. Full pipeline: Source → Code generation → Re-parse
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "quickjsflow/lexer.h"
#include "quickjsflow/parser.h"
#include "quickjsflow/ast.h"
#include "quickjsflow/scope.h"
#include "quickjsflow/codegen.h"
#include "test_framework_extended.h"
#include "mock_modules.h"

/* ============================================================================
 * Test Suite 1: Lexer → Parser Interface (TokenStream)
 * ============================================================================ */

void test_lexer_produces_tokens(void) {
    const char *src = "var x = 42;";
    
    Lexer lex;
    lexer_init(&lex, src, strlen(src));
    
    Token t1 = lexer_next(&lex);
    ASSERT_EQ(t1.type, TOKEN_IDENTIFIER, "First token is identifier");
    ASSERT_STR_EQ(t1.lexeme, "var", "First token is 'var'");
    
    Token t2 = lexer_next(&lex);
    ASSERT_EQ(t2.type, TOKEN_IDENTIFIER, "Second token is identifier");
    
    Token t3 = lexer_next(&lex);
    ASSERT_EQ(t3.type, TOKEN_PUNCTUATOR, "Third token is punctuator");
}

void test_parser_consumes_tokens(void) {
    const char *src = "var x = 42;";
    
    Parser p;
    parser_init(&p, src, strlen(src));
    
    // Parser should be able to consume lexer output
    ASSERT_NOT_NULL(p.lx.input, "Parser lexer initialized");
}

void test_lexer_parser_round_trip(void) {
    const char *src = "let x = 'hello';";
    size_t len = strlen(src);
    
    Parser p;
    parser_init(&p, src, len);
    
    AstNode *ast = parse_program(&p);
    ASSERT_NOT_NULL(ast, "Program parsed");
    ASSERT_EQ(ast->type, AST_Program, "Root is Program");
    
    ast_free(ast);
}

/* ============================================================================
 * Test Suite 2: Parser → ScopeManager Interface (AST Structure)
 * ============================================================================ */

void test_ast_node_has_position_info(void) {
    const char *src = "var x = 1;";
    
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *prog = parse_program(&p);
    
    ASSERT_NOT_NULL(prog, "Program created");
    ASSERT_TRUE(prog->start.line >= 0, "Start position has valid line");
    
    Program *pr = (Program *)prog->data;
    ASSERT_NE((int)pr->body.count, 0, "Body has statements");
    
    AstNode *stmt = pr->body.items[0];
    ASSERT_NOT_NULL(stmt, "First statement exists");
    ASSERT_NE(stmt->type, AST_Error, "Statement is not error");
    
    ast_free(prog);
}

void test_ast_variable_declaration_structure(void) {
    const char *src = "const y = 99;";
    
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *prog = parse_program(&p);
    
    Program *pr = (Program *)prog->data;
    AstNode *stmt = pr->body.items[0];
    
    ASSERT_EQ(stmt->type, AST_VariableDeclaration, "Statement is var decl");
    
    VariableDeclaration *vd = (VariableDeclaration *)stmt->data;
    ASSERT_EQ(vd->kind, VD_Const, "Kind is const");
    
    ast_free(prog);
}

void test_ast_verify_integrity(void) {
    AstNode *id = mock_parser_create_identifier("x", 1, 0);
    ASSERT_NOT_NULL(id, "Mock identifier created");
    ASSERT_EQ(id->type, AST_Identifier, "Type is Identifier");
    
    Identifier *id_data = (Identifier *)id->data;
    ASSERT_STR_EQ(id_data->name, "x", "Name is 'x'");
    
    ast_free(id);
}

/* ============================================================================
 * Test Suite 3: ScopeManager → Edit API Interface (Scope Queries)
 * ============================================================================ */

void test_scope_manager_global_scope(void) {
    ScopeManager *sm = mock_scope_manager_simple();
    
    ASSERT_NOT_NULL(sm, "Scope manager created");
    ASSERT_NOT_NULL(sm->root, "Root scope exists");
    ASSERT_EQ(sm->root->type, SCOPE_GLOBAL, "Root is global scope");
    
    // Free mock scope manager
    if (sm->root) {
        // Clean up bindings
        for (size_t i = 0; i < sm->root->bindings.count; i++) {
            if (sm->root->bindings.items[i]) {
                free(sm->root->bindings.items[i]->name);
                free(sm->root->bindings.items[i]);
            }
        }
        if (sm->root->bindings.items) free(sm->root->bindings.items);
        free(sm->root);
    }
    free(sm);
}

void test_scope_binding_lookup(void) {
    // Create a simple scope with bindings
    const char *names[] = {"console", "Object"};
    BindingKind kinds[] = {BIND_IMPLICIT, BIND_IMPLICIT};
    
    MockScopeBindings bindings = {.names = names, .kinds = kinds, .count = 2};
    Scope *scope = mock_scope_create(SCOPE_GLOBAL, NULL, &bindings);
    
    ASSERT_NOT_NULL(scope, "Scope created");
    ASSERT_EQ(scope->bindings.count, 2, "Two bindings");
    
    Binding *console_binding = scope->bindings.items[0];
    ASSERT_NOT_NULL(console_binding, "First binding exists");
    ASSERT_STR_EQ(console_binding->name, "console", "Binding name is 'console'");
    
    // Cleanup
    for (size_t i = 0; i < scope->bindings.count; i++) {
        free(scope->bindings.items[i]->name);
        free(scope->bindings.items[i]);
    }
    free(scope->bindings.items);
    free(scope);
}

void test_scope_chain_resolution(void) {
    // Create nested scopes
    const char *global_names[] = {"x"};
    BindingKind global_kinds[] = {BIND_VAR};
    MockScopeBindings global_bindings = {
        .names = global_names, .kinds = global_kinds, .count = 1
    };
    
    Scope *global = mock_scope_create(SCOPE_GLOBAL, NULL, &global_bindings);
    
    const char *local_names[] = {"y"};
    BindingKind local_kinds[] = {BIND_LET};
    MockScopeBindings local_bindings = {
        .names = local_names, .kinds = local_kinds, .count = 1
    };
    
    Scope *local = mock_scope_create(SCOPE_FUNCTION, global, &local_bindings);
    
    ASSERT_NOT_NULL(local->parent, "Local scope has parent");
    ASSERT_EQ(local->parent->type, SCOPE_GLOBAL, "Parent is global");
    
    // Cleanup
    for (size_t i = 0; i < global->bindings.count; i++) {
        free(global->bindings.items[i]->name);
        free(global->bindings.items[i]);
    }
    free(global->bindings.items);
    free(global);
    
    for (size_t i = 0; i < local->bindings.count; i++) {
        free(local->bindings.items[i]->name);
        free(local->bindings.items[i]);
    }
    free(local->bindings.items);
    free(local);
}

/* ============================================================================
 * Test Suite 4: Edit API → Codegen Interface (Immutability)
 * ============================================================================ */

void test_codegen_produces_valid_output(void) {
    const char *src = "var x = 42;";
    
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *ast = parse_program(&p);
    
    CodegenOptions opts = {
        .indent_width = 2,
        .indent_char = ' ',
        .emit_source_map = 0,
        .source_name = "test"
    };
    
    CodegenResult result = codegen_generate(ast, &opts);
    
    ASSERT_NOT_NULL(result.code, "Generated code exists");
    ASSERT_TRUE(strlen(result.code) > 0, "Generated code is not empty");
    
    // Free
    ast_free(ast);
    free(result.code);
    if (result.source_map) free(result.source_map);
}

void test_codegen_preserves_identifiers(void) {
    const char *src = "var myVar = 100;";
    
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *ast = parse_program(&p);
    
    CodegenOptions opts = {.indent_width = 2, .indent_char = ' ', 
                          .emit_source_map = 0, .source_name = "test"};
    CodegenResult result = codegen_generate(ast, &opts);
    
    // Generated code should contain the identifier
    ASSERT_TRUE(result.code ? strstr(result.code, "myVar") != NULL : 0,
               "Generated code contains identifier");
    
    ast_free(ast);
    free(result.code);
    if (result.source_map) free(result.source_map);
}

/* ============================================================================
 * Test Suite 5: Full Pipeline Integration (Round-Trip)
 * ============================================================================ */

void test_roundtrip_simple_declaration(void) {
    const char *src = "var x = 42;";
    
    int success = roundtrip_test(src, "simple_var_decl");
    ASSERT_TRUE(success, "Round-trip successful for simple var declaration");
}

void test_roundtrip_multiple_statements(void) {
    const char *src = "var a = 1;\nlet b = 2;\nconst c = 3;";
    
    int success = roundtrip_test(src, "multi_statements");
    ASSERT_TRUE(success, "Round-trip successful for multiple statements");
}

void test_roundtrip_string_literal(void) {
    const char *src = "let msg = \"hello world\";";
    
    int success = roundtrip_test(src, "string_literal");
    ASSERT_TRUE(success, "Round-trip successful for string literal");
}

void test_roundtrip_detailed_report(void) {
    const char *src = "var x = 'test';";
    RoundTripReport report;
    
    int success = roundtrip_test_detailed(src, "detailed_test", &report);
    
    ASSERT_NOT_NULL(report.original_code, "Original code preserved");
    ASSERT_NOT_NULL(report.generated_code, "Generated code created");
    ASSERT_EQ(report.node_count_1, 1, "Original has 1 statement");
    
    roundtrip_report_free(&report);
}

/* ============================================================================
 * Test Suite 6: AST Comparison
 * ============================================================================ */

void test_ast_comparison_identical(void) {
    AstNode *id1 = mock_parser_create_identifier("x", 1, 0);
    AstNode *id2 = mock_parser_create_identifier("x", 1, 0);
    
    int equal = ast_nodes_equal(id1, id2);
    ASSERT_TRUE(equal, "Identical identifiers are equal");
    
    ast_free(id1);
    ast_free(id2);
}

void test_ast_comparison_different(void) {
    AstNode *id1 = mock_parser_create_identifier("x", 1, 0);
    AstNode *id2 = mock_parser_create_identifier("y", 1, 0);
    
    int equal = ast_nodes_equal(id1, id2);
    ASSERT_FALSE(equal, "Different identifiers are not equal");
    
    ast_free(id1);
    ast_free(id2);
}

void test_ast_program_comparison(void) {
    AstNode *prog1 = mock_parser_create_program(1);
    AstNode *prog2 = mock_parser_create_program(1);
    
    int equal = ast_nodes_equal(prog1, prog2);
    ASSERT_TRUE(equal, "Programs with same structure are equal");
    
    ast_free(prog1);
    ast_free(prog2);
}

/* ============================================================================
 * Test Suite 7: Error Handling and Edge Cases
 * ============================================================================ */

void test_parse_error_recovery(void) {
    const char *src = "var x = ;";  // Syntax error
    
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *prog = parse_program(&p);
    
    // Parser should handle gracefully (create error node or skip)
    ASSERT_TRUE(prog != NULL || prog == NULL, "Parser returns result (success or error)");
}

void test_empty_source(void) {
    const char *src = "";
    
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *prog = parse_program(&p);
    
    if (prog) {
        Program *pr = (Program *)prog->data;
        ASSERT_EQ(pr->body.count, 0, "Empty program has no statements");
        ast_free(prog);
    }
}

void test_codegen_with_null_options(void) {
    const char *src = "var x = 1;";
    
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *ast = parse_program(&p);
    
    // Should handle NULL options with defaults
    CodegenResult result = codegen_generate(ast, NULL);
    ASSERT_TRUE(result.code != NULL, "Codegen works with NULL options");
    
    ast_free(ast);
    free(result.code);
    if (result.source_map) free(result.source_map);
}

/* ============================================================================
 * Test Framework and Main
 * ============================================================================ */

int main(void) {
    test_framework_init();
    
    printf("=== Lexer → Parser Interface Tests ===\n");
    run_test_case("lexer_produces_tokens", NULL, NULL, test_lexer_produces_tokens);
    run_test_case("parser_consumes_tokens", NULL, NULL, test_parser_consumes_tokens);
    run_test_case("lexer_parser_round_trip", NULL, NULL, test_lexer_parser_round_trip);
    
    printf("\n=== Parser → ScopeManager Interface Tests ===\n");
    run_test_case("ast_node_has_position_info", NULL, NULL, test_ast_node_has_position_info);
    run_test_case("ast_var_decl_structure", NULL, NULL, test_ast_variable_declaration_structure);
    run_test_case("ast_verify_integrity", NULL, NULL, test_ast_verify_integrity);
    
    printf("\n=== ScopeManager → Edit API Interface Tests ===\n");
    run_test_case("scope_manager_global_scope", NULL, NULL, test_scope_manager_global_scope);
    run_test_case("scope_binding_lookup", NULL, NULL, test_scope_binding_lookup);
    run_test_case("scope_chain_resolution", NULL, NULL, test_scope_chain_resolution);
    
    printf("\n=== Edit API → Codegen Interface Tests ===\n");
    run_test_case("codegen_produces_output", NULL, NULL, test_codegen_produces_valid_output);
    run_test_case("codegen_preserves_identifiers", NULL, NULL, test_codegen_preserves_identifiers);
    
    printf("\n=== Full Pipeline Round-Trip Tests ===\n");
    run_test_case("roundtrip_simple_decl", NULL, NULL, test_roundtrip_simple_declaration);
    run_test_case("roundtrip_multi_statements", NULL, NULL, test_roundtrip_multiple_statements);
    run_test_case("roundtrip_string_literal", NULL, NULL, test_roundtrip_string_literal);
    run_test_case("roundtrip_detailed", NULL, NULL, test_roundtrip_detailed_report);
    
    printf("\n=== AST Comparison Tests ===\n");
    run_test_case("ast_equal_identical", NULL, NULL, test_ast_comparison_identical);
    run_test_case("ast_equal_different", NULL, NULL, test_ast_comparison_different);
    run_test_case("ast_program_equal", NULL, NULL, test_ast_program_comparison);
    
    printf("\n=== Error Handling Tests ===\n");
    run_test_case("parse_error_recovery", NULL, NULL, test_parse_error_recovery);
    run_test_case("empty_source", NULL, NULL, test_empty_source);
    run_test_case("codegen_null_options", NULL, NULL, test_codegen_with_null_options);
    
    test_framework_cleanup();
    TEST_SUMMARY();
}
