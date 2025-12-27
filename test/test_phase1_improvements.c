#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quickjsflow/parser.h"
#include "quickjsflow/ast.h"

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

static AstNode *parse_code(const char *code) {
    Parser p;
    parser_init(&p, code, strlen(code));
    AstNode *ast = parse_program(&p);
    return ast;
}

// Test Boolean literals
int test_boolean_literals() {
    TEST("Boolean literals (true/false)");
    
    const char *code = "const x = true; const y = false;";
    AstNode *ast = parse_code(code);
    ASSERT(ast != NULL, "Parse boolean literals");
    ASSERT(ast->type == AST_Program, "Root is Program");
    
    Program *prog = (Program *)ast->data;
    ASSERT(prog->body.count == 2, "Has 2 statements");
    
    // Check first statement: const x = true;
    AstNode *stmt1 = prog->body.items[0];
    ASSERT(stmt1->type == AST_VariableDeclaration, "First is VariableDeclaration");
    
    VariableDeclaration *vd1 = (VariableDeclaration *)stmt1->data;
    ASSERT(vd1->declarations.count > 0, "Has declarators");
    
    AstNode *declarator1 = vd1->declarations.items[0];
    ASSERT(declarator1->type == AST_VariableDeclarator, "Is VariableDeclarator");
    
    VariableDeclarator *vdec1 = (VariableDeclarator *)declarator1->data;
    ASSERT(vdec1->init != NULL, "Has initializer");
    ASSERT(vdec1->init->type == AST_Literal, "Initializer is Literal");
    
    Literal *lit1 = (Literal *)vdec1->init->data;
    ASSERT(lit1->kind == LIT_Boolean, "Literal is Boolean");
    ASSERT(strcmp(lit1->raw, "true") == 0, "Boolean value is 'true'");
    
    ast_free(ast);
    TEST_END;
}

// Test null literal
int test_null_literal() {
    TEST("Null literal");
    
    const char *code = "const x = null;";
    AstNode *ast = parse_code(code);
    ASSERT(ast != NULL, "Parse null literal");
    
    Program *prog = (Program *)ast->data;
    AstNode *stmt = prog->body.items[0];
    VariableDeclaration *vd = (VariableDeclaration *)stmt->data;
    AstNode *declarator = vd->declarations.items[0];
    VariableDeclarator *vdec = (VariableDeclarator *)declarator->data;
    
    ASSERT(vdec->init != NULL, "Has initializer");
    ASSERT(vdec->init->type == AST_Literal, "Initializer is Literal");
    
    Literal *lit = (Literal *)vdec->init->data;
    ASSERT(lit->kind == LIT_Null, "Literal is Null");
    ASSERT(strcmp(lit->raw, "null") == 0, "Null value is 'null'");
    
    ast_free(ast);
    TEST_END;
}

// Test undefined literal
int test_undefined_literal() {
    TEST("Undefined literal");
    
    const char *code = "const x = undefined;";
    AstNode *ast = parse_code(code);
    ASSERT(ast != NULL, "Parse undefined literal");
    
    Program *prog = (Program *)ast->data;
    AstNode *stmt = prog->body.items[0];
    VariableDeclaration *vd = (VariableDeclaration *)stmt->data;
    AstNode *declarator = vd->declarations.items[0];
    VariableDeclarator *vdec = (VariableDeclarator *)declarator->data;
    
    ASSERT(vdec->init != NULL, "Has initializer");
    ASSERT(vdec->init->type == AST_Literal, "Initializer is Literal");
    
    Literal *lit = (Literal *)vdec->init->data;
    ASSERT(lit->kind == LIT_Undefined, "Literal is Undefined");
    ASSERT(strcmp(lit->raw, "undefined") == 0, "Undefined value is 'undefined'");
    
    ast_free(ast);
    TEST_END;
}

// Test import default specifier
int test_import_default_specifier() {
    TEST("Import default specifier");
    
    const char *code = "import React from 'react';";
    AstNode *ast = parse_code(code);
    ASSERT(ast != NULL, "Parse import default");
    
    Program *prog = (Program *)ast->data;
    ASSERT(prog->body.count > 0, "Has statements");
    
    AstNode *stmt = prog->body.items[0];
    ASSERT(stmt->type == AST_ImportDeclaration, "Is ImportDeclaration");
    
    ImportDeclaration *id = (ImportDeclaration *)stmt->data;
    ASSERT(id->specifiers.count == 1, "Has 1 specifier");
    ASSERT(id->specifiers.items[0]->type == AST_ImportDefaultSpecifier, "Is ImportDefaultSpecifier");
    
    ImportDefaultSpecifier *ids = (ImportDefaultSpecifier *)id->specifiers.items[0]->data;
    ASSERT(ids->local != NULL, "Has local identifier");
    ASSERT(ids->local->type == AST_Identifier, "Local is Identifier");
    
    Identifier *local_id = (Identifier *)ids->local->data;
    ASSERT(strcmp(local_id->name, "React") == 0, "Local name is 'React'");
    
    ast_free(ast);
    TEST_END;
}

// Test import namespace specifier
int test_import_namespace_specifier() {
    TEST("Import namespace specifier");
    
    const char *code = "import * as Utils from 'utils';";
    AstNode *ast = parse_code(code);
    ASSERT(ast != NULL, "Parse import namespace");
    
    Program *prog = (Program *)ast->data;
    ASSERT(prog->body.count > 0, "Has statements");
    
    AstNode *stmt = prog->body.items[0];
    ASSERT(stmt->type == AST_ImportDeclaration, "Is ImportDeclaration");
    
    ImportDeclaration *id = (ImportDeclaration *)stmt->data;
    ASSERT(id->specifiers.count == 1, "Has 1 specifier");
    ASSERT(id->specifiers.items[0]->type == AST_ImportNamespaceSpecifier, "Is ImportNamespaceSpecifier");
    
    ImportNamespaceSpecifier *ins = (ImportNamespaceSpecifier *)id->specifiers.items[0]->data;
    ASSERT(ins->local != NULL, "Has local identifier");
    ASSERT(ins->local->type == AST_Identifier, "Local is Identifier");
    
    Identifier *local_id = (Identifier *)ins->local->data;
    ASSERT(strcmp(local_id->name, "Utils") == 0, "Local name is 'Utils'");
    
    ast_free(ast);
    TEST_END;
}

// Test mixed import (default + named)
int test_mixed_import() {
    TEST("Mixed import (default + named)");
    
    const char *code = "import React, { useState } from 'react';";
    AstNode *ast = parse_code(code);
    ASSERT(ast != NULL, "Parse mixed import");
    
    Program *prog = (Program *)ast->data;
    AstNode *stmt = prog->body.items[0];
    ASSERT(stmt->type == AST_ImportDeclaration, "Is ImportDeclaration");
    
    ImportDeclaration *id = (ImportDeclaration *)stmt->data;
    ASSERT(id->specifiers.count == 2, "Has 2 specifiers");
    
    // First should be default
    ASSERT(id->specifiers.items[0]->type == AST_ImportDefaultSpecifier, "First is ImportDefaultSpecifier");
    
    // Second should be named
    ASSERT(id->specifiers.items[1]->type == AST_ImportSpecifier, "Second is ImportSpecifier");
    
    ast_free(ast);
    TEST_END;
}

// Test import with alias
int test_import_with_alias() {
    TEST("Import with alias");
    
    const char *code = "import { Component as Comp } from 'react';";
    AstNode *ast = parse_code(code);
    ASSERT(ast != NULL, "Parse import with alias");
    
    Program *prog = (Program *)ast->data;
    AstNode *stmt = prog->body.items[0];
    ImportDeclaration *id = (ImportDeclaration *)stmt->data;
    
    ASSERT(id->specifiers.count == 1, "Has 1 specifier");
    ASSERT(id->specifiers.items[0]->type == AST_ImportSpecifier, "Is ImportSpecifier");
    
    ImportSpecifier *is = (ImportSpecifier *)id->specifiers.items[0]->data;
    
    Identifier *imported = (Identifier *)is->imported->data;
    ASSERT(strcmp(imported->name, "Component") == 0, "Imported name is 'Component'");
    
    Identifier *local = (Identifier *)is->local->data;
    ASSERT(strcmp(local->name, "Comp") == 0, "Local name is 'Comp'");
    
    ast_free(ast);
    TEST_END;
}

int main() {
    printf("=================================\n");
    printf("Phase 1 Improvements Tests\n");
    printf("=================================\n");
    
    test_boolean_literals();
    test_null_literal();
    test_undefined_literal();
    test_import_default_specifier();
    test_import_namespace_specifier();
    test_mixed_import();
    test_import_with_alias();
    
    printf("\n=================================\n");
    printf("Results: %d/%d tests passed\n", test_passed, test_count);
    printf("=================================\n");
    
    return (test_passed == test_count) ? 0 : 1;
}
