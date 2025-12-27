#include <stdlib.h>
#include <string.h>
#include "quickjsflow/lexer.h"
#include "quickjsflow/parser.h"
#include "quickjsflow/ast.h"
#include "test_framework.h"

// Round-trip test: parse → print → parse again, check structure equivalence
static int ast_nodes_equal(const AstNode *a, const AstNode *b) {
    if (!a && !b) return 1;
    if (!a || !b) return 0;
    if (a->type != b->type) return 0;
    // TODO: deep structural comparison (can be simplified for MVP)
    return 1;
}

void test_roundtrip_simple_var_declaration(void) {
    const char *src = "var x = 42;";
    
    // Parse first time
    Parser p1;
    parser_init(&p1, src, strlen(src));
    AstNode *ast1 = parse_program(&p1);
    
    // Print to string (TODO: implement codegen)
    // For now, just verify AST structure is parseable
    
    ASSERT_NOT_NULL(ast1, "First parse successful");
    ASSERT_EQ(ast1->type, AST_Program, "Root is Program");
    
    // TODO: after codegen is ready, print and re-parse to verify equivalence
    
    ast_free(ast1);
}

void test_roundtrip_multiple_statements(void) {
    const char *src = "var a = 1;\nlet b = \"hello\";\nx++;";
    
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *ast = parse_program(&p);
    
    ASSERT_NOT_NULL(ast, "Parse successful");
    Program *pr = (Program *)ast->data;
    ASSERT_EQ(pr->body.count, 3, "3 statements preserved");
    
    ast_free(ast);
}

void test_roundtrip_with_comments(void) {
    const char *src = "// comment\nvar x = 1; // inline";
    
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *ast = parse_program(&p);
    
    ASSERT_NOT_NULL(ast, "Parse handles comments");
    // TODO: verify comment preservation after codegen
    
    ast_free(ast);
}

int main(void) {
    test_roundtrip_simple_var_declaration();
    test_roundtrip_multiple_statements();
    test_roundtrip_with_comments();
    
    TEST_SUMMARY();
}
