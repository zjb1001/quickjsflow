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

static void test_if_else(void) {
    const char *src = "if (x) { y = 1; } else y = 2;";
    AstNode *root = NULL;
    Program *pr = parse_prog(src, &root);

    ASSERT_EQ(pr->body.count, 1, "one statement");
    AstNode *stmt = pr->body.items[0];
    ASSERT_EQ(stmt->type, AST_IfStatement, "is if statement");
    IfStatement *ifs = (IfStatement *)stmt->data;
    ASSERT_NOT_NULL(ifs->test, "test present");
    ASSERT_EQ(ifs->consequent->type, AST_BlockStatement, "consequent block");
    ASSERT_NOT_NULL(ifs->alternate, "has else");

    ast_free(root);
}

static void test_while_and_do_while(void) {
    const char *src = "while (n < 10) n++;\ndo { n--; } while (n > 0);";
    AstNode *root = NULL;
    Program *pr = parse_prog(src, &root);
    ASSERT_EQ(pr->body.count, 2, "two statements");
    ASSERT_EQ(pr->body.items[0]->type, AST_WhileStatement, "first while");
    ASSERT_EQ(pr->body.items[1]->type, AST_DoWhileStatement, "second do-while");
    ast_free(root);
}

static void test_for_with_init_and_update(void) {
    const char *src = "for (let i = 0; i < 3; i++) { sum += i; }";
    AstNode *root = NULL;
    Program *pr = parse_prog(src, &root);
    ASSERT_EQ(pr->body.count, 1, "one statement");
    AstNode *stmt = pr->body.items[0];
    ASSERT_EQ(stmt->type, AST_ForStatement, "is for");
    ForStatement *fs = (ForStatement *)stmt->data;
    ASSERT_NOT_NULL(fs->init, "init present");
    ASSERT_NOT_NULL(fs->test, "test present");
    ASSERT_NOT_NULL(fs->update, "update present");
    ASSERT_EQ(fs->body->type, AST_BlockStatement, "body block");
    ast_free(root);
}

static void test_return_break_continue(void) {
    const char *src = "{ return 1; break; continue; }";
    AstNode *root = NULL;
    Program *pr = parse_prog(src, &root);
    ASSERT_EQ(pr->body.count, 1, "wrapper block");
    BlockStatement *bs = (BlockStatement *)pr->body.items[0]->data;
    ASSERT_EQ(bs->body.count, 3, "three inner statements");
    ASSERT_EQ(bs->body.items[0]->type, AST_ReturnStatement, "return");
    ASSERT_EQ(bs->body.items[1]->type, AST_BreakStatement, "break");
    ASSERT_EQ(bs->body.items[2]->type, AST_ContinueStatement, "continue");
    ast_free(root);
}

int main(void) {
    test_if_else();
    test_while_and_do_while();
    test_for_with_init_and_update();
    test_return_break_continue();
    TEST_SUMMARY();
}
