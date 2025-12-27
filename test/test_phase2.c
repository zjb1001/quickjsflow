#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quickjsflow/parser.h"
#include "quickjsflow/ast.h"
#include "quickjsflow/codegen.h"

static int test_count = 0;
static int test_passed = 0;

#define TEST(name) \
    printf("\n[TEST] %s\n", name); \
    test_count++;

#define ASSERT(cond, msg) \
    if (!(cond)) { \
        printf("  ❌ FAIL: %s\n", msg); \
        return 0; \
    } else { \
        printf("  ✓ PASS: %s\n", msg); \
    }

#define TEST_END \
    test_passed++; \
    return 1;

// Test helper: parse and validate
static AstNode *parse_code(const char *code) {
    Parser p;
    parser_init(&p, code, strlen(code));
    AstNode *ast = parse_program(&p);
    return ast;
}

// Phase 2 Tests

int test_for_of_loop() {
    TEST("for-of loop");
    const char *code = "for (const x of arr) { console.log(x); }";
    AstNode *ast = parse_code(code);
    ASSERT(ast != NULL, "Parse for-of");
    ASSERT(ast->type == AST_Program, "Root is Program");
    
    Program *prog = (Program *)ast->data;
    ASSERT(prog->body.count > 0, "Has statements");
    
    AstNode *stmt = prog->body.items[0];
    ASSERT(stmt->type == AST_ForOfStatement, "Statement is ForOfStatement");
    
    ast_free(ast);
    TEST_END;
}

int test_for_in_loop() {
    TEST("for-in loop");
    const char *code = "for (const key in obj) { console.log(key); }";
    AstNode *ast = parse_code(code);
    ASSERT(ast != NULL, "Parse for-in");
    ASSERT(ast->type == AST_Program, "Root is Program");
    
    Program *prog = (Program *)ast->data;
    ASSERT(prog->body.count > 0, "Has statements");
    
    AstNode *stmt = prog->body.items[0];
    ASSERT(stmt->type == AST_ForInStatement, "Statement is ForInStatement");
    
    ast_free(ast);
    TEST_END;
}

int test_template_literal() {
    TEST("template literal");
    const char *code = "`hello world`";
    AstNode *ast = parse_code(code);
    ASSERT(ast != NULL, "Parse template literal");
    ASSERT(ast->type == AST_Program, "Root is Program");
    
    Program *prog = (Program *)ast->data;
    ASSERT(prog->body.count > 0, "Has statements");
    
    AstNode *stmt = prog->body.items[0];
    ASSERT(stmt->type == AST_ExpressionStatement, "Statement is ExpressionStatement");
    
    ExpressionStatement *es = (ExpressionStatement *)stmt->data;
    ASSERT(es->expression != NULL, "Has expression");
    ASSERT(es->expression->type == AST_TemplateLiteral, "Expression is TemplateLiteral");
    
    ast_free(ast);
    TEST_END;
}

int test_class_declaration() {
    TEST("class declaration");
    const char *code = "class Foo { }";
    AstNode *ast = parse_code(code);
    ASSERT(ast != NULL, "Parse class");
    ASSERT(ast->type == AST_Program, "Root is Program");
    
    Program *prog = (Program *)ast->data;
    ASSERT(prog->body.count > 0, "Has statements");
    
    AstNode *stmt = prog->body.items[0];
    ASSERT(stmt->type == AST_ClassDeclaration, "Statement is ClassDeclaration");
    
    ClassDeclaration *cd = (ClassDeclaration *)stmt->data;
    ASSERT(cd->id != NULL, "Class has id");
    ASSERT(cd->id->type == AST_Identifier, "Class id is Identifier");
    
    ast_free(ast);
    TEST_END;
}

int test_class_with_extends() {
    TEST("class with extends");
    const char *code = "class Bar extends Foo { }";
    AstNode *ast = parse_code(code);
    ASSERT(ast != NULL, "Parse class with extends");
    ASSERT(ast->type == AST_Program, "Root is Program");
    
    Program *prog = (Program *)ast->data;
    ASSERT(prog->body.count > 0, "Has statements");
    
    AstNode *stmt = prog->body.items[0];
    ASSERT(stmt->type == AST_ClassDeclaration, "Statement is ClassDeclaration");
    
    ClassDeclaration *cd = (ClassDeclaration *)stmt->data;
    ASSERT(cd->superClass != NULL, "Class has superClass");
    
    ast_free(ast);
    TEST_END;
}

int test_this_expression() {
    TEST("this expression");
    const char *code = "this.value = 42;";
    AstNode *ast = parse_code(code);
    ASSERT(ast != NULL, "Parse this expression");
    ASSERT(ast->type == AST_Program, "Root is Program");
    
    Program *prog = (Program *)ast->data;
    ASSERT(prog->body.count > 0, "Has statements");
    
    AstNode *stmt = prog->body.items[0];
    ASSERT(stmt->type == AST_ExpressionStatement, "Statement is ExpressionStatement");
    
    ExpressionStatement *es = (ExpressionStatement *)stmt->data;
    ASSERT(es->expression->type == AST_AssignmentExpression, "Expression is AssignmentExpression");
    
    AssignmentExpression *ae = (AssignmentExpression *)es->expression->data;
    ASSERT(ae->left->type == AST_MemberExpression, "Left is MemberExpression");
    
    MemberExpression *me = (MemberExpression *)ae->left->data;
    ASSERT(me->object->type == AST_ThisExpression, "Object is ThisExpression");
    
    ast_free(ast);
    TEST_END;
}

int test_super_expression() {
    TEST("super expression");
    const char *code = "super.method();";
    AstNode *ast = parse_code(code);
    ASSERT(ast != NULL, "Parse super expression");
    ASSERT(ast->type == AST_Program, "Root is Program");
    
    Program *prog = (Program *)ast->data;
    ASSERT(prog->body.count > 0, "Has statements");
    
    AstNode *stmt = prog->body.items[0];
    ASSERT(stmt->type == AST_ExpressionStatement, "Statement is ExpressionStatement");
    
    ExpressionStatement *es = (ExpressionStatement *)stmt->data;
    ASSERT(es->expression->type == AST_CallExpression, "Expression is CallExpression");
    
    CallExpression *ce = (CallExpression *)es->expression->data;
    ASSERT(ce->callee->type == AST_MemberExpression, "Callee is MemberExpression");
    
    MemberExpression *me = (MemberExpression *)ce->callee->data;
    ASSERT(me->object->type == AST_Super, "Object is Super");
    
    ast_free(ast);
    TEST_END;
}

int test_ast_node_constructors() {
    TEST("Phase 2 AST node constructors");
    
    Position s = {1, 1};
    Position e = {1, 10};
    
    // Arrow function
    AstNode *arrow = ast_arrow_function_expression(0, s, e);
    ASSERT(arrow != NULL, "Create arrow function");
    ASSERT(arrow->type == AST_ArrowFunctionExpression, "Arrow function type");
    ast_free(arrow);
    
    // Template literal
    AstNode *tpl = ast_template_literal(s, e);
    ASSERT(tpl != NULL, "Create template literal");
    ASSERT(tpl->type == AST_TemplateLiteral, "Template literal type");
    ast_free(tpl);
    
    // Spread element
    AstNode *id = ast_identifier("x", s, e);
    AstNode *spread = ast_spread_element(id, s, e);
    ASSERT(spread != NULL, "Create spread element");
    ASSERT(spread->type == AST_SpreadElement, "Spread element type");
    ast_free(spread);
    
    // Rest element
    AstNode *id2 = ast_identifier("y", s, e);
    AstNode *rest = ast_rest_element(id2, s, e);
    ASSERT(rest != NULL, "Create rest element");
    ASSERT(rest->type == AST_RestElement, "Rest element type");
    ast_free(rest);
    
    // For-of statement
    AstNode *left = ast_identifier("item", s, e);
    AstNode *right = ast_identifier("array", s, e);
    AstNode *body = ast_block_statement(s, e);
    AstNode *forof = ast_for_of_statement(left, right, body, s, e);
    ASSERT(forof != NULL, "Create for-of");
    ASSERT(forof->type == AST_ForOfStatement, "For-of type");
    ast_free(forof);
    
    // For-in statement
    AstNode *left2 = ast_identifier("key", s, e);
    AstNode *right2 = ast_identifier("obj", s, e);
    AstNode *body2 = ast_block_statement(s, e);
    AstNode *forin = ast_for_in_statement(left2, right2, body2, s, e);
    ASSERT(forin != NULL, "Create for-in");
    ASSERT(forin->type == AST_ForInStatement, "For-in type");
    ast_free(forin);
    
    // Class declaration
    AstNode *classId = ast_identifier("MyClass", s, e);
    AstNode *classDecl = ast_class_declaration(classId, NULL, s, e);
    ASSERT(classDecl != NULL, "Create class declaration");
    ASSERT(classDecl->type == AST_ClassDeclaration, "Class declaration type");
    ast_free(classDecl);
    
    // This expression
    AstNode *thisExpr = ast_this_expression(s, e);
    ASSERT(thisExpr != NULL, "Create this expression");
    ASSERT(thisExpr->type == AST_ThisExpression, "This expression type");
    ast_free(thisExpr);
    
    // Super
    AstNode *superExpr = ast_super(s, e);
    ASSERT(superExpr != NULL, "Create super");
    ASSERT(superExpr->type == AST_Super, "Super type");
    ast_free(superExpr);
    
    TEST_END;
}

int main() {
    printf("======================\n");
    printf("Phase 2 Feature Tests\n");
    printf("======================\n");
    
    test_for_of_loop();
    test_for_in_loop();
    test_template_literal();
    test_class_declaration();
    test_class_with_extends();
    test_this_expression();
    test_super_expression();
    test_ast_node_constructors();
    
    printf("\n======================\n");
    printf("Results: %d/%d tests passed\n", test_passed, test_count);
    printf("======================\n");
    
    return (test_passed == test_count) ? 0 : 1;
}
