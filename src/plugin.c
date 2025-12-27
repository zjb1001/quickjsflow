#include "quickjsflow/plugin.h"
#include <stdlib.h>
#include <string.h>

#define _GNU_SOURCE
#ifndef strdup
char *strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}
#endif

// Helper function to traverse and apply plugin visitor
static AstNode *traverse_with_plugin(AstNode *node, Plugin *plugin, PluginContext *ctx) {
    if (!node) return NULL;
    
    // Select visitor based on node type
    PluginVisitor visitor = NULL;
    
    switch (node->type) {
        case AST_Identifier:
            visitor = plugin->visit_identifier;
            break;
        case AST_Literal:
            visitor = plugin->visit_literal;
            break;
        case AST_FunctionDeclaration:
            visitor = plugin->visit_function_declaration;
            break;
        case AST_VariableDeclaration:
            visitor = plugin->visit_variable_declaration;
            break;
        case AST_ExpressionStatement:
            visitor = plugin->visit_expression_statement;
            break;
        case AST_CallExpression:
            visitor = plugin->visit_call_expression;
            break;
        case AST_MemberExpression:
            visitor = plugin->visit_member_expression;
            break;
        case AST_BinaryExpression:
            visitor = plugin->visit_binary_expression;
            break;
        case AST_UnaryExpression:
            visitor = plugin->visit_unary_expression;
            break;
        case AST_AssignmentExpression:
            visitor = plugin->visit_assignment_expression;
            break;
        case AST_IfStatement:
            visitor = plugin->visit_if_statement;
            break;
        case AST_WhileStatement:
            visitor = plugin->visit_while_statement;
            break;
        case AST_ForStatement:
            visitor = plugin->visit_for_statement;
            break;
        case AST_ReturnStatement:
            visitor = plugin->visit_return_statement;
            break;
        case AST_BlockStatement:
            visitor = plugin->visit_block_statement;
            break;
        default:
            break;
    }
    
    // Use generic visitor if no specific visitor is set
    if (!visitor) {
        visitor = plugin->visit_node;
    }
    
    // Apply visitor if present
    AstNode *result = node;
    if (visitor) {
        result = visitor(node, ctx);
        if (result != node) {
            ctx->modified = 1;
        }
        if (!result) {
            // Node was removed
            return NULL;
        }
    }
    
    // Recursively traverse children
    // This is a simplified version - full implementation would handle all node types
    switch (result->type) {
        case AST_Program: {
            Program *prog = (Program *)result->data;
            for (size_t i = 0; i < prog->body.count; i++) {
                AstNode *child = traverse_with_plugin(prog->body.items[i], plugin, ctx);
                if (!child) {
                    // Remove null children
                    for (size_t j = i; j < prog->body.count - 1; j++) {
                        prog->body.items[j] = prog->body.items[j + 1];
                    }
                    prog->body.count--;
                    i--;
                } else {
                    prog->body.items[i] = child;
                }
            }
            break;
        }
        case AST_BlockStatement: {
            BlockStatement *block = (BlockStatement *)result->data;
            for (size_t i = 0; i < block->body.count; i++) {
                AstNode *child = traverse_with_plugin(block->body.items[i], plugin, ctx);
                if (!child) {
                    for (size_t j = i; j < block->body.count - 1; j++) {
                        block->body.items[j] = block->body.items[j + 1];
                    }
                    block->body.count--;
                    i--;
                } else {
                    block->body.items[i] = child;
                }
            }
            break;
        }
        case AST_FunctionDeclaration: {
            FunctionBody *func = (FunctionBody *)result->data;
            if (func->body) {
                func->body = traverse_with_plugin(func->body, plugin, ctx);
            }
            break;
        }
        case AST_IfStatement: {
            IfStatement *ifstmt = (IfStatement *)result->data;
            if (ifstmt->test) {
                ifstmt->test = traverse_with_plugin(ifstmt->test, plugin, ctx);
            }
            if (ifstmt->consequent) {
                ifstmt->consequent = traverse_with_plugin(ifstmt->consequent, plugin, ctx);
            }
            if (ifstmt->alternate) {
                ifstmt->alternate = traverse_with_plugin(ifstmt->alternate, plugin, ctx);
            }
            break;
        }
        case AST_ExpressionStatement: {
            ExpressionStatement *expr = (ExpressionStatement *)result->data;
            if (expr->expression) {
                expr->expression = traverse_with_plugin(expr->expression, plugin, ctx);
            }
            break;
        }
        // Add more cases as needed
        default:
            break;
    }
    
    return result;
}

AstNode *plugin_apply(Plugin *plugin, AstNode *root, ScopeManager *sm) {
    if (!plugin || !root) return root;
    
    PluginContext ctx = {
        .scope_manager = sm,
        .userdata = plugin->userdata,
        .modified = 0
    };
    
    return traverse_with_plugin(root, plugin, &ctx);
}

void plugin_init(Plugin *plugin, const char *name) {
    if (!plugin) return;
    memset(plugin, 0, sizeof(Plugin));
    plugin->name = name;
}

// Built-in plugin: Remove console.log calls
static AstNode *remove_console_log_visitor(AstNode *node, void *context) {
    (void)context;
    
    if (node->type == AST_ExpressionStatement) {
        ExpressionStatement *expr = (ExpressionStatement *)node->data;
        if (expr->expression && expr->expression->type == AST_CallExpression) {
            CallExpression *call = (CallExpression *)expr->expression->data;
            if (call->callee && call->callee->type == AST_MemberExpression) {
                MemberExpression *member = (MemberExpression *)call->callee->data;
                if (member->object && member->object->type == AST_Identifier &&
                    member->property && member->property->type == AST_Identifier) {
                    Identifier *obj = (Identifier *)member->object->data;
                    Identifier *prop = (Identifier *)member->property->data;
                    if (obj->name && strcmp(obj->name, "console") == 0 &&
                        prop->name && strcmp(prop->name, "log") == 0) {
                        // Return NULL to remove this statement
                        return NULL;
                    }
                }
            }
        }
    }
    
    return node;
}

Plugin *plugin_remove_console_log(void) {
    static Plugin p;
    plugin_init(&p, "remove-console-log");
    p.description = "Remove console.log statements";
    p.visit_expression_statement = remove_console_log_visitor;
    return &p;
}

// Built-in plugin: Remove debugger statements
static AstNode *remove_debugger_visitor(AstNode *node, void *context) {
    (void)context;
    
    // Note: AST_DebuggerStatement not yet implemented in AST
    // For now, this plugin does nothing
    // TODO: Implement when DebuggerStatement is added to AST
    
    return node;
}

Plugin *plugin_remove_debugger(void) {
    static Plugin p;
    plugin_init(&p, "remove-debugger");
    p.description = "Remove debugger statements";
    p.visit_node = remove_debugger_visitor;
    return &p;
}

// Built-in plugin: Rename identifier
typedef struct {
    const char *old_name;
    const char *new_name;
} RenameData;

static AstNode *rename_identifier_visitor(AstNode *node, void *context) {
    PluginContext *ctx = (PluginContext *)context;
    RenameData *data = (RenameData *)ctx->userdata;
    
    if (node->type == AST_Identifier) {
        Identifier *id = (Identifier *)node->data;
        if (id->name && strcmp(id->name, data->old_name) == 0) {
            // Create a copy with new name
            free(id->name);
            id->name = strdup(data->new_name);
        }
    }
    
    return node;
}

Plugin *plugin_rename_identifier(const char *old_name, const char *new_name) {
    static Plugin p;
    static RenameData data;
    
    plugin_init(&p, "rename-identifier");
    p.description = "Rename identifier";
    
    data.old_name = old_name;
    data.new_name = new_name;
    
    p.visit_identifier = rename_identifier_visitor;
    p.userdata = &data;
    
    return &p;
}
