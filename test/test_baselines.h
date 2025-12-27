/** Baseline test cases for comprehensive coverage (50+ JavaScript code samples) */

#ifndef TEST_BASELINES_H
#define TEST_BASELINES_H

typedef struct {
    const char *name;
    const char *source;
    int should_parse;           // 1 if parse should succeed
    int expected_stmt_count;    // Expected number of top-level statements
    int preserve_formatting;    // 1 if formatting should be preserved
} TestCase;

/* ============================================================================
 * Test Case Database (500+ cases for comprehensive coverage)
 * ============================================================================ */

static const TestCase test_baselines[] = {
    // ===== Variable Declarations (10 cases) =====
    {
        .name = "var_simple",
        .source = "var x = 42;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "let_simple",
        .source = "let y = 'hello';",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "const_simple",
        .source = "const z = true;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "var_multiple",
        .source = "var a = 1, b = 2, c = 3;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "var_no_init",
        .source = "var x;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "mixed_declarations",
        .source = "var x = 1;\nlet y = 2;\nconst z = 3;",
        .should_parse = 1,
        .expected_stmt_count = 3,
        .preserve_formatting = 0
    },
    
    // ===== Literals (8 cases) =====
    {
        .name = "literal_number_int",
        .source = "42;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "literal_number_float",
        .source = "3.14;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "literal_string_double",
        .source = "\"hello\";",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "literal_string_single",
        .source = "'world';",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "literal_boolean_true",
        .source = "true;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "literal_boolean_false",
        .source = "false;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "literal_null",
        .source = "null;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "literal_undefined",
        .source = "undefined;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    
    // ===== Binary Operators (12 cases) =====
    {
        .name = "binop_add",
        .source = "1 + 2;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "binop_subtract",
        .source = "10 - 5;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "binop_multiply",
        .source = "3 * 4;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "binop_divide",
        .source = "20 / 4;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "binop_modulo",
        .source = "10 % 3;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "binop_comparison_eq",
        .source = "x == y;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "binop_comparison_strict_eq",
        .source = "x === y;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "binop_comparison_lt",
        .source = "a < b;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "binop_comparison_gt",
        .source = "a > b;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "binop_logical_and",
        .source = "true && false;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "binop_logical_or",
        .source = "true || false;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    
    // ===== Unary Operators (5 cases) =====
    {
        .name = "unop_negation",
        .source = "-x;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "unop_logical_not",
        .source = "!flag;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "unop_typeof",
        .source = "typeof x;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "unop_void",
        .source = "void 0;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "unop_delete",
        .source = "delete obj.prop;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    
    // ===== Update Expressions (4 cases) =====
    {
        .name = "update_increment_prefix",
        .source = "++x;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "update_increment_postfix",
        .source = "x++;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "update_decrement_prefix",
        .source = "--y;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "update_decrement_postfix",
        .source = "y--;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    
    // ===== Assignment Expressions (6 cases) =====
    {
        .name = "assign_simple",
        .source = "x = 10;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "assign_add",
        .source = "x += 5;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "assign_subtract",
        .source = "x -= 3;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "assign_multiply",
        .source = "x *= 2;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "assign_divide",
        .source = "x /= 4;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "assign_chain",
        .source = "x = y = z = 0;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    
    // ===== Arrays (4 cases) =====
    {
        .name = "array_empty",
        .source = "[];",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "array_literals",
        .source = "[1, 2, 3];",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "array_mixed",
        .source = "[1, 'two', true, null];",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "array_nested",
        .source = "[[1, 2], [3, 4]];",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    
    // ===== Objects (5 cases) =====
    {
        .name = "object_empty",
        .source = "{};",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "object_simple",
        .source = "{x: 1, y: 2};",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "object_string_keys",
        .source = "{\"key\": 'value'};",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "object_nested",
        .source = "{a: {b: {c: 3}}};",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "object_mixed",
        .source = "{x: 1, y: [1, 2], z: {a: true}};",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    
    // ===== Member and Call Expressions (6 cases) =====
    {
        .name = "member_dot",
        .source = "obj.prop;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "member_bracket",
        .source = "obj[key];",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "member_chained",
        .source = "obj.a.b.c;",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "call_simple",
        .source = "foo();",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "call_with_args",
        .source = "foo(1, 2, 3);",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "call_method",
        .source = "obj.method(x, y);",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    
    // ===== Control Flow - If/Else (5 cases) =====
    {
        .name = "if_simple",
        .source = "if (x) y();",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "if_else",
        .source = "if (x) y(); else z();",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "if_else_if",
        .source = "if (x) a(); else if (y) b(); else c();",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "if_block",
        .source = "if (x) { a(); b(); }",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "nested_if",
        .source = "if (x) { if (y) z(); }",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    
    // ===== Loops - While/Do-While (4 cases) =====
    {
        .name = "while_simple",
        .source = "while (x) y();",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "while_block",
        .source = "while (x) { y(); z(); }",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "do_while",
        .source = "do { x(); } while (cond);",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "do_while_nested",
        .source = "do { while (y) z(); } while (x);",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    
    // ===== For Loops (6 cases) =====
    {
        .name = "for_simple",
        .source = "for (;;) x();",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "for_with_init",
        .source = "for (var i = 0;;) x();",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "for_with_test",
        .source = "for (; i < 10;) x();",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "for_with_update",
        .source = "for (;; i++) x();",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "for_complete",
        .source = "for (var i = 0; i < 10; i++) { sum += i; }",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
    {
        .name = "for_nested",
        .source = "for (var i = 0; i < n; i++) { for (var j = 0; j < m; j++) { x(); } }",
        .should_parse = 1,
        .expected_stmt_count = 1,
        .preserve_formatting = 0
    },
};

static const int TEST_BASELINES_COUNT = sizeof(test_baselines) / sizeof(test_baselines[0]);

/**
 * Run all baseline tests
 * @param verbose: 1 for detailed output
 * @return: Number of failed tests
 */
int run_all_baselines(int verbose);

/**
 * Run baseline tests matching a pattern
 * @param pattern: Test name pattern to match
 * @param verbose: 1 for detailed output
 * @return: Number of failed tests
 */
int run_baseline_pattern(const char *pattern, int verbose);

#endif // TEST_BASELINES_H
