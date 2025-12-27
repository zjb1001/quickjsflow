#ifndef QUICKJSFLOW_EDIT_H
#define QUICKJSFLOW_EDIT_H

#include <stddef.h>
#include "quickjsflow/ast.h"
#include "quickjsflow/scope.h"

typedef struct {
    int code;            // 0 = success
    char message[128];   // human-friendly error message
} EditStatus;

typedef AstNode *(*EditVisitor)(AstNode *node, void *userdata);

EditStatus edit_replace(AstNode *root, const AstNode *target, AstNode *replacement, AstNode **out_root);
EditStatus edit_remove(AstNode *root, const AstNode *target, AstNode **out_root);
EditStatus edit_insert(AstNode *root, const AstNode *parent, size_t index, AstNode *node, AstNode **out_root);
EditStatus edit_move(ScopeManager *sm, AstNode *root, const AstNode *target, const AstNode *new_parent, size_t index, AstNode **out_root);
EditStatus edit_rename(ScopeManager *sm, AstNode *root, const AstNode *binding_identifier, const char *new_name, AstNode **out_root);
AstNode *edit_transform(AstNode *root, EditVisitor visitor, void *userdata);

#endif
