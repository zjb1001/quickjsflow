#ifndef QUICKJSFLOW_PLUGIN_H
#define QUICKJSFLOW_PLUGIN_H

#include "quickjsflow/ast.h"
#include "quickjsflow/scope.h"

// Plugin visitor callback type
// Returns a node (can be original, modified copy, or new node)
// Return NULL to remove the node from the tree
typedef AstNode *(*PluginVisitor)(AstNode *node, void *context);

// Plugin context passed to visitor functions
typedef struct {
    ScopeManager *scope_manager;  // optional, can be NULL
    void *userdata;               // plugin-specific data
    int modified;                 // set to 1 if tree was modified
} PluginContext;

// Plugin definition structure
typedef struct {
    const char *name;
    const char *description;
    
    // Visitor callbacks for different node types
    // Each can be NULL if not needed
    PluginVisitor visit_identifier;
    PluginVisitor visit_literal;
    PluginVisitor visit_function_declaration;
    PluginVisitor visit_variable_declaration;
    PluginVisitor visit_expression_statement;
    PluginVisitor visit_call_expression;
    PluginVisitor visit_member_expression;
    PluginVisitor visit_binary_expression;
    PluginVisitor visit_unary_expression;
    PluginVisitor visit_assignment_expression;
    PluginVisitor visit_if_statement;
    PluginVisitor visit_while_statement;
    PluginVisitor visit_for_statement;
    PluginVisitor visit_return_statement;
    PluginVisitor visit_block_statement;
    
    // Generic visitor - called for all nodes if specific visitor is not set
    PluginVisitor visit_node;
    
    void *userdata;  // plugin-specific data
} Plugin;

// Plugin API functions
AstNode *plugin_apply(Plugin *plugin, AstNode *root, ScopeManager *sm);
void plugin_init(Plugin *plugin, const char *name);

// Built-in example plugins
Plugin *plugin_remove_console_log(void);
Plugin *plugin_rename_identifier(const char *old_name, const char *new_name);
Plugin *plugin_remove_debugger(void);

#endif
