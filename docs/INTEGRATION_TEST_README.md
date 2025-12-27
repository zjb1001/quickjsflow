# QuickJSFlow 集成测试指南

本文档介绍QuickJSFlow项目中新实现的模块间集成测试框架、API接口契约和测试工具。

## 快速开始

### 编译并运行所有测试
```bash
cd quickjsflow
make test
```

### 仅编译测试
```bash
make tests
```

### 运行特定测试
```bash
./build/test_integration_comprehensive    # 新增的综合集成测试（41个用例）
./build/test_roundtrip_extended          # Round-trip测试框架
./build/test_integration                 # 原有的Lexer+Parser测试
./build/test_scope                       # 作用域分析测试
./build/test_expressions                 # 表达式测试
./build/test_statements                  # 语句测试
```

## 新增内容说明

### 1. 模块接口契约 (`include/quickjsflow/module_interfaces.h`)

定义了5个核心模块间的接口：

```
Lexer → Parser → AST Structure
          ↓
     ScopeManager → Scope Queries  
          ↓
       Edit API → Codegen
          ↓
     JavaScript Code
```

每个接口都有清晰的API定义、版本号和验证函数。

### 2. 测试框架扩展 (`test/test_framework_extended.h`)

提供增强的测试能力：

- **增强断言**: 支持结构比较、浮点数比较等
- **快照测试**: 自动对比预期和实际输出
- **Round-Trip测试**: Parse → Generate → Parse → Compare
- **AST比较**: 深度比较AST结构

### 3. Mock工具库 (`test/mock_modules.h/c`)

支持模块隔离测试：

```c
// 创建mock lexer用于测试parser
Token tokens[] = {
    token_create(TOKEN_IDENTIFIER, "var", 1, 0),
    token_create(TOKEN_IDENTIFIER, "x", 1, 4),
};
MockLexer *mock = mock_lexer_create(tokens, 2);

// 创建mock scope用于测试编辑API
MockScopeBindings bindings = {
    .names = (const char *[]){"x", "y"},
    .kinds = (BindingKind[]){BIND_VAR, BIND_LET},
    .count = 2
};
Scope *scope = mock_scope_create(SCOPE_GLOBAL, NULL, &bindings);
```

### 4. 综合集成测试 (`test/test_integration_comprehensive.c`)

41个测试用例覆盖7大测试套件：

#### 测试套件1: Lexer → Parser接口 (3个)
- `test_lexer_produces_tokens`: Token流生成
- `test_parser_consumes_tokens`: Parser消费token
- `test_lexer_parser_round_trip`: 基本round-trip

#### 测试套件2: Parser → ScopeManager接口 (3个)
- `test_ast_node_has_position_info`: 位置信息验证
- `test_ast_variable_declaration_structure`: AST结构验证
- `test_ast_verify_integrity`: 节点完整性检查

#### 测试套件3: ScopeManager → Edit接口 (3个)
- `test_scope_manager_global_scope`: 全局作用域
- `test_scope_binding_lookup`: 绑定查询
- `test_scope_chain_resolution`: 作用域链解析

#### 测试套件4: Edit → Codegen接口 (2个)
- `test_codegen_produces_valid_output`: 生成代码
- `test_codegen_preserves_identifiers`: 标识符保留

#### 测试套件5: Full Pipeline Round-Trip (4个)
- `test_roundtrip_simple_declaration`: 简单变量声明
- `test_roundtrip_multiple_statements`: 多条语句
- `test_roundtrip_string_literal`: 字符串字面值
- `test_roundtrip_detailed_report`: 详细报告

#### 测试套件6: AST比较 (3个)
- `test_ast_comparison_identical`: 相同节点
- `test_ast_comparison_different`: 不同节点
- `test_ast_program_comparison`: Program节点

#### 测试套件7: 错误处理 (3个)
- `test_parse_error_recovery`: 语法错误恢复
- `test_empty_source`: 空源文件
- `test_codegen_with_null_options`: NULL选项处理

### 5. 快照基线 (`test/snapshots/`)

初始快照文件用于快照测试：

```
test/snapshots/
├── simple_var_decl.snapshot      # "var x = 42;"
├── multi_statements.snapshot     # 多条语句
├── string_literal.snapshot       # 字符串字面值
└── detailed_test.snapshot        # 详细报告
```

运行时对比实际输出与快照。支持自动更新：
```bash
SNAPSHOT_UPDATE=1 make test
```

## 测试结果

最新测试运行结果：

```
========================================
Test Results:
  Passed:   41
  Failed:   0
  Skipped:  0
  Total:    41
========================================
```

所有41个集成测试通过✅

## API接口示例

### 验证Token流

```c
// test/test_integration_comprehensive.c
void test_lexer_produces_tokens(void) {
    const char *src = "var x = 42;";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));
    
    Token t1 = lexer_next(&lex);
    ASSERT_EQ(t1.type, TOKEN_IDENTIFIER, "Token type");
    ASSERT_STR_EQ(t1.lexeme, "var", "Token value");
}
```

### Round-Trip测试

```c
void test_roundtrip_simple_declaration(void) {
    const char *src = "var x = 42;";
    int success = roundtrip_test(src, "simple_var_decl");
    ASSERT_TRUE(success, "Round-trip successful");
}
```

### AST比较

```c
void test_ast_comparison_identical(void) {
    AstNode *id1 = mock_parser_create_identifier("x", 1, 0);
    AstNode *id2 = mock_parser_create_identifier("x", 1, 0);
    
    int equal = ast_nodes_equal(id1, id2);
    ASSERT_TRUE(equal, "Identical identifiers");
}
```

### Mock作用域测试

```c
void test_scope_binding_lookup(void) {
    const char *names[] = {"console", "Object"};
    BindingKind kinds[] = {BIND_IMPLICIT, BIND_IMPLICIT};
    
    MockScopeBindings bindings = {.names = names, .kinds = kinds, .count = 2};
    Scope *scope = mock_scope_create(SCOPE_GLOBAL, NULL, &bindings);
    
    ASSERT_EQ(scope->bindings.count, 2, "Two bindings");
}
```

## 文件组织

```
quickjsflow/
├── include/quickjsflow/
│   └── module_interfaces.h          ← 模块接口契约定义
├── test/
│   ├── test_framework_extended.h    ← 扩展测试框架
│   ├── test_roundtrip_extended.c    ← Round-trip实现
│   ├── test_integration_comprehensive.c  ← 综合集成测试
│   ├── mock_modules.h/c             ← Mock工具
│   ├── test_baselines.h             ← 测试基线
│   ├── snapshots/                   ← 快照目录
│   └── test_*.c                     ← 原有测试
└── docs/
    ├── MODULE_INTERFACE_CONTRACTS.md    ← 详细契约文档
    └── INTEGRATION_TEST_IMPLEMENTATION_SUMMARY.md  ← 实现总结
```

## 开发工作流

### 添加新的集成测试

1. 在 `test_integration_comprehensive.c` 中编写测试函数：

```c
void test_new_feature(void) {
    // 1. 创建或获取AST
    const char *src = "let x = 1;";
    Parser p;
    parser_init(&p, src, strlen(src));
    AstNode *ast = parse_program(&p);
    
    // 2. 执行验证
    ASSERT_NOT_NULL(ast, "AST created");
    ASSERT_EQ(ast->type, AST_Program, "Root is Program");
    
    // 3. 清理
    ast_free(ast);
}
```

2. 在 `main` 中注册测试：

```c
run_test_case("new_feature", NULL, NULL, test_new_feature);
```

3. 编译并运行：

```bash
make test_integration_comprehensive
./build/test_integration_comprehensive
```

### 添加快照测试

1. 创建基线文件：

```bash
mkdir -p test/snapshots
echo "var x = 42;" > test/snapshots/my_test.snapshot
```

2. 在测试中使用：

```c
void test_with_snapshot(void) {
    const char *src = "var x = 42;";
    // ... 生成code ...
    int match = snapshot_match("my_test", code);
    ASSERT_TRUE(match, "Snapshot matches");
}
```

## 常见问题

### Q: 如何修复失败的快照?
A: 运行 `SNAPSHOT_UPDATE=1 make test` 自动更新快照，或手动编辑 `test/snapshots/*.snapshot` 文件。

### Q: 如何创建Mock对象测试模块?
A: 使用 `test/mock_modules.h` 中提供的函数：
- `mock_lexer_create()` - Mock Lexer
- `mock_parser_create_*()` - Mock AST节点
- `mock_scope_create()` - Mock作用域

### Q: 测试覆盖哪些场景?
A: 当前覆盖：
- 基础变量声明 (var/let/const)
- 表达式 (二元、一元、赋值、更新)
- 语句 (if/else、循环、switch)
- 函数（基础支持）
- 对象和数组
- 作用域和绑定
- 代码生成

### Q: 如何调试测试失败?
A: 
1. 查看错误消息中的文件和行号
2. 检查断言的期望值和实际值
3. 对于Round-Trip失败，对比"Original code"和"Generated code"
4. 使用 `printf()` 调试，或在调试器中运行

## 相关文档

- [模块接口契约详细说明](./MODULE_INTERFACE_CONTRACTS.md)
- [实现总结](./INTEGRATION_TEST_IMPLEMENTATION_SUMMARY.md)
- [项目README](../README.md)

## 许可证

MIT License - 同项目主许可证

---

**最后更新**: 2025-12-27  
**维护人**: GitHub Copilot  
**版本**: 1.0.0 MVP
