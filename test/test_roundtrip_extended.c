/**
 * @file test_roundtrip_extended.c
 * @brief Extended round-trip testing with detailed comparison and snapshots
 * 
 * Tests the complete pipeline: Parse → Generate → Parse → Compare
 * Ensures AST structure is preserved through code generation
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "quickjsflow/lexer.h"
#include "quickjsflow/parser.h"
#include "quickjsflow/ast.h"
#include "quickjsflow/codegen.h"
#include "test_framework_extended.h"

/* ============================================================================
 * AST Comparison Implementation
 * ============================================================================ */

static int compare_positions(const Position *p1, const Position *p2) {
    if (!p1 || !p2) return (p1 == p2) ? 1 : 0;
    return (p1->line == p2->line && p1->column == p2->column);
}

static int compare_identifiers(const Identifier *id1, const Identifier *id2) {
    if (!id1 && !id2) return 1;
    if (!id1 || !id2) return 0;
    return (id1->name && id2->name) ? (strcmp(id1->name, id2->name) == 0) : (id1->name == id2->name);
}

static int compare_literals(const Literal *lit1, const Literal *lit2) {
    if (!lit1 && !lit2) return 1;
    if (!lit1 || !lit2) return 0;
    return (lit1->kind == lit2->kind) &&
           ((lit1->raw && lit2->raw) ? (strcmp(lit1->raw, lit2->raw) == 0) : (lit1->raw == lit2->raw));
}

static int ast_nodes_equal_impl(const AstNode *node1, const AstNode *node2);

static int compare_variable_declaration(const VariableDeclaration *vd1,
                                       const VariableDeclaration *vd2,
                                       const AstNode *ast1,
                                       const AstNode *ast2) {
    if (!vd1 && !vd2) return 1;
    if (!vd1 || !vd2) return 0;
    
    if (vd1->kind != vd2->kind) return 0;
    if (vd1->declarations.count != vd2->declarations.count) return 0;
    
    // Simplified: just check count matches
    // Full implementation would recursively compare all declarators
    return 1;
}

int ast_nodes_equal(const AstNode *node1, const AstNode *node2) {
    return ast_nodes_equal_impl(node1, node2);
}

static int ast_nodes_equal_impl(const AstNode *node1, const AstNode *node2) {
    if (!node1 && !node2) return 1;
    if (!node1 || !node2) return 0;
    
    // Type must match
    if (node1->type != node2->type) return 0;
    
    // Position information should be similar (may vary slightly due to formatting)
    // For structural equality, we don't strictly require position match
    
    // Type-specific comparison
    switch (node1->type) {
        case AST_Program: {
            Program *p1 = (Program *)node1->data;
            Program *p2 = (Program *)node2->data;
            if (!p1 || !p2) return (p1 == p2);
            
            if (p1->body.count != p2->body.count) return 0;
            for (size_t i = 0; i < p1->body.count; i++) {
                if (!ast_nodes_equal_impl(p1->body.items[i], p2->body.items[i]))
                    return 0;
            }
            return 1;
        }
        
        case AST_Identifier: {
            Identifier *id1 = (Identifier *)node1->data;
            Identifier *id2 = (Identifier *)node2->data;
            return compare_identifiers(id1, id2);
        }
        
        case AST_Literal: {
            Literal *lit1 = (Literal *)node1->data;
            Literal *lit2 = (Literal *)node2->data;
            return compare_literals(lit1, lit2);
        }
        
        case AST_VariableDeclaration: {
            VariableDeclaration *vd1 = (VariableDeclaration *)node1->data;
            VariableDeclaration *vd2 = (VariableDeclaration *)node2->data;
            return compare_variable_declaration(vd1, vd2, node1, node2);
        }
        
        case AST_ExpressionStatement: {
            ExpressionStatement *es1 = (ExpressionStatement *)node1->data;
            ExpressionStatement *es2 = (ExpressionStatement *)node2->data;
            if (!es1 || !es2) return (es1 == es2);
            return ast_nodes_equal_impl(es1->expression, es2->expression);
        }
        
        case AST_BlockStatement: {
            BlockStatement *bs1 = (BlockStatement *)node1->data;
            BlockStatement *bs2 = (BlockStatement *)node2->data;
            if (!bs1 || !bs2) return (bs1 == bs2);
            if (bs1->body.count != bs2->body.count) return 0;
            for (size_t i = 0; i < bs1->body.count; i++) {
                if (!ast_nodes_equal_impl(bs1->body.items[i], bs2->body.items[i]))
                    return 0;
            }
            return 1;
        }
        
        // For other node types, return 1 (simplified for MVP)
        default:
            return 1;
    }
}

/* ============================================================================
 * Round-Trip Testing
 * ============================================================================ */

int roundtrip_test(const char *source, const char *test_name) {
    if (!source || !test_name) {
        fprintf(stderr, "roundtrip_test: NULL input\n");
        return 0;
    }
    
    size_t source_len = strlen(source);
    
    // Step 1: Parse original source
    Parser p1;
    parser_init(&p1, source, source_len);
    AstNode *ast1 = parse_program(&p1);
    
    if (!ast1) {
        fprintf(stderr, "Round-trip FAIL [%s]: Failed to parse original source\n", test_name);
        return 0;
    }
    
    // Step 2: Generate code from AST1
    CodegenOptions opts = {
        .indent_width = 2,
        .indent_char = ' ',
        .emit_source_map = 0,
        .source_name = test_name
    };
    
    CodegenResult gen_result = codegen_generate(ast1, &opts);
    if (!gen_result.code) {
        fprintf(stderr, "Round-trip FAIL [%s]: Code generation failed\n", test_name);
        ast_free(ast1);
        return 0;
    }
    
    // Step 3: Parse generated code
    Parser p2;
    parser_init(&p2, gen_result.code, strlen(gen_result.code));
    AstNode *ast2 = parse_program(&p2);
    
    if (!ast2) {
        fprintf(stderr, "Round-trip FAIL [%s]: Failed to parse generated code\n", test_name);
        ast_free(ast1);
        free(gen_result.code);
        if (gen_result.source_map) free(gen_result.source_map);
        return 0;
    }
    
    // Step 4: Compare ASTs structurally
    int match = ast_nodes_equal(ast1, ast2);
    
    if (!match) {
        fprintf(stderr, "Round-trip FAIL [%s]: AST structure mismatch\n", test_name);
        fprintf(stderr, "  Original code:\n%s\n", source);
        fprintf(stderr, "  Generated code:\n%s\n", gen_result.code);
    }
    
    // Cleanup
    ast_free(ast1);
    ast_free(ast2);
    free(gen_result.code);
    if (gen_result.source_map) free(gen_result.source_map);
    
    return match;
}

int roundtrip_test_detailed(const char *source, const char *test_name,
                           RoundTripReport *report) {
    if (!report) return 0;
    memset(report, 0, sizeof(RoundTripReport));
    
    if (!source || !test_name) {
        report->success = 0;
        report->mismatch_reason = "NULL input";
        return 0;
    }
    
    size_t source_len = strlen(source);
    report->original_code = malloc(source_len + 1);
    strcpy(report->original_code, source);
    
    // Parse original
    Parser p1;
    parser_init(&p1, source, source_len);
    AstNode *ast1 = parse_program(&p1);
    
    if (!ast1) {
        report->success = 0;
        report->mismatch_reason = "Parse original failed";
        return 0;
    }
    
    // Generate code
    CodegenOptions opts = {
        .indent_width = 2,
        .indent_char = ' ',
        .emit_source_map = 0,
        .source_name = test_name
    };
    
    CodegenResult gen_result = codegen_generate(ast1, &opts);
    if (!gen_result.code) {
        report->success = 0;
        report->mismatch_reason = "Codegen failed";
        ast_free(ast1);
        return 0;
    }
    
    size_t gen_len = strlen(gen_result.code);
    report->generated_code = malloc(gen_len + 1);
    strcpy(report->generated_code, gen_result.code);
    
    // Parse generated
    Parser p2;
    parser_init(&p2, gen_result.code, gen_len);
    AstNode *ast2 = parse_program(&p2);
    
    if (!ast2) {
        report->success = 0;
        report->mismatch_reason = "Parse generated failed";
        ast_free(ast1);
        free(gen_result.code);
        if (gen_result.source_map) free(gen_result.source_map);
        return 0;
    }
    
    // Compare
    int match = ast_nodes_equal(ast1, ast2);
    report->success = match;
    if (!match) {
        report->mismatch_reason = "AST structure differs";
    }
    
    // Count nodes (simplified)
    if (ast1 && ast1->type == AST_Program) {
        Program *p = (Program *)ast1->data;
        report->node_count_1 = p->body.count;
    }
    if (ast2 && ast2->type == AST_Program) {
        Program *p = (Program *)ast2->data;
        report->node_count_2 = p->body.count;
    }
    
    // Cleanup
    ast_free(ast1);
    ast_free(ast2);
    free(gen_result.code);
    if (gen_result.source_map) free(gen_result.source_map);
    
    return match;
}

void roundtrip_report_free(RoundTripReport *report) {
    if (!report) return;
    if (report->original_code) free(report->original_code);
    if (report->generated_code) free(report->generated_code);
}

/* ============================================================================
 * Snapshot Testing Implementation
 * ============================================================================ */

int snapshot_match(const char *test_name, const char *actual) {
    if (!test_name || !actual) return 0;
    
    // Build snapshot filename
    char snapshot_path[512];
    snprintf(snapshot_path, sizeof(snapshot_path), "test/snapshots/%s.snapshot", test_name);
    
    // Try to read expected snapshot
    FILE *fp = fopen(snapshot_path, "r");
    if (!fp) {
        fprintf(stderr, "Snapshot not found: %s\n", snapshot_path);
        fprintf(stderr, "Create with SNAPSHOT_UPDATE=1\n");
        return 0;
    }
    
    // Read expected content
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *expected = malloc(size + 1);
    fread(expected, 1, size, fp);
    expected[size] = '\0';
    fclose(fp);
    
    int match = (strcmp(actual, expected) == 0);
    
    if (!match) {
        // Write actual for comparison
        char actual_path[512];
        snprintf(actual_path, sizeof(actual_path), "test/snapshots/%s.actual", test_name);
        FILE *actual_fp = fopen(actual_path, "w");
        if (actual_fp) {
            fputs(actual, actual_fp);
            fclose(actual_fp);
        }
        fprintf(stderr, "Snapshot mismatch for %s\n", test_name);
        fprintf(stderr, "Expected: %s\nActual: %s\nActual saved to: %s\n",
                snapshot_path, actual, actual_path);
    }
    
    free(expected);
    return match;
}

void snapshot_update(const char *test_name, const char *actual) {
    if (!test_name || !actual) return;
    
    char snapshot_path[512];
    snprintf(snapshot_path, sizeof(snapshot_path), "test/snapshots/%s.snapshot", test_name);
    
    FILE *fp = fopen(snapshot_path, "w");
    if (fp) {
        fputs(actual, fp);
        fclose(fp);
    }
}

char *snapshot_load(const char *test_name) {
    if (!test_name) return NULL;
    
    char snapshot_path[512];
    snprintf(snapshot_path, sizeof(snapshot_path), "test/snapshots/%s.snapshot", test_name);
    
    FILE *fp = fopen(snapshot_path, "r");
    if (!fp) return NULL;
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    fread(content, 1, size, fp);
    content[size] = '\0';
    fclose(fp);
    
    return content;
}

void snapshot_save(const char *test_name, const char *content) {
    if (!test_name || !content) return;
    snapshot_update(test_name, content);
}

/* ============================================================================
 * Test Framework Implementation
 * ============================================================================ */

ExtendedTestStats test_stats = {0, 0, 0, 0, NULL, 0, 0};

void test_framework_init(void) {
    test_stats.passed = 0;
    test_stats.failed = 0;
    test_stats.skipped = 0;
    test_stats.assertions = 0;
}

void test_framework_cleanup(void) {
    if (test_stats.results) {
        free(test_stats.results);
        test_stats.results = NULL;
    }
}

void skip_test(const char *reason) {
    test_stats.skipped++;
    fprintf(stdout, "SKIP: %s\n", reason ? reason : "(no reason)");
}

void run_test_case(const char *name, 
                   void (*setup)(void),
                   void (*teardown)(void),
                   void (*test)(void)) {
    if (setup) setup();
    
    int before_failed = test_stats.failed;
    test();
    int after_failed = test_stats.failed;
    
    if (after_failed > before_failed) {
        fprintf(stderr, "TEST FAILED: %s\n", name);
    }
    
    if (teardown) teardown();
}

void test_summary_detailed(FILE *fp) {
    if (!fp) return;
    
    fprintf(fp, "=== Detailed Test Summary ===\n");
    fprintf(fp, "Passed:      %d\n", test_stats.passed);
    fprintf(fp, "Failed:      %d\n", test_stats.failed);
    fprintf(fp, "Skipped:     %d\n", test_stats.skipped);
    fprintf(fp, "Assertions:  %d\n", test_stats.assertions);
    fprintf(fp, "Total:       %d\n", 
            test_stats.passed + test_stats.failed + test_stats.skipped);
}

char *test_results_to_json(void) {
    // Simple JSON-like output (manual string building)
    // For MVP, just use printf-style output
    char *result = malloc(1024);
    if (!result) return NULL;
    
    snprintf(result, 1024,
        "{\"passed\": %d, \"failed\": %d, \"skipped\": %d, \"assertions\": %d}",
        test_stats.passed, test_stats.failed, test_stats.skipped, test_stats.assertions);
    
    return result;
}

char *ast_to_json(const AstNode *node) {
    // Simple JSON-like string generation (not full cJSON)
    if (!node) return NULL;
    
    char *buf = malloc(512);
    if (!buf) return NULL;
    
    const char *type_name = "Unknown";
    switch (node->type) {
        case AST_Program: type_name = "Program"; break;
        case AST_Identifier: type_name = "Identifier"; break;
        case AST_Literal: type_name = "Literal"; break;
        case AST_VariableDeclaration: type_name = "VariableDeclaration"; break;
        case AST_ExpressionStatement: type_name = "ExpressionStatement"; break;
        case AST_BlockStatement: type_name = "BlockStatement"; break;
        default: type_name = "Unknown"; break;
    }
    
    snprintf(buf, 512, 
        "{\"type\": \"%s\", \"position\": {\"line\": %d, \"column\": %d}}",
        type_name, node->start.line, node->start.column);
    
    return buf;
}

char *ast_to_string(const AstNode *node, int indent) {
    // Simple implementation: allocate a reasonable buffer
    char *buf = malloc(4096);
    if (!buf) return NULL;
    
    int written = snprintf(buf, 4096, "%*sNode(type=%d, pos=(%d,%d))\n",
                          indent, "", node->type, 
                          node->start.line, node->start.column);
    
    if (written >= 4096) {
        // Buffer too small
        free(buf);
        return NULL;
    }
    
    return buf;
}