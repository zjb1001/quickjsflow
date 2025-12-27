#include <stdlib.h>
#include <string.h>
#include "quickjsflow/lexer.h"
#include "test_framework.h"

void test_lexer_simple(void) {
    const char *src = "var x = 1;";
    Lexer lx;
    lexer_init(&lx, src, strlen(src));
    
    Token t1 = lexer_next(&lx);
    ASSERT_EQ(t1.type, TOKEN_IDENTIFIER, "First token is identifier");
    token_free(&t1);
    
    Token t2 = lexer_next(&lx);
    ASSERT_EQ(t2.type, TOKEN_IDENTIFIER, "Second token is identifier");
    token_free(&t2);
}

int main(void) {
    test_lexer_simple();
    TEST_SUMMARY();
}
