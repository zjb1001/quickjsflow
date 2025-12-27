#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quickjsflow/lexer.h"
#include "quickjsflow/parser.h"
#include "quickjsflow/ast.h"
#include "quickjsflow/codegen.h"

int main(void) {
    const char *src = 
        "// This is a leading comment\n"
        "var x = 1; // inline comment\n"
        "let y = 2;\n"
        "/* block comment */\n"
        "function add(a, b) {\n"
        "  return a + b;\n"
        "}\n";

    printf("=== Original Source ===\n%s\n", src);

    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *ast = parse_program(&p);

    if (!ast) {
        printf("Parse failed\n");
        return 1;
    }

    Program *pr = (Program *)ast->data;
    printf("=== Parse Results ===\n");
    printf("Statements: %zu\n", pr->body.count);
    printf("Comments captured: %zu\n", pr->comment_count);
    for (size_t i = 0; i < pr->comment_count; i++) {
        printf("  Comment %zu: %s '%s'\n", i+1, 
               pr->comments[i]->is_block ? "block" : "line",
               pr->comments[i]->text);
    }

    // Test codegen without source map
    printf("\n=== Codegen (no source map) ===\n");
    CodegenOptions opts1 = {0};
    CodegenResult gen1 = codegen_generate(ast, &opts1);
    if (gen1.code) {
        printf("%s\n", gen1.code);
    } else {
        printf("Codegen failed\n");
    }

    // Test codegen with source map
    printf("\n=== Codegen (with source map) ===\n");
    CodegenOptions opts2 = {0};
    opts2.emit_source_map = 1;
    opts2.source_name = "test.js";
    CodegenResult gen2 = codegen_generate(ast, &opts2);
    if (gen2.code) {
        printf("Generated code:\n%s\n", gen2.code);
    }
    if (gen2.source_map) {
        printf("\nSource Map:\n%s\n", gen2.source_map);
    } else {
        printf("No source map generated\n");
    }

    codegen_result_free(&gen1);
    codegen_result_free(&gen2);
    ast_free(ast);

    printf("\n=== Test Complete ===\n");
    return 0;
}
