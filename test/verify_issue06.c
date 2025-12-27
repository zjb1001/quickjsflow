#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quickjsflow/lexer.h"
#include "quickjsflow/parser.h"
#include "quickjsflow/ast.h"
#include "quickjsflow/codegen.h"

int main(void) {
    // Test 1: Simple expression with source map
    const char *src1 = "var x = 42;";
    
    Parser p1;
    parser_init(&p1, src1, strlen(src1));
    AstNode *ast1 = parse_program(&p1);

    CodegenOptions opts = {0};
    opts.indent_width = 2;
    opts.emit_source_map = 1;
    opts.source_name = "input.js";
    
    CodegenResult gen1 = codegen_generate(ast1, &opts);
    
    printf("Test 1: Simple variable declaration\n");
    printf("Code: %s\n", gen1.code ? gen1.code : "(null)");
    printf("Map:  %s\n\n", gen1.source_map ? gen1.source_map : "(null)");
    
    codegen_result_free(&gen1);
    ast_free(ast1);

    // Test 2: Multiple statements
    const char *src2 = "var a = 1;\nvar b = 2;\nvar c = a + b;";
    
    Parser p2;
    parser_init(&p2, src2, strlen(src2));
    AstNode *ast2 = parse_program(&p2);
    
    CodegenResult gen2 = codegen_generate(ast2, &opts);
    
    printf("Test 2: Multiple statements\n");
    printf("Code:\n%s\n", gen2.code ? gen2.code : "(null)");
    printf("Map: %s\n\n", gen2.source_map ? gen2.source_map : "(null)");
    
    codegen_result_free(&gen2);
    ast_free(ast2);

    // Test 3: With comments
    const char *src3 = "// comment\nvar x = 1;";
    
    Parser p3;
    parser_init(&p3, src3, strlen(src3));
    AstNode *ast3 = parse_program(&p3);
    
    Program *pr3 = (Program *)ast3->data;
    printf("Test 3: With comments\n");
    printf("Comments captured: %zu\n", pr3->comment_count);
    
    CodegenResult gen3 = codegen_generate(ast3, &opts);
    printf("Code:\n%s\n", gen3.code ? gen3.code : "(null)");
    
    codegen_result_free(&gen3);
    ast_free(ast3);

    // Test 4: Function with body
    const char *src4 = "function f(x) {\n  return x + 1;\n}";
    
    Parser p4;
    parser_init(&p4, src4, strlen(src4));
    AstNode *ast4 = parse_program(&p4);
    
    CodegenResult gen4 = codegen_generate(ast4, &opts);
    
    printf("\nTest 4: Function declaration\n");
    printf("Code:\n%s\n", gen4.code ? gen4.code : "(null)");
    printf("Map: %s\n", gen4.source_map ? gen4.source_map : "(null)");
    
    codegen_result_free(&gen4);
    ast_free(ast4);

    return 0;
}
