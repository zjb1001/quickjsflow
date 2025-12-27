#include <stdlib.h>
#include <string.h>
#include "quickjsflow/parser.h"
#include "test_framework.h"

void test_parser_simple_parse(void) {
    const char *src = "var x;";
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *prog = parse_program(&p);
    
    ASSERT_NOT_NULL(prog, "Program created");
    ast_free(prog);
}

int main(void) {
    test_parser_simple_parse();
    TEST_SUMMARY();
}
