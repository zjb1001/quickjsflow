#include <stdlib.h>
#include <string.h>
#include "quickjsflow/lexer.h"
#include "quickjsflow/parser.h"
#include "quickjsflow/ast.h"
#include "quickjsflow/codegen.h"
#include "test_framework.h"

static int str_eq(const char *a, const char *b) {
    if (!a || !b) return a == b;
    return strcmp(a, b) == 0;
}

// Round-trip test: parse → print → parse again, check structure equivalence
static int ast_nodes_equal(const AstNode *a, const AstNode *b) {
    if (!a && !b) return 1;
    if (!a || !b) return 0;
    if (a->type != b->type) return 0;
    switch (a->type) {
        case AST_Program: {
            Program *pa = (Program *)a->data;
            Program *pb = (Program *)b->data;
            if (pa->body.count != pb->body.count) return 0;
            for (size_t i = 0; i < pa->body.count; ++i) {
                if (!ast_nodes_equal(pa->body.items[i], pb->body.items[i])) return 0;
            }
            return 1;
        }
        case AST_VariableDeclaration: {
            VariableDeclaration *va = (VariableDeclaration *)a->data;
            VariableDeclaration *vb = (VariableDeclaration *)b->data;
            if (va->kind != vb->kind) return 0;
            if (va->declarations.count != vb->declarations.count) return 0;
            for (size_t i = 0; i < va->declarations.count; ++i) {
                if (!ast_nodes_equal(va->declarations.items[i], vb->declarations.items[i])) return 0;
            }
            return 1;
        }
        case AST_VariableDeclarator: {
            VariableDeclarator *da = (VariableDeclarator *)a->data;
            VariableDeclarator *db = (VariableDeclarator *)b->data;
            return ast_nodes_equal(da->id, db->id) && ast_nodes_equal(da->init, db->init);
        }
        case AST_Identifier: {
            Identifier *ia = (Identifier *)a->data;
            Identifier *ib = (Identifier *)b->data;
            if (!ia || !ib) return ia == ib;
            return str_eq(ia->name, ib->name);
        }
        case AST_Literal: {
            Literal *la = (Literal *)a->data;
            Literal *lb = (Literal *)b->data;
            if (la->kind != lb->kind) return 0;
            return str_eq(la->raw, lb->raw);
        }
        case AST_ExpressionStatement: {
            ExpressionStatement *ea = (ExpressionStatement *)a->data;
            ExpressionStatement *eb = (ExpressionStatement *)b->data;
            return ast_nodes_equal(ea->expression, eb->expression);
        }
        case AST_UpdateExpression: {
            UpdateExpression *ua = (UpdateExpression *)a->data;
            UpdateExpression *ub = (UpdateExpression *)b->data;
            if (ua->prefix != ub->prefix) return 0;
            if (!str_eq(ua->operator, ub->operator)) return 0;
            return ast_nodes_equal(ua->argument, ub->argument);
        }
        case AST_UnaryExpression: {
            UnaryExpression *ua = (UnaryExpression *)a->data;
            UnaryExpression *ub = (UnaryExpression *)b->data;
            if (ua->prefix != ub->prefix) return 0;
            if (!str_eq(ua->operator, ub->operator)) return 0;
            return ast_nodes_equal(ua->argument, ub->argument);
        }
        case AST_BinaryExpression: {
            BinaryExpression *ba = (BinaryExpression *)a->data;
            BinaryExpression *bb = (BinaryExpression *)b->data;
            if (!str_eq(ba->operator, bb->operator)) return 0;
            return ast_nodes_equal(ba->left, bb->left) && ast_nodes_equal(ba->right, bb->right);
        }
        case AST_AssignmentExpression: {
            AssignmentExpression *aa = (AssignmentExpression *)a->data;
            AssignmentExpression *ab = (AssignmentExpression *)b->data;
            if (!str_eq(aa->operator, ab->operator)) return 0;
            return ast_nodes_equal(aa->left, ab->left) && ast_nodes_equal(aa->right, ab->right);
        }
        case AST_BlockStatement: {
            BlockStatement *ba = (BlockStatement *)a->data;
            BlockStatement *bb = (BlockStatement *)b->data;
            if (ba->body.count != bb->body.count) return 0;
            for (size_t i = 0; i < ba->body.count; ++i) {
                if (!ast_nodes_equal(ba->body.items[i], bb->body.items[i])) return 0;
            }
            return 1;
        }
        case AST_ReturnStatement: {
            ReturnStatement *ra = (ReturnStatement *)a->data;
            ReturnStatement *rb = (ReturnStatement *)b->data;
            return ast_nodes_equal(ra->argument, rb->argument);
        }
        case AST_CallExpression: {
            CallExpression *ca = (CallExpression *)a->data;
            CallExpression *cb = (CallExpression *)b->data;
            if (!ast_nodes_equal(ca->callee, cb->callee)) return 0;
            if (ca->arguments.count != cb->arguments.count) return 0;
            for (size_t i = 0; i < ca->arguments.count; ++i) {
                if (!ast_nodes_equal(ca->arguments.items[i], cb->arguments.items[i])) return 0;
            }
            return 1;
        }
        case AST_MemberExpression: {
            MemberExpression *ma = (MemberExpression *)a->data;
            MemberExpression *mb = (MemberExpression *)b->data;
            if (ma->computed != mb->computed) return 0;
            return ast_nodes_equal(ma->object, mb->object) && ast_nodes_equal(ma->property, mb->property);
        }
        case AST_FunctionDeclaration:
        case AST_FunctionExpression: {
            FunctionBody *fa = (FunctionBody *)a->data;
            FunctionBody *fb = (FunctionBody *)b->data;
            if (!str_eq(fa->name, fb->name)) return 0;
            if (fa->params.count != fb->params.count) return 0;
            for (size_t i = 0; i < fa->params.count; ++i) {
                if (!ast_nodes_equal(fa->params.items[i], fb->params.items[i])) return 0;
            }
            return ast_nodes_equal(fa->body, fb->body);
        }
        default:
            return 1;
    }
}

void test_roundtrip_simple_var_declaration(void) {
    const char *src = "var x = 42;";
    
    Parser p1;
    parser_init(&p1, src, strlen(src));
    AstNode *ast1 = parse_program(&p1);

    ASSERT_NOT_NULL(ast1, "First parse successful");
    ASSERT_EQ(ast1->type, AST_Program, "Root is Program");

    CodegenOptions opts = {0};
    CodegenResult gen = codegen_generate(ast1, &opts);
    ASSERT_NOT_NULL(gen.code, "Codegen produced code");

    Parser p2;
    parser_init(&p2, gen.code, strlen(gen.code));
    AstNode *ast2 = parse_program(&p2);
    ASSERT_NOT_NULL(ast2, "Second parse successful");
    ASSERT_EQ(ast2->type, AST_Program, "Reparse root is Program");
    ASSERT_EQ(ast_nodes_equal(ast1, ast2), 1, "AST structure preserved");

    codegen_result_free(&gen);
    ast_free(ast1);
    ast_free(ast2);
}

void test_roundtrip_multiple_statements(void) {
    const char *src = "var a = 1;\nlet b = \"hello\";\nx++;";
    
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *ast = parse_program(&p);
    
    ASSERT_NOT_NULL(ast, "Parse successful");
    Program *pr = (Program *)ast->data;
    ASSERT_EQ(pr->body.count, 3, "3 statements preserved");

    CodegenResult gen = codegen_generate(ast, NULL);
    ASSERT_NOT_NULL(gen.code, "Codegen produced code");

    Parser p2;
    parser_init(&p2, gen.code, strlen(gen.code));
    AstNode *ast2 = parse_program(&p2);
    ASSERT_NOT_NULL(ast2, "Reparse successful");
    ASSERT_EQ(ast_nodes_equal(ast, ast2), 1, "AST structure preserved");

    codegen_result_free(&gen);
    ast_free(ast);
    ast_free(ast2);
}

void test_roundtrip_with_comments(void) {
    const char *src = "// comment\nvar x = 1; // inline";
    
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *ast = parse_program(&p);
    
    ASSERT_NOT_NULL(ast, "Parse handles comments");
    CodegenResult gen = codegen_generate(ast, NULL);
    ASSERT_NOT_NULL(gen.code, "Codegen produced code with comments");
    ASSERT_NOT_NULL(strstr(gen.code, "// comment"), "Leading comment preserved");
    ASSERT_NOT_NULL(strstr(gen.code, "// inline"), "Inline comment preserved");

    Parser p2;
    parser_init(&p2, gen.code, strlen(gen.code));
    AstNode *ast2 = parse_program(&p2);
    ASSERT_NOT_NULL(ast2, "Reparse after codegen");
    ASSERT_EQ(ast_nodes_equal(ast, ast2), 1, "AST structure preserved");

    codegen_result_free(&gen);
    ast_free(ast);
    ast_free(ast2);
}

void test_source_map_generation(void) {
    const char *src = "var x = 1;\nreturn x;";

    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *ast = parse_program(&p);
    ASSERT_NOT_NULL(ast, "Parse successful");

    CodegenOptions opts = {0};
    opts.emit_source_map = 1;
    opts.source_name = "input.js";
    CodegenResult gen = codegen_generate(ast, &opts);

    ASSERT_NOT_NULL(gen.code, "Codegen produced code");
    ASSERT_NOT_NULL(gen.source_map, "Source map emitted");
    ASSERT_NOT_NULL(strstr(gen.source_map, "\"version\":3"), "Source map has version");
    ASSERT_NOT_NULL(strstr(gen.source_map, "input.js"), "Source name recorded");
    ASSERT_NOT_NULL(strstr(gen.source_map, "\"mappings\":"), "Mappings present");

    codegen_result_free(&gen);
    ast_free(ast);
}

int main(void) {
    test_roundtrip_simple_var_declaration();
    test_roundtrip_multiple_statements();
    test_roundtrip_with_comments();
    test_source_map_generation();
    
    TEST_SUMMARY();
}
