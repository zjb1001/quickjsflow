#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quickjsflow/lexer.h"
#include "quickjsflow/parser.h"
#include "quickjsflow/cfg.h"
#include "test_framework.h"

// ============================================================================
// 测试辅助函数
// ============================================================================

static CFG *parse_and_build_cfg(const char *code) {
    Parser parser;
    parser_init(&parser, code, strlen(code));
    
    AstNode *ast = parse_program(&parser);
    
    if (!ast || ast->type != AST_Program) {
        return NULL;
    }
    
    Program *program = (Program *)ast->data;
    if (program->body.count == 0) {
        return NULL;
    }
    
    // 获取第一个函数声明
    AstNode *func = NULL;
    for (size_t i = 0; i < program->body.count; i++) {
        if (program->body.items[i]->type == AST_FunctionDeclaration) {
            func = program->body.items[i];
            break;
        }
    }
    
    if (!func) {
        return NULL;
    }
    
    CFG *cfg = qjs_build_cfg(func, NULL, NULL);
    return cfg;
}

// ============================================================================
// 基础 CFG 构建测试
// ============================================================================

void test_basic_block_creation() {
    BasicBlock *bb = qjs_bb_new(0);
    ASSERT_NOT_NULL(bb, "Should create basic block");
    ASSERT_EQ(bb->id, 0, "Block ID should be 0");
    ASSERT_EQ(bb->stmt_count, 0, "Block should have no statements initially");
    ASSERT_EQ(bb->out_count, 0, "Block should have no outgoing edges initially");
    ASSERT_EQ(bb->in_count, 0, "Block should have no incoming edges initially");
    
    qjs_bb_free(bb);
}

void test_add_statement_to_block() {
    BasicBlock *bb = qjs_bb_new(0);
    
    // 创建一个简单的 Identifier 节点用于测试
    Identifier *id_data = (Identifier *)malloc(sizeof(Identifier));
    id_data->name = "x";
    
    AstNode *stmt = (AstNode *)malloc(sizeof(AstNode));
    stmt->type = AST_Identifier;
    stmt->data = id_data;
    
    qjs_bb_add_stmt(bb, stmt);
    
    ASSERT_EQ(bb->stmt_count, 1, "Block should have 1 statement");
    ASSERT_EQ(bb->statements[0], stmt, "Statement should be added correctly");
    
    free(stmt);
    free(id_data);
    qjs_bb_free(bb);
}

void test_cfg_edge_creation() {
    BasicBlock *bb1 = qjs_bb_new(0);
    BasicBlock *bb2 = qjs_bb_new(1);
    
    CFGEdge *edge = qjs_cfg_edge_new(CFG_EDGE_NORMAL, bb1, bb2, NULL);
    
    ASSERT_NOT_NULL(edge, "Should create edge");
    ASSERT_EQ(edge->type, CFG_EDGE_NORMAL, "Edge type should be NORMAL");
    ASSERT_EQ(edge->from, bb1, "Edge from should be bb1");
    ASSERT_EQ(edge->to, bb2, "Edge to should be bb2");
    
    qjs_cfg_edge_free(edge);
    qjs_bb_free(bb1);
    qjs_bb_free(bb2);
}

void test_cfg_connect_blocks() {
    BasicBlock *bb1 = qjs_bb_new(0);
    BasicBlock *bb2 = qjs_bb_new(1);
    
    qjs_cfg_connect(bb1, bb2, CFG_EDGE_NORMAL, NULL);
    
    ASSERT_EQ(bb1->out_count, 1, "bb1 should have 1 outgoing edge");
    ASSERT_EQ(bb2->in_count, 1, "bb2 should have 1 incoming edge");
    ASSERT_EQ(bb1->outgoing[0]->to, bb2, "Outgoing edge should point to bb2");
    ASSERT_EQ(bb2->incoming[0]->from, bb1, "Incoming edge should be from bb1");
    
    qjs_bb_free(bb1);
    qjs_bb_free(bb2);
}

// ============================================================================
// 控制流构建测试
// ============================================================================

void test_sequential_statements_cfg() {
    const char *code = 
        "function test() {\n"
        "  var x = 1;\n"
        "  var y = 2;\n"
        "  var z = 3;\n"
        "}\n";
    
    CFG *cfg = parse_and_build_cfg(code);
    ASSERT_NOT_NULL(cfg, "Should build CFG for sequential statements");
    
    // 应该有: entry -> 语句块 -> exit
    ASSERT_NOT_NULL(cfg->entry, "CFG should have entry block");
    ASSERT_NOT_NULL(cfg->exit, "CFG should have exit block");
    
    qjs_cfg_free(cfg);
}

void test_if_statement_cfg() {
    const char *code = 
        "function test(x) {\n"
        "  if (x > 0) {\n"
        "    console.log('positive');\n"
        "  } else {\n"
        "    console.log('non-positive');\n"
        "  }\n"
        "}\n";
    
    CFG *cfg = parse_and_build_cfg(code);
    ASSERT_NOT_NULL(cfg, "Should build CFG for if statement");
    
    // 应该有条件分支
    int has_true_edge = 0;
    int has_false_edge = 0;
    
    for (size_t i = 0; i < cfg->edge_count; i++) {
        if (cfg->edges[i]->type == CFG_EDGE_TRUE) has_true_edge = 1;
        if (cfg->edges[i]->type == CFG_EDGE_FALSE) has_false_edge = 1;
    }
    
    ASSERT_EQ(has_true_edge, 1, "CFG should have TRUE edge for if");
    ASSERT_EQ(has_false_edge, 1, "CFG should have FALSE edge for else");
    
    qjs_cfg_free(cfg);
}

void test_while_loop_cfg() {
    const char *code = 
        "function test(x) {\n"
        "  while (x > 0) {\n"
        "    x--;\n"
        "  }\n"
        "}\n";
    
    CFG *cfg = parse_and_build_cfg(code);
    ASSERT_NOT_NULL(cfg, "Should build CFG for while loop");
    
    // 应该有循环回边
    int has_loop_edge = 0;
    for (size_t i = 0; i < cfg->edge_count; i++) {
        CFGEdge *edge = cfg->edges[i];
        if (edge->type == CFG_EDGE_CONTINUE) {
            has_loop_edge = 1;
        }
    }
    
    ASSERT_EQ(has_loop_edge, 1, "CFG should have loop back edge");
    
    qjs_cfg_free(cfg);
}

void test_for_loop_cfg() {
    const char *code = 
        "function test() {\n"
        "  for (var i = 0; i < 10; i++) {\n"
        "    console.log(i);\n"
        "  }\n"
        "}\n";
    
    CFG *cfg = parse_and_build_cfg(code);
    ASSERT_NOT_NULL(cfg, "Should build CFG for for loop");
    
    qjs_cfg_free(cfg);
}

// ============================================================================
// 导出功能测试
// ============================================================================

void test_cfg_to_json() {
    const char *code = 
        "function test() {\n"
        "  var x = 1;\n"
        "}\n";
    
    CFG *cfg = parse_and_build_cfg(code);
    ASSERT_NOT_NULL(cfg, "Should build CFG");
    
    char *json = qjs_cfg_to_json(cfg);
    ASSERT_NOT_NULL(json, "Should export CFG to JSON");
    
    // 检查 JSON 内容
    int has_blocks = (strstr(json, "\"blocks\"") != NULL);
    int has_edges = (strstr(json, "\"edges\"") != NULL);
    ASSERT_EQ(has_blocks, 1, "JSON should contain blocks");
    ASSERT_EQ(has_edges, 1, "JSON should contain edges");
    
    free(json);
    qjs_cfg_free(cfg);
}

void test_cfg_to_dot() {
    const char *code = 
        "function test() {\n"
        "  var x = 1;\n"
        "}\n";
    
    CFG *cfg = parse_and_build_cfg(code);
    ASSERT_NOT_NULL(cfg, "Should build CFG");
    
    char *dot = qjs_cfg_to_dot(cfg);
    ASSERT_NOT_NULL(dot, "Should export CFG to DOT");
    
    // 检查 DOT 内容
    int has_digraph = (strstr(dot, "digraph") != NULL);
    int has_edges = (strstr(dot, "->") != NULL);
    ASSERT_EQ(has_digraph, 1, "DOT should contain digraph");
    ASSERT_EQ(has_edges, 1, "DOT should contain edges");
    
    free(dot);
    qjs_cfg_free(cfg);
}

void test_cfg_to_mermaid() {
    const char *code = 
        "function test() {\n"
        "  var x = 1;\n"
        "}\n";
    
    CFG *cfg = parse_and_build_cfg(code);
    ASSERT_NOT_NULL(cfg, "Should build CFG");
    
    char *mermaid = qjs_cfg_to_mermaid(cfg);
    ASSERT_NOT_NULL(mermaid, "Should export CFG to Mermaid");
    
    // 检查 Mermaid 内容
    int has_flowchart = (strstr(mermaid, "flowchart") != NULL);
    ASSERT_EQ(has_flowchart, 1, "Mermaid should contain flowchart");
    
    free(mermaid);
    qjs_cfg_free(cfg);
}

// ============================================================================
// 查询测试
// ============================================================================

void test_block_successors() {
    BasicBlock *bb1 = qjs_bb_new(0);
    BasicBlock *bb2 = qjs_bb_new(1);
    BasicBlock *bb3 = qjs_bb_new(2);
    
    qjs_cfg_connect(bb1, bb2, CFG_EDGE_TRUE, NULL);
    qjs_cfg_connect(bb1, bb3, CFG_EDGE_FALSE, NULL);
    
    size_t count = 0;
    BasicBlock **successors = qjs_bb_successors(bb1, &count);
    
    ASSERT_EQ(count, 2, "bb1 should have 2 successors");
    ASSERT_NOT_NULL(successors, "Should return successors");
    
    free(successors);
    qjs_bb_free(bb1);
    qjs_bb_free(bb2);
    qjs_bb_free(bb3);
}

void test_block_predecessors() {
    BasicBlock *bb1 = qjs_bb_new(0);
    BasicBlock *bb2 = qjs_bb_new(1);
    BasicBlock *bb3 = qjs_bb_new(2);
    
    qjs_cfg_connect(bb1, bb3, CFG_EDGE_NORMAL, NULL);
    qjs_cfg_connect(bb2, bb3, CFG_EDGE_NORMAL, NULL);
    
    size_t count = 0;
    BasicBlock **predecessors = qjs_bb_predecessors(bb3, &count);
    
    ASSERT_EQ(count, 2, "bb3 should have 2 predecessors");
    ASSERT_NOT_NULL(predecessors, "Should return predecessors");
    
    free(predecessors);
    qjs_bb_free(bb1);
    qjs_bb_free(bb2);
    qjs_bb_free(bb3);
}

// ============================================================================
// 主测试运行器
// ============================================================================

int main() {
    printf("\n========== CFG Construction Tests ==========\n\n");
    
    // 基本块创建测试
    test_basic_block_creation();
    test_add_statement_to_block();
    test_cfg_edge_creation();
    test_cfg_connect_blocks();
    
    // 控制流构建测试
    test_sequential_statements_cfg();
    test_if_statement_cfg();
    test_while_loop_cfg();
    test_for_loop_cfg();
    
    // 导出功能测试
    test_cfg_to_json();
    test_cfg_to_dot();
    test_cfg_to_mermaid();
    
    // 查询测试
    test_block_successors();
    test_block_predecessors();
    
    TEST_SUMMARY();
}
