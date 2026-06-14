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
// codegen_result_free. When owner_arena is set, the code buffer is arena-backed
// and codegen_result_free will not free() it.
typedef struct {
    char *code;       // generated JavaScript source
    char *source_map; // JSON string; NULL when emit_source_map==0
    Arena *owner_arena;       // internal: set by qjsf_codegen()
} CodegenResult;

CodegenResult codegen_generate(const AstNode *root, const CodegenOptions *options);
void codegen_result_free(CodegenResult *result);

/* Context-based codegen: allocates output from ctx's arena.
 * The returned CodegenResult.code is arena-backed; freed with ctx. */
struct qjsf_context_s;
CodegenResult qjsf_codegen(struct qjsf_context_s *ctx, const AstNode *root, const CodegenOptions *options);

#endif
