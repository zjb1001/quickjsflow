#ifndef QUICKJSFLOW_MODULE_INTERFACES_H
#define QUICKJSFLOW_MODULE_INTERFACES_H

/**
 * @file module_interfaces.h
 * @brief Module Interface Contracts - Defines API boundaries between components
 * 
 * This file formalizes the contracts between different modules in quickjsflow,
 * ensuring that each module can be developed and tested independently while
 * maintaining compatibility with downstream consumers.
 * 
 * Version: 1.0.0
 * Last Updated: 2025-12
 */

#include <stddef.h>
#include "quickjsflow/lexer.h"
#include "quickjsflow/parser.h"
#include "quickjsflow/ast.h"
#include "quickjsflow/scope.h"
#include "quickjsflow/codegen.h"

/* ============================================================================
 * MODULE 1: Lexer → Parser Interface (TokenStream)
 * ============================================================================
 * 
 * Contract: Lexer produces Token stream, Parser consumes it
 * Versioning: Lexer version 1.0, TokenType enum is fixed
 * Stability: Lexer API is stable; new token types added with backward compat
 */

typedef struct {
    Lexer *lexer;
    Token lookahead;
    int has_lookahead;
} TokenStream;

/**
 * Initialize a token stream from source text
 * @param stream: Output token stream structure
 * @param input: Source code string
 * @param length: Length of source code
 * @return: 0 on success, non-zero on error
 */
int tokenstream_init(TokenStream *stream, const char *input, size_t length);

/**
 * Get next token from stream (advances position)
 * @param stream: Token stream
 * @return: Next token, or TOKEN_EOF at end
 */
Token tokenstream_next(TokenStream *stream);

/**
 * Peek at next token without consuming it
 * @param stream: Token stream
 * @return: Next token without advancing
 */
Token tokenstream_peek(TokenStream *stream);

/**
 * Check if stream has reached EOF
 * @param stream: Token stream
 * @return: 1 if at EOF, 0 otherwise
 */
int tokenstream_is_eof(TokenStream *stream);

void tokenstream_free(TokenStream *stream);

/* ============================================================================
 * MODULE 2: Parser → ScopeManager Interface (AST Structure)
 * ============================================================================
 * 
 * Contract: Parser produces AST with complete node information
 * Versioning: AST version 1.0 (ESTree compatible)
 * Stability: New node types use AST_* enum; node layout is immutable
 * 
 * Invariants:
 * - Every AstNode has valid type, start, end positions
 * - Parent pointers are set consistently (or NULL for root)
 * - All node data matches its type (no type confusion)
 * - Comments preserved in source order
 */

typedef struct {
    int version_major;  // 1
    int version_minor;  // 0
    int patch;          // 0
    const char *estree_version;  // "5.0" or higher
} ASTVersion;

/**
 * Get AST interface version
 * @return: Version structure
 */
ASTVersion ast_get_version(void);

/**
 * Verify AST structural integrity
 * @param node: AST node to verify
 * @return: 1 if valid, 0 if invalid
 * 
 * Checks:
 * - Node type is valid (within AST_* enum range)
 * - Position information is consistent (start <= end)
 * - Data pointer is non-NULL (unless allowed to be NULL)
 * - All children are valid nodes
 */
int ast_verify(const AstNode *node);

/**
 * Recursively verify entire AST tree
 * @param root: Root program node
 * @param error_count: Output parameter for error count
 * @return: 1 if all nodes valid, 0 otherwise
 */
int ast_verify_tree(const AstNode *root, int *error_count);

/**
 * Clone an AST node (structural sharing with reference counting)
 * @param node: Node to clone
 * @return: New reference to node (increments refcount)
 */
AstNode *ast_clone(const AstNode *node);

/**
 * Release an AST node reference
 * @param node: Node to release
 * 
 * Decrements refcount; only frees when refcount reaches 0
 */
void ast_release(AstNode *node);

/* ============================================================================
 * MODULE 3: ScopeManager → Edit API Interface (Scope Query Contract)
 * ============================================================================
 * 
 * Contract: ScopeManager analyzes AST and provides scope/binding queries
 * Versioning: Scope interface version 1.0
 * Stability: Query API is read-only; no modifications to scopes
 * 
 * Invariants:
 * - All bindings are resolved or marked as unresolved
 * - Scope chain is complete (every scope has parent or is root)
 * - No circular dependencies in scope hierarchy
 */

typedef struct {
    int version_major;  // 1
    int version_minor;  // 0
} ScopeInterfaceVersion;

ScopeInterfaceVersion scope_interface_version(void);

/**
 * Resolve a variable reference in a scope
 * @param scope: Starting scope for lookup
 * @param name: Variable name to resolve
 * @return: Binding if found, NULL otherwise
 * 
 * Uses normal scope chain resolution (scope → parent → ... → global)
 */
Binding *scope_resolve_name(Scope *scope, const char *name);

/**
 * Get all bindings visible from a scope
 * @param scope: Scope to query
 * @return: Vector of all visible bindings
 * 
 * Includes bindings from scope chain, with shadowing resolved
 */
BindingVec scope_get_all_visible(Scope *scope);

/**
 * Get scope for a specific AST node
 * @param sm: Scope manager
 * @param node: AST node
 * @return: Scope object, NULL if not found
 */
Scope *scope_manager_get_scope(const ScopeManager *sm, const AstNode *node);

/**
 * Get parent scope
 * @param scope: Current scope
 * @return: Parent scope, NULL if global
 */
Scope *scope_get_parent(const Scope *scope);

/**
 * Get all child scopes
 * @param scope: Parent scope
 * @return: Vector of child scopes
 */
ScopeVec scope_get_children(const Scope *scope);

/* ============================================================================
 * MODULE 4: Edit API → Codegen Interface (Immutability Contract)
 * ============================================================================
 * 
 * Contract: All edit operations return new AST copy; original unchanged
 * Versioning: Edit API version 1.0
 * Stability: All modifications preserve AST node IDs and structure
 * 
 * Invariants:
 * - Every edit operation returns a new AST (original untouched)
 * - Modified nodes have new refcount=1
 * - Parent pointers updated in modified subtree
 * - Comments preserve original positions
 */

typedef struct {
    int version_major;  // 1
    int version_minor;  // 0
} EditInterfaceVersion;

EditInterfaceVersion edit_interface_version(void);

/**
 * Edit operations should follow this pattern (not implemented in MVP)
 * 
 * // Replace a node with a new one
 * AstNode *ast_replace_node(const AstNode *parent, 
 *                           const AstNode *old_child,
 *                           const AstNode *new_child);
 * 
 * // Remove a node
 * AstNode *ast_remove_node(const AstNode *parent,
 *                          const AstNode *child);
 * 
 * // Insert a node
 * AstNode *ast_insert_node(const AstNode *parent,
 *                          size_t index,
 *                          const AstNode *new_child);
 */

/* ============================================================================
 * MODULE 5: Codegen Interface
 * ============================================================================
 * 
 * Contract: Codegen accepts verified AST and produces JavaScript source
 * Versioning: Codegen version 1.0
 * Stability: Output format is deterministic (same AST → same code)
 * 
 * Invariants:
 * - All identifiers preserved (names unchanged)
 * - Literals preserve raw values (no semantic changes)
 * - Comments preserved in output (with proper formatting)
 * - Source map (if enabled) is valid JSON and maps all nodes
 */

typedef struct {
    int version_major;  // 1
    int version_minor;  // 0
} CodegenInterfaceVersion;

CodegenInterfaceVersion codegen_interface_version(void);

/**
 * Generate code with full result details
 * @param root: AST Program node
 * @param options: Generation options (or NULL for defaults)
 * @return: Result with code and optionally source map
 */
CodegenResult codegen_generate_with_version(const AstNode *root,
                                            const CodegenOptions *options);

/**
 * Validate that generated code is well-formed
 * @param code: Generated code string
 * @return: 1 if valid JS syntax, 0 otherwise (best effort)
 */
int codegen_verify_output(const char *code);

/* ============================================================================
 * End-to-End Pipeline Interface
 * ============================================================================
 * 
 * Complete pipeline: Source → Lexer → Parser → Scope → Codegen → Source'
 */

typedef struct {
    TokenStream *tokens;
    AstNode *ast;
    ScopeManager *scopes;
    CodegenResult generated;
    int pipeline_version;
} PipelineState;

/**
 * Execute full pipeline from source to generated code
 * @param source: JavaScript source code
 * @param length: Length of source
 * @param options: Code generation options
 * @return: Pipeline state (caller must free)
 */
PipelineState *pipeline_execute(const char *source, size_t length,
                                 const CodegenOptions *options);

void pipeline_state_free(PipelineState *state);

#endif // QUICKJSFLOW_MODULE_INTERFACES_H
