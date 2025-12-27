#ifndef QUICKJSFLOW_CFG_H
#define QUICKJSFLOW_CFG_H

#include <stddef.h>
#include "quickjsflow/ast.h"
#include "quickjsflow/scope.h"

/**
 * Control Flow Graph (CFG) - Issue 07
 * 
 * 构建函数/模块级控制流图，支持基本块、边类型、数据流分析。
 * 
 * 核心概念：
 * - BasicBlock: 顺序执行的语句序列，无内部分支
 * - CFGEdge: 连接基本块的边，标记类型（Normal/True/False/Exception等）
 * - CFG: 完整的控制流图，包含所有基本块与边
 * - DefUseChain: 可选的数据流分析（定义-使用链）
 */

// ============================================================================
// 边类型（Edge Types）
// ============================================================================

typedef enum {
    CFG_EDGE_NORMAL = 1,      // 顺序执行
    CFG_EDGE_TRUE,            // 条件真分支
    CFG_EDGE_FALSE,           // 条件假分支
    CFG_EDGE_EXCEPTION,       // 异常边
    CFG_EDGE_BREAK,           // break 跳转
    CFG_EDGE_CONTINUE,        // continue 跳转
    CFG_EDGE_RETURN,          // 返回边
    CFG_EDGE_CASE,            // switch case 分支
    CFG_EDGE_DEFAULT          // switch default 分支
} CFGEdgeType;

// ============================================================================
// 基本块（Basic Block）
// ============================================================================

typedef struct BasicBlock {
    int id;                        // 基本块 ID（唯一标识）
    const AstNode **statements;    // 包含的语句列表
    size_t stmt_count;
    size_t stmt_capacity;
    
    struct CFGEdge **outgoing;     // 出边列表
    size_t out_count;
    size_t out_capacity;
    
    struct CFGEdge **incoming;     // 入边列表
    size_t in_count;
    size_t in_capacity;
    
    int is_entry;                  // 是否为入口块
    int is_exit;                   // 是否为出口块
    int unreachable;               // 是否不可达（死代码）
} BasicBlock;

// ============================================================================
// CFG 边（Edge）
// ============================================================================

typedef struct CFGEdge {
    CFGEdgeType type;              // 边类型
    BasicBlock *from;              // 源基本块
    BasicBlock *to;                // 目标基本块
    const AstNode *condition;      // 条件表达式（用于条件边）
} CFGEdge;

// ============================================================================
// 定义-使用链（Def-Use Chain）- 可选
// ============================================================================

typedef struct DefUseNode {
    const AstNode *node;           // 定义/使用的 AST 节点
    BasicBlock *block;             // 所在基本块
    int is_def;                    // 是否为定义（0=使用，1=定义）
    const char *var_name;          // 变量名
    struct DefUseNode **uses;      // 使用点列表（若为定义）
    size_t use_count;
    size_t use_capacity;
} DefUseNode;

typedef struct {
    DefUseNode **items;
    size_t count;
    size_t capacity;
} DefUseChain;

// ============================================================================
// 控制流图（CFG）
// ============================================================================

typedef struct CFG {
    BasicBlock **blocks;           // 所有基本块
    size_t block_count;
    size_t block_capacity;
    
    CFGEdge **edges;               // 所有边
    size_t edge_count;
    size_t edge_capacity;
    
    BasicBlock *entry;             // 入口块
    BasicBlock *exit;              // 出口块
    
    const AstNode *function;       // 关联的函数节点
    Scope *scope;                  // 关联的作用域（可选）
    
    DefUseChain *def_use;          // 定义-使用链（可选）
} CFG;

// ============================================================================
// CFG 构建选项
// ============================================================================

typedef struct {
    int build_def_use;             // 是否构建 Def-Use 链（需要 Scope）
    int remove_unreachable;        // 是否移除不可达代码块
    int simplify;                  // 是否简化 CFG（合并单入单出块）
} CFGOptions;

// ============================================================================
// 公共 API
// ============================================================================

/**
 * 为函数构建 CFG
 * @param func_node - FunctionDeclaration/FunctionExpression/ArrowFunctionExpression
 * @param scope - 关联的作用域（用于 Def-Use 分析，可为 NULL）
 * @param options - 构建选项（可为 NULL，使用默认值）
 * @return CFG 指针，使用后需调用 qjs_cfg_free() 释放
 */
CFG *qjs_build_cfg(const AstNode *func_node, Scope *scope, const CFGOptions *options);

/**
 * 为程序顶层构建 CFG（模块级别）
 * @param program_node - Program 节点
 * @param scope - 关联的作用域（可为 NULL）
 * @param options - 构建选项（可为 NULL）
 * @return CFG 指针
 */
CFG *qjs_build_cfg_toplevel(const AstNode *program_node, Scope *scope, const CFGOptions *options);

/**
 * 释放 CFG 资源
 */
void qjs_cfg_free(CFG *cfg);

// ============================================================================
// 基本块操作
// ============================================================================

/**
 * 创建新基本块
 */
BasicBlock *qjs_bb_new(int id);

/**
 * 添加语句到基本块
 */
void qjs_bb_add_stmt(BasicBlock *bb, const AstNode *stmt);

/**
 * 释放基本块
 */
void qjs_bb_free(BasicBlock *bb);

// ============================================================================
// 边操作
// ============================================================================

/**
 * 创建 CFG 边
 */
CFGEdge *qjs_cfg_edge_new(CFGEdgeType type, BasicBlock *from, BasicBlock *to, const AstNode *condition);

/**
 * 连接两个基本块
 */
void qjs_cfg_connect(BasicBlock *from, BasicBlock *to, CFGEdgeType type, const AstNode *condition);

/**
 * 释放边
 */
void qjs_cfg_edge_free(CFGEdge *edge);

// ============================================================================
// 查询与遍历
// ============================================================================

/**
 * 获取后继基本块
 */
BasicBlock **qjs_bb_successors(const BasicBlock *bb, size_t *count);

/**
 * 获取前驱基本块
 */
BasicBlock **qjs_bb_predecessors(const BasicBlock *bb, size_t *count);

/**
 * DFS 遍历 CFG
 * @param cfg - 控制流图
 * @param visitor - 访问回调函数
 * @param user_data - 用户数据
 */
typedef void (*CFGVisitor)(BasicBlock *bb, void *user_data);
void qjs_cfg_dfs(CFG *cfg, CFGVisitor visitor, void *user_data);

// ============================================================================
// 导出功能
// ============================================================================

/**
 * 导出为 JSON 格式
 * @param cfg - 控制流图
 * @return JSON 字符串（需调用 free() 释放）
 */
char *qjs_cfg_to_json(const CFG *cfg);

/**
 * 导出为 DOT 格式（Graphviz）
 * @param cfg - 控制流图
 * @return DOT 格式字符串（需调用 free() 释放）
 */
char *qjs_cfg_to_dot(const CFG *cfg);

/**
 * 导出为 Mermaid 格式（可选）
 * @param cfg - 控制流图
 * @return Mermaid 格式字符串（需调用 free() 释放）
 */
char *qjs_cfg_to_mermaid(const CFG *cfg);

// ============================================================================
// 数据流分析（可选）
// ============================================================================

/**
 * 构建 Def-Use 链
 * @param cfg - 控制流图（需已关联 Scope）
 * @return DefUseChain 指针
 */
DefUseChain *qjs_build_def_use_chain(CFG *cfg);

/**
 * 活跃变量分析
 * @param cfg - 控制流图
 * @param bb - 基本块
 * @return 活跃变量名列表（需调用 free() 释放）
 */
char **qjs_live_variables(const CFG *cfg, const BasicBlock *bb, size_t *count);

/**
 * 释放 Def-Use 链
 */
void qjs_def_use_chain_free(DefUseChain *chain);

#endif // QUICKJSFLOW_CFG_H
