#include <stdlib.h>
#include <string.h>
#include "quickjsflow/parser.h"
#include "quickjsflow/edit.h"
#include "quickjsflow/scope.h"
#include "test_framework.h"

static AstNode *parse_source(const char *src) {
    Parser p; parser_init(&p, src, strlen(src));
    return parse_program(&p);
}

static const char *id_name(AstNode *id) {
    if (!id || id->type != AST_Identifier) return "";
    Identifier *i = (Identifier *)id->data;
    return i && i->name ? i->name : "";
}

static void test_replace_literal(void) {
    AstNode *root = parse_source("var a = 1; var b = 2;");
    Program *pr = (Program *)root->data;
    AstNode *var_b = pr->body.items[1];
    VariableDeclaration *vd = (VariableDeclaration *)var_b->data;
    VariableDeclarator *decl = (VariableDeclarator *)vd->declarations.items[0]->data;
    AstNode *lit_two = decl->init;

    AstNode *replacement = ast_literal(LIT_Number, "3", (Position){0,0}, (Position){0,0});
    AstNode *new_root = NULL;
    EditStatus st = edit_replace(root, lit_two, replacement, &new_root);
    ASSERT_EQ(st.code, 0, "replace succeeds");

    Program *new_pr = (Program *)new_root->data;
    VariableDeclaration *new_vd = (VariableDeclaration *)new_pr->body.items[1]->data;
    VariableDeclarator *new_decl = (VariableDeclarator *)new_vd->declarations.items[0]->data;
    Literal *lit = (Literal *)new_decl->init->data;
    ASSERT_STR_EQ(lit->raw, "3", "literal replaced to 3");

    Literal *old_lit = (Literal *)decl->init->data;
    ASSERT_STR_EQ(old_lit->raw, "2", "original tree untouched");

    ast_free(replacement);
    ast_free(root);
    ast_free(new_root);
}

static void test_remove_statement(void) {
    AstNode *root = parse_source("var a = 1; var b = 2;");
    Program *pr = (Program *)root->data;
    AstNode *second = pr->body.items[1];
    AstNode *new_root = NULL;
    EditStatus st = edit_remove(root, second, &new_root);
    ASSERT_EQ(st.code, 0, "remove succeeds");

    Program *new_pr = (Program *)new_root->data;
    ASSERT_EQ(new_pr->body.count, 1, "body shrinks to one statement");

    ast_free(root);
    ast_free(new_root);
}

static void test_insert_statement(void) {
    AstNode *root = parse_source("var a = 1;");
    AstNode *new_decl = ast_variable_declaration(VD_Var);
    VariableDeclaration *vd = (VariableDeclaration *)new_decl->data;
    AstNode *id = ast_identifier("b", (Position){0,0}, (Position){0,0});
    AstNode *lit = ast_literal(LIT_Number, "2", (Position){0,0}, (Position){0,0});
    AstNode *decl = ast_variable_declarator(id, lit);
    astvec_push(&vd->declarations, decl);

    AstNode *parent = root; // program is the parent
    AstNode *new_root = NULL;
    EditStatus st = edit_insert(root, parent, 0, new_decl, &new_root);
    ASSERT_EQ(st.code, 0, "insert succeeds");

    Program *new_pr = (Program *)new_root->data;
    ASSERT_EQ(new_pr->body.count, 2, "body grows");
    VariableDeclaration *first_vd = (VariableDeclaration *)new_pr->body.items[0]->data;
    VariableDeclarator *first_decl = (VariableDeclarator *)first_vd->declarations.items[0]->data;
    ASSERT_STR_EQ(id_name(first_decl->id), "b", "new decl is first");

    ast_free(new_decl);
    ast_free(root);
    ast_free(new_root);
}

static void test_rename_conflict_shadow(void) {
    AstNode *root = parse_source("let x = 1; { let y = x; }");
    Program *pr = (Program *)root->data;
    AstNode *decl_stmt = pr->body.items[0];
    VariableDeclaration *vd = (VariableDeclaration *)decl_stmt->data;
    VariableDeclarator *decl = (VariableDeclarator *)vd->declarations.items[0]->data;
    AstNode *binding_id = decl->id;

    ScopeManager sm; scope_manager_init(&sm);
    scope_analyze(&sm, root, 0);

    AstNode *new_root = NULL;
    EditStatus st = edit_rename(&sm, root, binding_id, "y", &new_root);
    ASSERT_EQ(st.code == 0, 0, "rename rejected due to capture");

    scope_manager_free(&sm);
    ast_free(root);
    if (new_root) ast_free(new_root);
}

static void test_rename_updates_references(void) {
    AstNode *root = parse_source("function f(){ let x = 1; x = x + 1; }");
    Program *pr = (Program *)root->data;
    AstNode *fn = pr->body.items[0];
    FunctionBody *fb = (FunctionBody *)fn->data;
    BlockStatement *body = (BlockStatement *)fb->body->data;
    AstNode *let_stmt = body->body.items[0];
    VariableDeclaration *vd = (VariableDeclaration *)let_stmt->data;
    VariableDeclarator *decl = (VariableDeclarator *)vd->declarations.items[0]->data;
    AstNode *binding_id = decl->id;

    ScopeManager sm; scope_manager_init(&sm);
    scope_analyze(&sm, root, 0);

    AstNode *new_root = NULL;
    EditStatus st = edit_rename(&sm, root, binding_id, "y", &new_root);
    ASSERT_EQ(st.code, 0, "rename succeeds");

    Program *npr = (Program *)new_root->data;
    FunctionBody *nfb = (FunctionBody *)npr->body.items[0]->data;
    BlockStatement *nbody = (BlockStatement *)nfb->body->data;
    VariableDeclaration *nvd = (VariableDeclaration *)nbody->body.items[0]->data;
    VariableDeclarator *ndecl = (VariableDeclarator *)nvd->declarations.items[0]->data;
    ASSERT_STR_EQ(id_name(ndecl->id), "y", "binding renamed");

    AstNode *assign_stmt = nbody->body.items[1];
    ExpressionStatement *es = (ExpressionStatement *)assign_stmt->data;
    AssignmentExpression *ae = (AssignmentExpression *)es->expression->data;
    ASSERT_STR_EQ(id_name(ae->left), "y", "write reference renamed");
    BinaryExpression *be = (BinaryExpression *)ae->right->data;
    ASSERT_STR_EQ(id_name(be->left), "y", "read reference renamed");

    scope_manager_free(&sm);
    ast_free(root);
    ast_free(new_root);
}

static void test_move_detects_capture(void) {
    const char *src = "let a = 1; function f(){ return a; } { let a = 2; }";
    AstNode *root = parse_source(src);
    Program *pr = (Program *)root->data;
    AstNode *fn = pr->body.items[1];
    AstNode *block = pr->body.items[2];

    ScopeManager sm; scope_manager_init(&sm);
    scope_analyze(&sm, root, 0);

    AstNode *new_root = NULL;
    EditStatus st = edit_move(&sm, root, fn, block, 0, &new_root);
    ASSERT_EQ(st.code == 0, 0, "move rejected due to capture");

    scope_manager_free(&sm);
    ast_free(root);
    if (new_root) ast_free(new_root);
}

int main(void) {
    test_replace_literal();
    test_remove_statement();
    test_insert_statement();
    test_rename_conflict_shadow();
    test_rename_updates_references();
    test_move_detects_capture();
    TEST_SUMMARY();
}
