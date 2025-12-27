#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "../include/quickjsflow/lexer.h"
#include "../include/quickjsflow/parser.h"

// AFL/libFuzzer integration
#ifdef __AFL_FUZZ_TESTCASE_LEN
  __AFL_FUZZ_INIT();
#endif

// Fuzz target for AFL
#ifdef __AFL_HAVE_MANUAL_CONTROL
  #define AFL_INIT() __AFL_INIT()
  #define AFL_LOOP(n) __AFL_LOOP(n)
#else
  #define AFL_INIT()
  #define AFL_LOOP(n) 1
#endif

// libFuzzer entry point
#ifdef LIBFUZZER
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0 || size > 1024 * 1024) return 0; // Skip empty or too large
    
    char* code = malloc(size + 1);
    if (!code) return 0;
    
    memcpy(code, data, size);
    code[size] = '\0';
    
    // Test lexer
    Lexer lexer;
    lexer_init(&lexer, code, size);
    
    Token token;
    int token_count = 0;
    do {
        token = lexer_next(&lexer);
        token_free(&token);
        token_count++;
        if (token_count > 100000) break; // Prevent infinite loops
    } while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);
    
    // Test parser
    Parser parser;
    parser_init(&parser, code, size);
    
    AstNode* program = parse_program(&parser);
    if (program) {
        ast_free(program);
    }
    
    free(code);
    return 0;
}
#endif

// Standalone fuzzer for AFL
#ifndef LIBFUZZER
int main(int argc, char** argv) {
    #ifdef __AFL_HAVE_MANUAL_CONTROL
    AFL_INIT();
    #endif
    
    char* input_file = NULL;
    
    if (argc > 1) {
        input_file = argv[1];
    } else {
        input_file = "stdin";
    }
    
    #ifdef __AFL_HAVE_MANUAL_CONTROL
    while (AFL_LOOP(1000)) {
    #endif
        FILE* fp;
        
        if (strcmp(input_file, "stdin") == 0) {
            fp = stdin;
        } else {
            fp = fopen(input_file, "rb");
            if (!fp) {
                fprintf(stderr, "Cannot open %s\n", input_file);
                return 1;
            }
        }
        
        // Read input
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        
        if (size < 0 || size > 1024 * 1024) {
            if (fp != stdin) fclose(fp);
            continue;
        }
        
        char* code = malloc(size + 1);
        if (!code) {
            if (fp != stdin) fclose(fp);
            continue;
        }
        
        size_t read_size = fread(code, 1, size, fp);
        code[read_size] = '\0';
        
        if (fp != stdin) fclose(fp);
        
        // Fuzz lexer
        Lexer lexer;
        lexer_init(&lexer, code, read_size);
        
        Token token;
        int token_count = 0;
        do {
            token = lexer_next(&lexer);
            token_free(&token);
            token_count++;
            if (token_count > 100000) break;
        } while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);
        
        // Fuzz parser
        Parser parser;
        parser_init(&parser, code, read_size);
        
        AstNode* program = parse_program(&parser);
        if (program) {
            ast_free(program);
        }
        
        free(code);
        
    #ifdef __AFL_HAVE_MANUAL_CONTROL
    }
    #endif
    
    return 0;
}
#endif
