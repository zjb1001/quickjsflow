#ifndef QUICKJSFLOW_SCOPE_H
#define QUICKJSFLOW_SCOPE_H

#include <stddef.h>
#include "quickjsflow/ast.h"

typedef enum {
    SCOPE_GLOBAL = 1,
    SCOPE_MODULE,
    SCOPE_FUNCTION,
    SCOPE_BLOCK,
    SCOPE_CATCH,
    SCOPE_FOR
} ScopeType;

typedef enum {
    BIND_VAR = 1,
    BIND_LET,
    BIND_CONST,
    BIND_FUNCTION,
    BIND_PARAM,
    BIND_CATCH,
    BIND_IMPORT,
    BIND_IMPLICIT
} BindingKind;

typedef struct Scope Scope;

typedef struct Binding {
    char *name;
    BindingKind kind;
    Position loc;
    const AstNode *node;
    Scope *scope;
    struct Binding *shadowed; // nearest outer binding shadowed by this one
} Binding;

typedef struct Reference {
    char *name;
    int is_write;
    int in_tdz;
    Position loc;
    const AstNode *node;
    Binding *resolved;
    Scope *scope;
} Reference;

typedef struct {
    Binding **items;
    size_t count;
    size_t capacity;
} BindingVec;

typedef struct {
    Reference **items;
    size_t count;
    size_t capacity;
} ReferenceVec;

typedef struct {
    Scope **items;
    size_t count;
    size_t capacity;
} ScopeVec;

typedef struct ScopeMapEntry ScopeMapEntry;

struct Scope {
    ScopeType type;
    Scope *parent;
    const AstNode *node;
    BindingVec bindings;
    ReferenceVec references;
    ScopeVec children;
};

typedef struct {
    Scope *root;
    ScopeMapEntry *map;
    size_t map_count;
    size_t map_capacity;
} ScopeManager;

void scope_manager_init(ScopeManager *sm);
void scope_manager_free(ScopeManager *sm);
int scope_analyze(ScopeManager *sm, AstNode *root, int is_module);

Binding *scope_lookup_local(Scope *scope, const char *name);
Binding *scope_resolve(Scope *scope, const char *name);
Scope *scope_of_node(const ScopeManager *sm, const AstNode *node);

void scope_dump(const Scope *scope, int indent);
void scope_dump_json(const Scope *scope);

#endif
