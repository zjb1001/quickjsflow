#ifndef QUICKJSFLOW_CODEGEN_H
#define QUICKJSFLOW_CODEGEN_H

#include "quickjsflow/ast.h"

// Options to control formatting and optional source map emission.
typedef struct {
    int indent_width;    // number of indent characters per level (default: 2)
    char indent_char;    // indent character (default: ' ')
    int emit_source_map; // 0 = off, 1 = emit source map json
    const char *source_name; // optional name of original source file
} CodegenOptions;

// Result of code generation. Caller owns the buffers and must free with
// codegen_result_free.
typedef struct {
    char *code;       // generated JavaScript source
    char *source_map; // JSON string; NULL when emit_source_map==0
} CodegenResult;

CodegenResult codegen_generate(const AstNode *root, const CodegenOptions *options);
void codegen_result_free(CodegenResult *result);

#endif
