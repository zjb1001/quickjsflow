#include <stdlib.h>
#include <string.h>
#include "quickjsflow/lexer.h"
#include "quickjsflow/parser.h"
#include "quickjsflow/ast.h"
#include "test_framework.h"

void test_lexer_parser_variable_declaration(void) {
    const char *src = "var x = 42;";
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *prog = parse_program(&p);
    
    ASSERT_NOT_NULL(prog, "Program created");
    ASSERT_EQ(prog->type, AST_Program, "Program type");
    
    Program *pr = (Program *)prog->data;
    ASSERT_EQ((int)pr->body.count, 1, "Body count");
    
    AstNode *stmt = pr->body.items[0];
    ASSERT_EQ(stmt->type, AST_VariableDeclaration, "Statement type");
    
    VariableDeclaration *vd = (VariableDeclaration *)stmt->data;
    ASSERT_EQ(vd->kind, VD_Var, "Kind var");
    ASSERT_EQ((int)vd->declarations.count, 1, "Declarators count");
    
    ast_free(prog);
}

void test_lexer_parser_string_literal(void) {
    const char *src = "let s = \"hello\";";
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *prog = parse_program(&p);
    
    ASSERT_NOT_NULL(prog, "Program");
    Program *pr = (Program *)prog->data;
    ASSERT_EQ((int)pr->body.count, 1, "1 stmt");
    
    ast_free(prog);
}

void test_lexer_parser_multiple_statements(void) {
    const char *src = "var a = 1;\nlet b = 2;\nconst c = 3;";
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *prog = parse_program(&p);
    
    ASSERT_NOT_NULL(prog, "Program");
    Program *pr = (Program *)prog->data;
    ASSERT_EQ((int)pr->body.count, 3, "3 statements");
    
    ast_free(prog);
}

int main(void) {
    test_lexer_parser_variable_declaration();
    test_lexer_parser_string_literal();
    test_lexer_parser_multiple_statements();
    
    TEST_SUMMARY();
}
