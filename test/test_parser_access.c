#include <stdlib.h>
#include <string.h>
#include "quickjsflow/parser.h"
#include "test_framework.h"

void test_parser_access_program_body(void) {
    const char *src = "var x;";
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *prog = parse_program(&p);
    
    ASSERT_NOT_NULL(prog, "Program created");
    Program *pr = (Program *)prog->data;
    ASSERT_EQ(pr->body.count, 1, "Body count 1");
    
    ast_free(prog);
}

int main(void) {
    test_parser_access_program_body();
    TEST_SUMMARY();
}
