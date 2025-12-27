#include <stdio.h>
#include <string.h>
#include "quickjsflow/parser.h"
#include "quickjsflow/ast.h"
#include "test_framework.h"

static Program *parse_prog(const char *src, AstNode **out_root) {
    Parser p; parser_init(&p, src, strlen(src));
    AstNode *root = parse_program(&p);
    if (out_root) *out_root = root;
    return (Program *)root->data;
}

static void test_switch_statement(void) {
    const char *src = "switch (x) { case 1: a(); break; default: b(); }";
    AstNode *root = NULL;
    Program *pr = parse_prog(src, &root);
    ASSERT_EQ(pr->body.count, 1, "one stmt");
    ASSERT_EQ(pr->body.items[0]->type, AST_SwitchStatement, "is switch");
    SwitchStatement *ss = (SwitchStatement *)pr->body.items[0]->data;
    ASSERT_EQ(ss->cases.count, 2, "two cases including default");
    SwitchCase *c0 = (SwitchCase *)ss->cases.items[0]->data;
    ASSERT_NOT_NULL(c0->test, "first case test present");
    SwitchCase *c1 = (SwitchCase *)ss->cases.items[1]->data;
    ASSERT_EQ(c1->test == NULL, 1, "default has null test");
    ast_free(root);
}

static void test_try_catch_finally(void) {
    const char *src = "try { foo(); } catch (e) { bar(); } finally { baz(); }";
    AstNode *root = NULL;
    Program *pr = parse_prog(src, &root);
    ASSERT_EQ(pr->body.count, 1, "one stmt");
    ASSERT_EQ(pr->body.items[0]->type, AST_TryStatement, "is try");
    TryStatement *ts = (TryStatement *)pr->body.items[0]->data;
    ASSERT_EQ(ts->handlers.count, 1, "one catch");
    ASSERT_NOT_NULL(ts->finalizer, "has finally");
    ast_free(root);
}

static void test_throw_statement(void) {
    const char *src = "throw error;";
    AstNode *root = NULL;
    Program *pr = parse_prog(src, &root);
    ASSERT_EQ(pr->body.count, 1, "one stmt");
    ASSERT_EQ(pr->body.items[0]->type, AST_ThrowStatement, "is throw");
    ast_free(root);
}

static void test_function_decl_and_expr(void) {
    const char *src = "function foo(a, b) { return a + b; }\nconst f = function(x) { return x; };";
    AstNode *root = NULL;
    Program *pr = parse_prog(src, &root);
    ASSERT_EQ(pr->body.count, 2, "two stmts");
    ASSERT_EQ(pr->body.items[0]->type, AST_FunctionDeclaration, "decl");
    FunctionBody *fb0 = (FunctionBody *)pr->body.items[0]->data;
    ASSERT_EQ(fb0->params.count, 2, "two params");
    AstNode *decl2 = pr->body.items[1];
    VariableDeclaration *vd = (VariableDeclaration *)decl2->data;
    VariableDeclarator *vdtr = (VariableDeclarator *)vd->declarations.items[0]->data;
    ASSERT_EQ(vdtr->init->type, AST_FunctionExpression, "init is func expr");
    FunctionBody *fb1 = (FunctionBody *)vdtr->init->data;
    ASSERT_EQ(fb1->params.count, 1, "one param");
    ast_free(root);
}

static void test_import_export(void) {
    const char *src = "import foo from \"mod\";\nexport { foo } from \"mod\";\nexport default foo;";
    AstNode *root = NULL;
    Program *pr = parse_prog(src, &root);
    ASSERT_EQ(pr->body.count, 3, "three stmts");
    ASSERT_EQ(pr->body.items[0]->type, AST_ImportDeclaration, "import");
    ImportDeclaration *id = (ImportDeclaration *)pr->body.items[0]->data;
    ASSERT_EQ(id->specifiers.count, 1, "one specifier");
    ASSERT_STR_EQ(id->source, "mod", "module name");
    ASSERT_EQ(pr->body.items[1]->type, AST_ExportNamedDeclaration, "export named");
    ExportNamedDeclaration *end = (ExportNamedDeclaration *)pr->body.items[1]->data;
    ASSERT_EQ(end->specifiers.count, 1, "one exported");
    ASSERT_STR_EQ(end->source, "mod", "export from");
    ASSERT_EQ(pr->body.items[2]->type, AST_ExportDefaultDeclaration, "export default");
    ast_free(root);
}

int main(void) {
    test_switch_statement();
    test_try_catch_finally();
    test_throw_statement();
    test_function_decl_and_expr();
    test_import_export();
    TEST_SUMMARY();
}
