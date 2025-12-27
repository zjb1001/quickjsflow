#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "quickjsflow/parser.h"
#include "test_framework.h"

void test_parser_init_value_declaration(void) {
    const char *src = "var x = 42;";
    Parser p;
    printf("Parsing: %s\n", src);
    parser_init(&p, src, strlen(src));
    printf("Parser initialized\n");
    
    AstNode *prog = parse_program(&p);
    printf("Program parsed\n");
    
    ASSERT_NOT_NULL(prog, "Program created");
    if (!prog) return;
    printf("Program is not null\n");
    
    Program *pr = (Program *)prog->data;
    printf("Got Program data, body.count = %zu\n", pr->body.count);
    
    if (pr->body.count > 0) {
        AstNode *stmt = pr->body.items[0];
        printf("Got first statement, type = %d\n", stmt->type);
        
        if (stmt->type == AST_VariableDeclaration) {
            VariableDeclaration *vd = (VariableDeclaration *)stmt->data;
            printf("VariableDeclaration kind = %d\n", vd->kind);
            printf("declarations.count = %zu\n", vd->declarations.count);
        }
    }
    
    printf("Freeing AST...\n");
    ast_free(prog);
    printf("Done\n");
}

int main(void) {
    test_parser_init_value_declaration();
    TEST_SUMMARY();
}
