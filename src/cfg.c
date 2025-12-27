#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quickjsflow/cfg.h"
#include "quickjsflow/ast.h"

// ============================================================================
// 内部辅助函数声明
// ============================================================================

static void cfg_build_statements(CFG *cfg, const AstVec *stmts, BasicBlock *entry, BasicBlock *exit);
static BasicBlock *cfg_build_statement(CFG *cfg, const AstNode *stmt, BasicBlock *current, BasicBlock *exit);
static void cfg_add_block(CFG *cfg, BasicBlock *block);
static void cfg_add_edge(CFG *cfg, BasicBlock *from, BasicBlock *to, CFGEdgeType type, const AstNode *condition);
static const char *edge_type_string(CFGEdgeType type);
static const char *ast_type_string(AstNodeType type);

// ============================================================================
// 基本块创建与操作
// ============================================================================

BasicBlock *qjs_bb_new(int id) {
    BasicBlock *bb = (BasicBlock *)malloc(sizeof(BasicBlock));
    if (!bb) return NULL;
    
    bb->id = id;
    bb->statements = NULL;
    bb->stmt_count = 0;
    bb->stmt_capacity = 0;
    
    bb->outgoing = NULL;
    bb->out_count = 0;
    bb->out_capacity = 0;
    
    bb->incoming = NULL;
    bb->in_count = 0;
    bb->in_capacity = 0;
    
    bb->is_entry = 0;
    bb->is_exit = 0;
    bb->unreachable = 0;
    
    return bb;
}

void qjs_bb_add_stmt(BasicBlock *bb, const AstNode *stmt) {
    if (!bb || !stmt) return;
    
    if (bb->stmt_count >= bb->stmt_capacity) {
        bb->stmt_capacity = bb->stmt_capacity == 0 ? 16 : bb->stmt_capacity * 2;
        AstNode **new_stmts = (AstNode **)realloc(bb->statements, bb->stmt_capacity * sizeof(AstNode *));
        if (!new_stmts) return;
        bb->statements = new_stmts;
    }
    
    bb->statements[bb->stmt_count++] = stmt;
}

void qjs_bb_free(BasicBlock *bb) {
    if (!bb) return;
    free(bb->statements);
    free(bb->outgoing);
    free(bb->incoming);
    free(bb);
}

// ============================================================================
// CFG 边创建与操作
// ============================================================================

CFGEdge *qjs_cfg_edge_new(CFGEdgeType type, BasicBlock *from, BasicBlock *to, const AstNode *condition) {
    CFGEdge *edge = (CFGEdge *)malloc(sizeof(CFGEdge));
    if (!edge) return NULL;
    
    edge->type = type;
    edge->from = from;
    edge->to = to;
    edge->condition = condition;
    
    return edge;
}

void qjs_cfg_edge_free(CFGEdge *edge) {
    if (edge) free(edge);
}

void qjs_cfg_connect(BasicBlock *from, BasicBlock *to, CFGEdgeType type, const AstNode *condition) {
    if (!from || !to) return;
    
    // 添加出边
    if (from->out_count >= from->out_capacity) {
        from->out_capacity = from->out_capacity == 0 ? 8 : from->out_capacity * 2;
        CFGEdge **new_edges = (CFGEdge **)realloc(from->outgoing, from->out_capacity * sizeof(CFGEdge *));
        if (!new_edges) return;
        from->outgoing = new_edges;
    }
    
    CFGEdge *edge = qjs_cfg_edge_new(type, from, to, condition);
    if (edge) {
        from->outgoing[from->out_count++] = edge;
    }
    
    // 添加入边
    if (to->in_count >= to->in_capacity) {
        to->in_capacity = to->in_capacity == 0 ? 8 : to->in_capacity * 2;
        CFGEdge **new_edges = (CFGEdge **)realloc(to->incoming, to->in_capacity * sizeof(CFGEdge *));
        if (!new_edges) return;
        to->incoming = new_edges;
    }
    
    to->incoming[to->in_count++] = edge;
}

// ============================================================================
// CFG 创建与操作
// ============================================================================

static void cfg_add_block(CFG *cfg, BasicBlock *block) {
    if (!cfg || !block) return;
    
    if (cfg->block_count >= cfg->block_capacity) {
        cfg->block_capacity = cfg->block_capacity == 0 ? 16 : cfg->block_capacity * 2;
        BasicBlock **new_blocks = (BasicBlock **)realloc(cfg->blocks, cfg->block_capacity * sizeof(BasicBlock *));
        if (!new_blocks) return;
        cfg->blocks = new_blocks;
    }
    
    cfg->blocks[cfg->block_count++] = block;
}

static void cfg_add_edge(CFG *cfg, BasicBlock *from, BasicBlock *to, CFGEdgeType type, const AstNode *condition) {
    if (!cfg || !from || !to) return;
    
    if (cfg->edge_count >= cfg->edge_capacity) {
        cfg->edge_capacity = cfg->edge_capacity == 0 ? 32 : cfg->edge_capacity * 2;
        CFGEdge **new_edges = (CFGEdge **)realloc(cfg->edges, cfg->edge_capacity * sizeof(CFGEdge *));
        if (!new_edges) return;
        cfg->edges = new_edges;
    }
    
    CFGEdge *edge = qjs_cfg_edge_new(type, from, to, condition);
    if (edge) {
        cfg->edges[cfg->edge_count++] = edge;
        
        // 同时更新块的出入边
        if (from->out_count >= from->out_capacity) {
            from->out_capacity = from->out_capacity == 0 ? 8 : from->out_capacity * 2;
            CFGEdge **new_out = (CFGEdge **)realloc(from->outgoing, from->out_capacity * sizeof(CFGEdge *));
            if (!new_out) return;
            from->outgoing = new_out;
        }
        from->outgoing[from->out_count++] = edge;
        
        if (to->in_count >= to->in_capacity) {
            to->in_capacity = to->in_capacity == 0 ? 8 : to->in_capacity * 2;
            CFGEdge **new_in = (CFGEdge **)realloc(to->incoming, to->in_capacity * sizeof(CFGEdge *));
            if (!new_in) return;
            to->incoming = new_in;
        }
        to->incoming[to->in_count++] = edge;
    }
}

// ============================================================================
// CFG 构建逻辑
// ============================================================================

/**
 * 检查是否为分支终结语句
 */
// 注：is_branching_statement 函数当前未使用，保留以供后续扩展
// static int is_branching_statement(const AstNode *stmt) {
//     if (!stmt) return 0;
//     switch (stmt->type) {
//         case AST_IfStatement:
//         case AST_WhileStatement:
//         case AST_DoWhileStatement:
//         case AST_ForStatement:
//         case AST_SwitchStatement:
//         case AST_TryStatement:
//         case AST_ReturnStatement:
//         case AST_ThrowStatement:
//         case AST_BreakStatement:
//         case AST_ContinueStatement:
//             return 1;
//         default:
//             return 0;
//     }
// }

/**
 * 构建单个语句的 CFG
 * 返回语句执行完毕后应继续的基本块
 */
static BasicBlock *cfg_build_statement(CFG *cfg, const AstNode *stmt, BasicBlock *current, BasicBlock *exit) {
    if (!stmt || !current) return current;
    
    qjs_bb_add_stmt(current, stmt);
    
    switch (stmt->type) {
        case AST_BlockStatement: {
            // 递归处理块内语句
            BlockStatement *block = (BlockStatement *)stmt->data;
            if (block && block->body.count > 0) {
                BasicBlock *block_end = current;
                for (size_t i = 0; i < block->body.count; i++) {
                    block_end = cfg_build_statement(cfg, block->body.items[i], block_end, exit);
                }
                return block_end;
            }
            return current;
        }
        
        case AST_IfStatement: {
            IfStatement *if_stmt = (IfStatement *)stmt->data;
            
            // 创建 then 分支块
            BasicBlock *then_block = qjs_bb_new(cfg->block_count);
            cfg_add_block(cfg, then_block);
            cfg_add_edge(cfg, current, then_block, CFG_EDGE_TRUE, if_stmt->test);
            
            // 创建 else 分支块（如果有）
            BasicBlock *else_block = NULL;
            if (if_stmt->alternate) {
                else_block = qjs_bb_new(cfg->block_count);
                cfg_add_block(cfg, else_block);
                cfg_add_edge(cfg, current, else_block, CFG_EDGE_FALSE, if_stmt->test);
            } else {
                // 没有 else，false 分支直接连接到后续块
                else_block = qjs_bb_new(cfg->block_count);
                cfg_add_block(cfg, else_block);
                cfg_add_edge(cfg, current, else_block, CFG_EDGE_FALSE, if_stmt->test);
            }
            
            // 创建汇合块（分支之后）
            BasicBlock *merge_block = qjs_bb_new(cfg->block_count);
            cfg_add_block(cfg, merge_block);
            
            // 递归处理 then 分支
            BasicBlock *then_end = cfg_build_statement(cfg, if_stmt->consequent, then_block, exit);
            if (then_end) {
                cfg_add_edge(cfg, then_end, merge_block, CFG_EDGE_NORMAL, NULL);
            }
            
            // 递归处理 else 分支
            if (if_stmt->alternate) {
                BasicBlock *else_end = cfg_build_statement(cfg, if_stmt->alternate, else_block, exit);
                if (else_end) {
                    cfg_add_edge(cfg, else_end, merge_block, CFG_EDGE_NORMAL, NULL);
                }
            } else {
                cfg_add_edge(cfg, else_block, merge_block, CFG_EDGE_NORMAL, NULL);
            }
            
            return merge_block;
        }
        
        case AST_WhileStatement: {
            WhileStatement *while_stmt = (WhileStatement *)stmt->data;
            
            // 创建循环体块
            BasicBlock *loop_block = qjs_bb_new(cfg->block_count);
            cfg_add_block(cfg, loop_block);
            cfg_add_edge(cfg, current, loop_block, CFG_EDGE_TRUE, while_stmt->test);
            
            // 创建退出块
            BasicBlock *exit_block = qjs_bb_new(cfg->block_count);
            cfg_add_block(cfg, exit_block);
            cfg_add_edge(cfg, current, exit_block, CFG_EDGE_FALSE, while_stmt->test);
            
            // 递归处理循环体
            BasicBlock *loop_end = cfg_build_statement(cfg, while_stmt->body, loop_block, exit_block);
            if (loop_end) {
                cfg_add_edge(cfg, loop_end, current, CFG_EDGE_CONTINUE, NULL);
            }
            
            return exit_block;
        }
        
        case AST_DoWhileStatement: {
            DoWhileStatement *do_while = (DoWhileStatement *)stmt->data;
            
            // 创建循环体块
            BasicBlock *loop_block = qjs_bb_new(cfg->block_count);
            cfg_add_block(cfg, loop_block);
            cfg_add_edge(cfg, current, loop_block, CFG_EDGE_NORMAL, NULL);
            
            // 创建退出块
            BasicBlock *exit_block = qjs_bb_new(cfg->block_count);
            cfg_add_block(cfg, exit_block);
            
            // 递归处理循环体
            BasicBlock *loop_end = cfg_build_statement(cfg, do_while->body, loop_block, exit_block);
            if (loop_end) {
                cfg_add_edge(cfg, loop_end, current, CFG_EDGE_TRUE, do_while->test);
                cfg_add_edge(cfg, loop_end, exit_block, CFG_EDGE_FALSE, do_while->test);
            }
            
            return exit_block;
        }
        
        case AST_ForStatement: {
            ForStatement *for_stmt = (ForStatement *)stmt->data;
            
            // 创建循环体块
            BasicBlock *loop_block = qjs_bb_new(cfg->block_count);
            cfg_add_block(cfg, loop_block);
            
            BasicBlock *test_block = current;
            if (for_stmt->test) {
                cfg_add_edge(cfg, test_block, loop_block, CFG_EDGE_TRUE, for_stmt->test);
            } else {
                cfg_add_edge(cfg, test_block, loop_block, CFG_EDGE_NORMAL, NULL);
            }
            
            // 创建退出块
            BasicBlock *exit_block = qjs_bb_new(cfg->block_count);
            cfg_add_block(cfg, exit_block);
            if (for_stmt->test) {
                cfg_add_edge(cfg, test_block, exit_block, CFG_EDGE_FALSE, for_stmt->test);
            }
            
            // 递归处理循环体
            BasicBlock *loop_end = cfg_build_statement(cfg, for_stmt->body, loop_block, exit_block);
            if (loop_end) {
                cfg_add_edge(cfg, loop_end, test_block, CFG_EDGE_CONTINUE, NULL);
            }
            
            return exit_block;
        }
        
        case AST_SwitchStatement: {
            SwitchStatement *switch_stmt = (SwitchStatement *)stmt->data;
            
            // 创建退出块
            BasicBlock *exit_block = qjs_bb_new(cfg->block_count);
            cfg_add_block(cfg, exit_block);
            
            BasicBlock *prev_case = current;
            for (size_t i = 0; i < switch_stmt->cases.count; i++) {
                SwitchCase *case_node = (SwitchCase *)switch_stmt->cases.items[i]->data;
                
                BasicBlock *case_block = qjs_bb_new(cfg->block_count);
                cfg_add_block(cfg, case_block);
                
                CFGEdgeType edge_type = case_node->test ? CFG_EDGE_CASE : CFG_EDGE_DEFAULT;
                cfg_add_edge(cfg, prev_case, case_block, edge_type, case_node->test);
                
                // 处理 case 内的语句
                BasicBlock *case_end = case_block;
                for (size_t j = 0; j < case_node->consequent.count; j++) {
                    case_end = cfg_build_statement(cfg, case_node->consequent.items[j], case_end, exit_block);
                }
                
                prev_case = case_end;
            }
            
            if (prev_case) {
                cfg_add_edge(cfg, prev_case, exit_block, CFG_EDGE_NORMAL, NULL);
            }
            
            return exit_block;
        }
        
        case AST_TryStatement: {
            TryStatement *try_stmt = (TryStatement *)stmt->data;
            if (!try_stmt) return current;
            
            // 创建 try 块
            BasicBlock *try_block = qjs_bb_new(cfg->block_count);
            cfg_add_block(cfg, try_block);
            cfg_add_edge(cfg, current, try_block, CFG_EDGE_NORMAL, NULL);
            
            // 创建汇合块
            BasicBlock *merge_block = qjs_bb_new(cfg->block_count);
            cfg_add_block(cfg, merge_block);
            
            // 处理 try 块（BlockStatement）
            BasicBlock *try_end = try_block;
            if (try_stmt->block) {
                BlockStatement *try_body = (BlockStatement *)try_stmt->block->data;
                if (try_body && try_body->body.count > 0) {
                    for (size_t i = 0; i < try_body->body.count; i++) {
                        try_end = cfg_build_statement(cfg, try_body->body.items[i], try_end, merge_block);
                    }
                }
            }
            if (try_end) {
                cfg_add_edge(cfg, try_end, merge_block, CFG_EDGE_NORMAL, NULL);
            }
            
            // 处理 catch 块（如果有）
            if (try_stmt->handlers.count > 0) {
                for (size_t h = 0; h < try_stmt->handlers.count; h++) {
                    AstNode *handler_node = try_stmt->handlers.items[h];
                    CatchClause *catch_clause = (CatchClause *)handler_node->data;
                    
                    BasicBlock *catch_block = qjs_bb_new(cfg->block_count);
                    cfg_add_block(cfg, catch_block);
                    cfg_add_edge(cfg, try_block, catch_block, CFG_EDGE_EXCEPTION, NULL);
                    
                    // 处理 catch 块内的语句（BlockStatement）
                    BasicBlock *catch_end = catch_block;
                    if (catch_clause && catch_clause->body) {
                        BlockStatement *catch_body = (BlockStatement *)catch_clause->body->data;
                        if (catch_body && catch_body->body.count > 0) {
                            for (size_t i = 0; i < catch_body->body.count; i++) {
                                catch_end = cfg_build_statement(cfg, catch_body->body.items[i], catch_end, merge_block);
                            }
                        }
                    }
                    if (catch_end) {
                        cfg_add_edge(cfg, catch_end, merge_block, CFG_EDGE_NORMAL, NULL);
                    }
                }
            }
            
            // 处理 finally 块（如果有）
            if (try_stmt->finalizer) {
                BasicBlock *finally_block = qjs_bb_new(cfg->block_count);
                cfg_add_block(cfg, finally_block);
                cfg_add_edge(cfg, merge_block, finally_block, CFG_EDGE_NORMAL, NULL);
                
                // 处理 finally 块内的语句（BlockStatement）
                BasicBlock *finally_end = finally_block;
                BlockStatement *finally_body = (BlockStatement *)try_stmt->finalizer->data;
                if (finally_body && finally_body->body.count > 0) {
                    for (size_t i = 0; i < finally_body->body.count; i++) {
                        finally_end = cfg_build_statement(cfg, finally_body->body.items[i], finally_end, NULL);
                    }
                }
                return finally_end;
            }
            
            return merge_block;
        }
        
        case AST_ReturnStatement:
        case AST_ThrowStatement:
        case AST_BreakStatement:
        case AST_ContinueStatement: {
            // 这些是终结语句，不继续到下一个块
            if (exit) {
                cfg_add_edge(cfg, current, exit, CFG_EDGE_RETURN, NULL);
            }
            return NULL;
        }
        
        case AST_ExpressionStatement:
        case AST_VariableDeclaration:
        default:
            // 普通语句，继续到下一个
            return current;
    }
}

/**
 * 构建语句序列的 CFG
 */
static void cfg_build_statements(CFG *cfg, const AstVec *stmts, BasicBlock *entry, BasicBlock *exit) {
    if (!stmts || stmts->count == 0) return;
    
    BasicBlock *current = entry;
    for (size_t i = 0; i < stmts->count; i++) {
        if (!current) break;
        current = cfg_build_statement(cfg, stmts->items[i], current, exit);
    }
    
    // 最后的语句块连接到出口
    if (current && exit) {
        cfg_add_edge(cfg, current, exit, CFG_EDGE_NORMAL, NULL);
    }
}

CFG *qjs_build_cfg(const AstNode *func_node, Scope *scope, const CFGOptions *options) {
    (void)options;  // 未使用的参数
    if (!func_node) return NULL;
    
    CFG *cfg = (CFG *)malloc(sizeof(CFG));
    if (!cfg) return NULL;
    
    cfg->blocks = NULL;
    cfg->block_count = 0;
    cfg->block_capacity = 0;
    
    cfg->edges = NULL;
    cfg->edge_count = 0;
    cfg->edge_capacity = 0;
    
    cfg->function = func_node;
    cfg->scope = scope;
    cfg->def_use = NULL;
    
    // 创建入口块和出口块
    BasicBlock *entry = qjs_bb_new(0);
    entry->is_entry = 1;
    cfg_add_block(cfg, entry);
    cfg->entry = entry;
    
    BasicBlock *exit = qjs_bb_new(1);
    exit->is_exit = 1;
    cfg_add_block(cfg, exit);
    cfg->exit = exit;
    
    // 获取函数体
    AstVec *body = NULL;
    switch (func_node->type) {
        case AST_FunctionDeclaration:
        case AST_FunctionExpression: {
            FunctionBody *func = (FunctionBody *)func_node->data;
            if (func && func->body) {
                BlockStatement *block_stmt = (BlockStatement *)func->body->data;
                if (block_stmt) {
                    body = &block_stmt->body;
                }
            }
            break;
        }
        default:
            qjs_cfg_free(cfg);
            return NULL;
    }
    
    if (body && body->count > 0) {
        cfg_build_statements(cfg, body, entry, exit);
    } else {
        cfg_add_edge(cfg, entry, exit, CFG_EDGE_NORMAL, NULL);
    }
    
    return cfg;
}

CFG *qjs_build_cfg_toplevel(const AstNode *program_node, Scope *scope, const CFGOptions *options) {
    (void)options;  // 未使用的参数
    if (!program_node || program_node->type != AST_Program) return NULL;
    
    CFG *cfg = (CFG *)malloc(sizeof(CFG));
    if (!cfg) return NULL;
    
    cfg->blocks = NULL;
    cfg->block_count = 0;
    cfg->block_capacity = 0;
    
    cfg->edges = NULL;
    cfg->edge_count = 0;
    cfg->edge_capacity = 0;
    
    cfg->function = program_node;
    cfg->scope = scope;
    cfg->def_use = NULL;
    
    // 创建入口块和出口块
    BasicBlock *entry = qjs_bb_new(0);
    entry->is_entry = 1;
    cfg_add_block(cfg, entry);
    cfg->entry = entry;
    
    BasicBlock *exit = qjs_bb_new(1);
    exit->is_exit = 1;
    cfg_add_block(cfg, exit);
    cfg->exit = exit;
    
    // 获取程序体
    Program *program = (Program *)program_node->data;
    cfg_build_statements(cfg, &program->body, entry, exit);
    
    return cfg;
}

void qjs_cfg_free(CFG *cfg) {
    if (!cfg) return;
    
    for (size_t i = 0; i < cfg->block_count; i++) {
        qjs_bb_free(cfg->blocks[i]);
    }
    free(cfg->blocks);
    
    for (size_t i = 0; i < cfg->edge_count; i++) {
        qjs_cfg_edge_free(cfg->edges[i]);
    }
    free(cfg->edges);
    
    if (cfg->def_use) {
        qjs_def_use_chain_free(cfg->def_use);
    }
    
    free(cfg);
}

// ============================================================================
// 查询与遍历
// ============================================================================

BasicBlock **qjs_bb_successors(const BasicBlock *bb, size_t *count) {
    if (!bb || !count) return NULL;
    
    *count = bb->out_count;
    if (*count == 0) return NULL;
    
    BasicBlock **successors = (BasicBlock **)malloc(*count * sizeof(BasicBlock *));
    if (!successors) return NULL;
    
    for (size_t i = 0; i < *count; i++) {
        successors[i] = bb->outgoing[i]->to;
    }
    
    return successors;
}

BasicBlock **qjs_bb_predecessors(const BasicBlock *bb, size_t *count) {
    if (!bb || !count) return NULL;
    
    *count = bb->in_count;
    if (*count == 0) return NULL;
    
    BasicBlock **predecessors = (BasicBlock **)malloc(*count * sizeof(BasicBlock *));
    if (!predecessors) return NULL;
    
    for (size_t i = 0; i < *count; i++) {
        predecessors[i] = bb->incoming[i]->from;
    }
    
    return predecessors;
}

void qjs_cfg_dfs(CFG *cfg, CFGVisitor visitor, void *user_data) {
    if (!cfg || !visitor) return;
    
    // 标记已访问的块
    char *visited = (char *)calloc(cfg->block_count, sizeof(char));
    if (!visited) return;
    
    // 使用栈进行 DFS
    BasicBlock **stack = (BasicBlock **)malloc(cfg->block_count * sizeof(BasicBlock *));
    if (!stack) {
        free(visited);
        return;
    }
    
    int top = 0;
    stack[top++] = cfg->entry;
    visited[cfg->entry->id] = 1;
    
    while (top > 0) {
        BasicBlock *bb = stack[--top];
        visitor(bb, user_data);
        
        for (size_t i = 0; i < bb->out_count; i++) {
            BasicBlock *successor = bb->outgoing[i]->to;
            if (!visited[successor->id]) {
                visited[successor->id] = 1;
                stack[top++] = successor;
            }
        }
    }
    
    free(stack);
    free(visited);
}

// ============================================================================
// 导出功能
// ============================================================================

static const char *edge_type_string(CFGEdgeType type) {
    switch (type) {
        case CFG_EDGE_NORMAL: return "normal";
        case CFG_EDGE_TRUE: return "true";
        case CFG_EDGE_FALSE: return "false";
        case CFG_EDGE_EXCEPTION: return "exception";
        case CFG_EDGE_BREAK: return "break";
        case CFG_EDGE_CONTINUE: return "continue";
        case CFG_EDGE_RETURN: return "return";
        case CFG_EDGE_CASE: return "case";
        case CFG_EDGE_DEFAULT: return "default";
        default: return "unknown";
    }
}

static const char *ast_type_string(AstNodeType type) {
    switch (type) {
        case AST_ExpressionStatement: return "ExpressionStatement";
        case AST_VariableDeclaration: return "VariableDeclaration";
        case AST_IfStatement: return "IfStatement";
        case AST_WhileStatement: return "WhileStatement";
        case AST_DoWhileStatement: return "DoWhileStatement";
        case AST_ForStatement: return "ForStatement";
        case AST_SwitchStatement: return "SwitchStatement";
        case AST_TryStatement: return "TryStatement";
        case AST_ReturnStatement: return "ReturnStatement";
        case AST_ThrowStatement: return "ThrowStatement";
        case AST_BreakStatement: return "BreakStatement";
        case AST_ContinueStatement: return "ContinueStatement";
        case AST_BlockStatement: return "BlockStatement";
        default: return "Unknown";
    }
}

char *qjs_cfg_to_json(const CFG *cfg) {
    if (!cfg) return NULL;
    
    // 估算缓冲区大小
    size_t buf_size = 4096;
    char *buf = (char *)malloc(buf_size);
    if (!buf) return NULL;
    
    size_t pos = 0;
    
    // 开始 JSON
    pos += snprintf(buf + pos, buf_size - pos, "{\n  \"blocks\": [\n");
    
    // 导出基本块
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *bb = cfg->blocks[i];
        
        // 检查缓冲区大小
        if (pos + 512 > buf_size) {
            buf_size *= 2;
            char *new_buf = (char *)realloc(buf, buf_size);
            if (!new_buf) {
                free(buf);
                return NULL;
            }
            buf = new_buf;
        }
        
        pos += snprintf(buf + pos, buf_size - pos, 
            "    {\"id\": %d, \"entry\": %d, \"exit\": %d, \"stmts\": %zu}%s\n",
            bb->id, bb->is_entry, bb->is_exit, bb->stmt_count,
            i < cfg->block_count - 1 ? "," : "");
    }
    
    pos += snprintf(buf + pos, buf_size - pos, "  ],\n  \"edges\": [\n");
    
    // 导出边
    for (size_t i = 0; i < cfg->edge_count; i++) {
        CFGEdge *edge = cfg->edges[i];
        
        // 检查缓冲区大小
        if (pos + 512 > buf_size) {
            buf_size *= 2;
            char *new_buf = (char *)realloc(buf, buf_size);
            if (!new_buf) {
                free(buf);
                return NULL;
            }
            buf = new_buf;
        }
        
        pos += snprintf(buf + pos, buf_size - pos,
            "    {\"from\": %d, \"to\": %d, \"type\": \"%s\"}%s\n",
            edge->from->id, edge->to->id, edge_type_string(edge->type),
            i < cfg->edge_count - 1 ? "," : "");
    }
    
    pos += snprintf(buf + pos, buf_size - pos, "  ]\n}\n");
    
    return buf;
}

char *qjs_cfg_to_dot(const CFG *cfg) {
    if (!cfg) return NULL;
    
    size_t buf_size = 8192;
    char *buf = (char *)malloc(buf_size);
    if (!buf) return NULL;
    
    size_t pos = 0;
    
    // 开始 DOT 文件
    pos += snprintf(buf + pos, buf_size - pos, "digraph CFG {\n");
    pos += snprintf(buf + pos, buf_size - pos, "  rankdir=TB;\n");
    pos += snprintf(buf + pos, buf_size - pos, "  node [shape=box];\n\n");
    
    // 定义节点
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *bb = cfg->blocks[i];
        
        if (pos + 256 > buf_size) {
            buf_size *= 2;
            char *new_buf = (char *)realloc(buf, buf_size);
            if (!new_buf) {
                free(buf);
                return NULL;
            }
            buf = new_buf;
        }
        
        const char *shape = bb->is_entry ? "ellipse" : (bb->is_exit ? "box" : "box");
        const char *style = bb->unreachable ? "dashed" : "solid";
        
        pos += snprintf(buf + pos, buf_size - pos,
            "  bb%d [label=\"BB%d (%zu stmts)\", shape=%s, style=%s];\n",
            bb->id, bb->id, bb->stmt_count, shape, style);
    }
    
    pos += snprintf(buf + pos, buf_size - pos, "\n");
    
    // 定义边
    for (size_t i = 0; i < cfg->edge_count; i++) {
        CFGEdge *edge = cfg->edges[i];
        
        if (pos + 256 > buf_size) {
            buf_size *= 2;
            char *new_buf = (char *)realloc(buf, buf_size);
            if (!new_buf) {
                free(buf);
                return NULL;
            }
            buf = new_buf;
        }
        
        const char *color = "black";
        if (edge->type == CFG_EDGE_TRUE) color = "green";
        else if (edge->type == CFG_EDGE_FALSE) color = "red";
        else if (edge->type == CFG_EDGE_EXCEPTION) color = "orange";
        
        pos += snprintf(buf + pos, buf_size - pos,
            "  bb%d -> bb%d [label=\"%s\", color=%s];\n",
            edge->from->id, edge->to->id, edge_type_string(edge->type), color);
    }
    
    pos += snprintf(buf + pos, buf_size - pos, "}\n");
    
    return buf;
}

char *qjs_cfg_to_mermaid(const CFG *cfg) {
    if (!cfg) return NULL;
    
    size_t buf_size = 8192;
    char *buf = (char *)malloc(buf_size);
    if (!buf) return NULL;
    
    size_t pos = 0;
    
    // 开始 Mermaid 图表
    pos += snprintf(buf + pos, buf_size - pos, "flowchart TD\n");
    
    // 定义节点
    for (size_t i = 0; i < cfg->block_count; i++) {
        BasicBlock *bb = cfg->blocks[i];
        
        if (pos + 256 > buf_size) {
            buf_size *= 2;
            char *new_buf = (char *)realloc(buf, buf_size);
            if (!new_buf) {
                free(buf);
                return NULL;
            }
            buf = new_buf;
        }
        
        const char *label;
        if (bb->is_entry) {
            label = "Entry";
        } else if (bb->is_exit) {
            label = "Exit";
        } else {
            label = "Basic Block";
        }
        
        pos += snprintf(buf + pos, buf_size - pos, "    bb%d[\"%s %d\"]\n", bb->id, label, bb->id);
    }
    
    pos += snprintf(buf + pos, buf_size - pos, "\n");
    
    // 定义边
    for (size_t i = 0; i < cfg->edge_count; i++) {
        CFGEdge *edge = cfg->edges[i];
        
        if (pos + 256 > buf_size) {
            buf_size *= 2;
            char *new_buf = (char *)realloc(buf, buf_size);
            if (!new_buf) {
                free(buf);
                return NULL;
            }
            buf = new_buf;
        }
        
        const char *label = edge_type_string(edge->type);
        pos += snprintf(buf + pos, buf_size - pos,
            "    bb%d -->|%s| bb%d\n",
            edge->from->id, label, edge->to->id);
    }
    
    return buf;
}

// ============================================================================
// 数据流分析（可选）
// ============================================================================

DefUseChain *qjs_build_def_use_chain(CFG *cfg) {
    if (!cfg) return NULL;
    
    DefUseChain *chain = (DefUseChain *)malloc(sizeof(DefUseChain));
    if (!chain) return NULL;
    
    chain->items = NULL;
    chain->count = 0;
    chain->capacity = 0;
    
    // TODO: 实现 Def-Use 链构建
    // 需要遍历 CFG 的所有基本块和语句，追踪变量的定义与使用
    
    return chain;
}

char **qjs_live_variables(const CFG *cfg, const BasicBlock *bb, size_t *count) {
    if (!cfg || !bb || !count) return NULL;
    
    *count = 0;
    // TODO: 实现活跃变量分析
    
    return NULL;
}

void qjs_def_use_chain_free(DefUseChain *chain) {
    if (!chain) return;
    
    for (size_t i = 0; i < chain->count; i++) {
        free(chain->items[i]->uses);
        free(chain->items[i]);
    }
    free(chain->items);
    free(chain);
}
