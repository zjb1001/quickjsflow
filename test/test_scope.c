#include <stdlib.h>
#include <string.h>
#include "quickjsflow/parser.h"
#include "quickjsflow/scope.h"
#include "test_framework.h"

static AstNode *parse_source(const char *src) {
    Parser p;
    parser_init(&p, src, strlen(src));
    return parse_program(&p);
}

static Binding *find_binding(Scope *scope, const char *name) {
    return scope_lookup_local(scope, name);
}

static Reference *find_reference(Scope *scope, const char *name, int ordinal) {
    int seen = 0;
    for (size_t i = 0; i < scope->references.count; ++i) {
        Reference *r = scope->references.items[i];
        if (r && r->name && strcmp(r->name, name) == 0) {
            if (seen == ordinal) return r;
            seen++;
        }
    }
    return NULL;
}

static void test_global_bindings(void) {
    const char *src = "var a; let b = 1; const c = 2;";
    AstNode *root = parse_source(src);

    ScopeManager sm;
    scope_manager_init(&sm);
    scope_analyze(&sm, root, 0);

    Binding *a = find_binding(sm.root, "a");
    Binding *b = find_binding(sm.root, "b");
    Binding *c = find_binding(sm.root, "c");

    ASSERT_NOT_NULL(a, "var a bound in global scope");
    ASSERT_EQ(a->kind, BIND_VAR, "var binding kind");
    ASSERT_NOT_NULL(b, "let b bound in global scope");
    ASSERT_EQ(b->kind, BIND_LET, "let binding kind");
    ASSERT_NOT_NULL(c, "const c bound in global scope");
    ASSERT_EQ(c->kind, BIND_CONST, "const binding kind");

    scope_manager_free(&sm);
    ast_free(root);
}

static void test_function_scopes(void) {
    const char *src = "function foo(a){ var x = a; let y = x; { const z = y; } }";
    AstNode *root = parse_source(src);

    ScopeManager sm; scope_manager_init(&sm);
    scope_analyze(&sm, root, 0);

    Program *pr = (Program *)root->data;
    AstNode *fn_node = pr->body.items[0];
    Scope *fn_scope = scope_of_node(&sm, fn_node);

    Binding *foo = find_binding(sm.root, "foo");
    ASSERT_NOT_NULL(foo, "foo hoisted to parent scope");
    ASSERT_EQ(foo->kind, BIND_FUNCTION, "function binding kind");

    Binding *param_a = find_binding(fn_scope, "a");
    Binding *var_x = find_binding(fn_scope, "x");
    Binding *let_y = find_binding(fn_scope, "y");

    ASSERT_NOT_NULL(param_a, "param a in function scope");
    ASSERT_EQ(param_a->kind, BIND_PARAM, "param binding kind");
    ASSERT_NOT_NULL(var_x, "var x in function scope");
    ASSERT_EQ(var_x->kind, BIND_VAR, "var hoisted to function scope");
    ASSERT_NOT_NULL(let_y, "let y in function scope");
    ASSERT_EQ(let_y->kind, BIND_LET, "let stays lexical in function scope");

    FunctionBody *fb = (FunctionBody *)fn_node->data;
    BlockStatement *fn_body = (BlockStatement *)fb->body->data;
    AstNode *inner_block = fn_body->body.items[2];
    Scope *inner_scope = scope_of_node(&sm, inner_block);
    Binding *const_z = find_binding(inner_scope, "z");
    ASSERT_NOT_NULL(const_z, "const z in inner block scope");
    ASSERT_EQ(const_z->kind, BIND_CONST, "const binding kind");

    Reference *ref_a = find_reference(fn_scope, "a", 0);
    Reference *ref_x = find_reference(fn_scope, "x", 0);
    ASSERT_NOT_NULL(ref_a, "reference to a recorded");
    ASSERT_NOT_NULL(ref_x, "reference to x recorded");
    ASSERT_EQ(ref_a->resolved == param_a, 1, "a resolves to param binding");
    ASSERT_EQ(ref_x->resolved == var_x, 1, "x resolves to var binding");

    scope_manager_free(&sm);
    ast_free(root);
}

static void test_tdz_detection(void) {
    const char *src = "function f(){ console.log(x); let x = 1; }";
    AstNode *root = parse_source(src);

    ScopeManager sm; scope_manager_init(&sm);
    scope_analyze(&sm, root, 0);

    Program *pr = (Program *)root->data;
    AstNode *fn_node = pr->body.items[0];
    Scope *fn_scope = scope_of_node(&sm, fn_node);

    Binding *let_x = find_binding(fn_scope, "x");
    ASSERT_NOT_NULL(let_x, "let x declared");

    Reference *ref_x = find_reference(fn_scope, "x", 0);
    ASSERT_NOT_NULL(ref_x, "reference to x captured");
    ASSERT_EQ(ref_x->resolved == let_x, 1, "reference resolves to let binding");
    ASSERT_EQ(ref_x->in_tdz, 1, "reference before let is in TDZ");

    scope_manager_free(&sm);
    ast_free(root);
}

static void test_catch_scope(void) {
    const char *src = "try { throw 1; } catch (e) { e; }";
    AstNode *root = parse_source(src);

    ScopeManager sm; scope_manager_init(&sm);
    scope_analyze(&sm, root, 0);

    Program *pr = (Program *)root->data;
    AstNode *try_node = pr->body.items[0];
    TryStatement *ts = (TryStatement *)try_node->data;
    AstNode *catch_node = ts->handlers.items[0];
    Scope *catch_scope = scope_of_node(&sm, catch_node);

    Binding *catch_e = find_binding(catch_scope, "e");
    ASSERT_NOT_NULL(catch_e, "catch binding exists");
    ASSERT_EQ(catch_e->kind, BIND_CATCH, "catch binding kind");

    Reference *ref_e = find_reference(catch_scope, "e", 0);
    ASSERT_NOT_NULL(ref_e, "reference to e captured");
    ASSERT_EQ(ref_e->resolved == catch_e, 1, "catch reference resolves");
    ASSERT_EQ(ref_e->in_tdz, 0, "catch reference not in TDZ");

    scope_manager_free(&sm);
    ast_free(root);
}

static void test_for_scope_and_hoisting(void) {
    const char *src = "for (let i = 0; i < 2; i++) { var x = i; }";
    AstNode *root = parse_source(src);

    ScopeManager sm; scope_manager_init(&sm);
    scope_analyze(&sm, root, 0);

    Program *pr = (Program *)root->data;
    AstNode *for_node = pr->body.items[0];
    Scope *for_scope = scope_of_node(&sm, for_node);

    Binding *i_binding = find_binding(for_scope, "i");
    ASSERT_NOT_NULL(i_binding, "loop let binding exists");
    ASSERT_EQ(i_binding->kind, BIND_LET, "loop binding is let");

    Binding *x_binding = find_binding(sm.root, "x");
    ASSERT_NOT_NULL(x_binding, "var x hoists to global scope");
    ASSERT_EQ(x_binding->kind, BIND_VAR, "var binding kind");

    Reference *ref_i = find_reference(for_scope, "i", 0);
    ASSERT_NOT_NULL(ref_i, "loop body sees i");
    ASSERT_EQ(ref_i->resolved == i_binding, 1, "loop reference resolves to let binding");

    scope_manager_free(&sm);
    ast_free(root);
}

static void test_shadowing_detection(void) {
    const char *src = "let a = 1; { let a = 2; a; }";
    AstNode *root = parse_source(src);

    ScopeManager sm; scope_manager_init(&sm);
    scope_analyze(&sm, root, 0);

    Program *pr = (Program *)root->data;
    AstNode *block = pr->body.items[1];
    Scope *outer_scope = sm.root;
    Scope *inner_scope = scope_of_node(&sm, block);

    Binding *outer_a = find_binding(outer_scope, "a");
    Binding *inner_a = find_binding(inner_scope, "a");
    ASSERT_NOT_NULL(outer_a, "outer a binding exists");
    ASSERT_NOT_NULL(inner_a, "inner a binding exists");
    ASSERT_EQ(inner_a->shadowed == outer_a, 1, "inner a shadows outer a");

    Reference *ref_inner_a = find_reference(inner_scope, "a", 0);
    ASSERT_NOT_NULL(ref_inner_a, "inner reference recorded");
    ASSERT_EQ(ref_inner_a->resolved == inner_a, 1, "reference resolves to inner binding");

    scope_manager_free(&sm);
    ast_free(root);
}

static void test_implicit_globals(void) {
    const char *src = "function f(){ x = 1; } x;";
    AstNode *root = parse_source(src);

    ScopeManager sm; scope_manager_init(&sm);
    scope_analyze(&sm, root, 0);

    Binding *implicit_x = find_binding(sm.root, "x");
    ASSERT_NOT_NULL(implicit_x, "implicit global created for x");
    ASSERT_EQ(implicit_x->kind, BIND_IMPLICIT, "implicit binding kind");

    Program *pr = (Program *)root->data;
    AstNode *fn_node = pr->body.items[0];
    Scope *fn_scope = scope_of_node(&sm, fn_node);

    Reference *ref_fn_x = find_reference(fn_scope, "x", 0);
    ASSERT_NOT_NULL(ref_fn_x, "function reference to x captured");
    ASSERT_EQ(ref_fn_x->resolved == implicit_x, 1, "function write resolves to implicit global");
    ASSERT_EQ(ref_fn_x->is_write, 1, "function reference is write");

    Reference *ref_global_x = find_reference(sm.root, "x", 0);
    ASSERT_NOT_NULL(ref_global_x, "global reference to x captured");
    ASSERT_EQ(ref_global_x->resolved == implicit_x, 1, "global read resolves to implicit global");

    scope_manager_free(&sm);
    ast_free(root);
}

int main(void) {
    test_global_bindings();
    test_function_scopes();
    test_tdz_detection();
    test_catch_scope();
    test_for_scope_and_hoisting();
    test_shadowing_detection();
    test_implicit_globals();
    TEST_SUMMARY();
}
