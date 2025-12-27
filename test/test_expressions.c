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

static void test_object_and_array_literals(void) {
    const char *src = "let a = { x: 1, y: 2 };\nlet arr = [1,,3];";
    AstNode *root = NULL;
    Program *pr = parse_prog(src, &root);

    ASSERT_EQ(pr->body.count, 2, "two statements parsed");

    // let a = { x: 1, y: 2 };
    AstNode *decl1 = pr->body.items[0];
    ASSERT_EQ(decl1->type, AST_VariableDeclaration, "first is var decl");
    VariableDeclaration *vd1 = (VariableDeclaration *)decl1->data;
    ASSERT_EQ(vd1->kind, VD_Let, "first decl kind let");
    ASSERT_EQ(vd1->declarations.count, 1, "one declarator");
    VariableDeclarator *d1 = (VariableDeclarator *)vd1->declarations.items[0]->data;
    ASSERT_EQ(d1->init->type, AST_ObjectExpression, "init is object literal");
    ObjectExpression *obj = (ObjectExpression *)d1->init->data;
    ASSERT_EQ(obj->properties.count, 2, "object has 2 props");
    Property *p0 = (Property *)obj->properties.items[0]->data;
    Property *p1 = (Property *)obj->properties.items[1]->data;
    Identifier *k0 = (Identifier *)p0->key->data;
    Identifier *k1 = (Identifier *)p1->key->data;
    ASSERT_STR_EQ(k0->name, "x", "first key x");
    ASSERT_STR_EQ(k1->name, "y", "second key y");

    // let arr = [1,,3];
    AstNode *decl2 = pr->body.items[1];
    ASSERT_EQ(decl2->type, AST_VariableDeclaration, "second is var decl");
    VariableDeclaration *vd2 = (VariableDeclaration *)decl2->data;
    VariableDeclarator *d2 = (VariableDeclarator *)vd2->declarations.items[0]->data;
    ASSERT_EQ(d2->init->type, AST_ArrayExpression, "init is array literal");
    ArrayExpression *arr = (ArrayExpression *)d2->init->data;
    ASSERT_EQ(arr->elements.count, 3, "array has 3 slots (with hole)");
    ASSERT_NOT_NULL(arr->elements.items[0], "first element present");
    ASSERT_EQ(arr->elements.items[1] == NULL, 1, "second element is hole");
    ASSERT_NOT_NULL(arr->elements.items[2], "third element present");

    ast_free(root);
}

static void test_member_call_assignment(void) {
    const char *src = "obj.foo = bar();";
    AstNode *root = NULL;
    Program *pr = parse_prog(src, &root);

    ASSERT_EQ(pr->body.count, 1, "one statement parsed");
    AstNode *stmt = pr->body.items[0];
    ASSERT_EQ(stmt->type, AST_ExpressionStatement, "is expression statement");
    ExpressionStatement *es = (ExpressionStatement *)stmt->data;
    ASSERT_EQ(es->expression->type, AST_AssignmentExpression, "root is assignment");
    AssignmentExpression *ae = (AssignmentExpression *)es->expression->data;
    ASSERT_STR_EQ(ae->operator, "=", "assignment operator");
    ASSERT_EQ(ae->left->type, AST_MemberExpression, "lhs is member");
    MemberExpression *me = (MemberExpression *)ae->left->data;
    ASSERT_EQ(me->computed, 0, "dot access");
    ASSERT_EQ(me->property->type, AST_Identifier, "property is identifier");
    ASSERT_EQ(ae->right->type, AST_CallExpression, "rhs is call");
    CallExpression *ce = (CallExpression *)ae->right->data;
    ASSERT_EQ(ce->arguments.count, 0, "call has no args");

    ast_free(root);
}

static void test_unary_update_binary_precedence(void) {
    const char *src = "x++ + 2 * -y;";
    AstNode *root = NULL;
    Program *pr = parse_prog(src, &root);

    ASSERT_EQ(pr->body.count, 1, "one statement parsed");
    ExpressionStatement *es = (ExpressionStatement *)pr->body.items[0]->data;
    ASSERT_EQ(es->expression->type, AST_BinaryExpression, "root is binary");
    BinaryExpression *add = (BinaryExpression *)es->expression->data;
    ASSERT_STR_EQ(add->operator, "+", "outer op plus");

    ASSERT_EQ(add->left->type, AST_UpdateExpression, "left is postfix update");
    UpdateExpression *ue = (UpdateExpression *)add->left->data;
    ASSERT_EQ(ue->prefix, 0, "postfix");

    ASSERT_EQ(add->right->type, AST_BinaryExpression, "right is binary mul");
    BinaryExpression *mul = (BinaryExpression *)add->right->data;
    ASSERT_STR_EQ(mul->operator, "*", "inner op mul");
    ASSERT_EQ(mul->left->type, AST_Literal, "mul left literal");
    ASSERT_EQ(mul->right->type, AST_UnaryExpression, "mul right unary");
    UnaryExpression *un = (UnaryExpression *)mul->right->data;
    ASSERT_STR_EQ(un->operator, "-", "unary minus");

    ast_free(root);
}

int main(void) {
    test_object_and_array_literals();
    test_member_call_assignment();
    test_unary_update_binary_precedence();
    TEST_SUMMARY();
}
