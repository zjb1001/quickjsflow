# Module Interface Contracts and Verification

## 目标
明确各模块之间的API契约，支持独立开发和测试，确保端到端流程完整性。

## 1. Lexer → Parser: Token Stream Interface

### 接口定义
```c
// 基本的token流操作
Token lexer_next(Lexer *lx);        // 获取下一个token，推进位置
Token lexer_peek(Lexer *lx);        // 查看下一个token，不推进
void  lexer_init(Lexer *lx, ...);   // 初始化
```

### 契约要点
- **稳定性**: Token 类型枚举固定，新增token类型向后兼容
- **完整性**: Lexer产生的所有token必须包含：
  - `type`: TOKEN_IDENTIFIER | TOKEN_NUMBER | TOKEN_STRING | TOKEN_PUNCTUATOR | TOKEN_KEYWORD
  - `lexeme`: 原始文本（可为NULL用于EOF）
  - `start_line, start_col, end_line, end_col`: 位置信息
  - `error`: 错误标志（用于错误恢复）
- **定位准确**: 位置信息必须与源文本一致（行/列计数准确）

### 验证方式
```c
// test_lexer_parser_token_stream.c
void test_token_count(const char *src, int expected_count);
void test_token_positions_continuous(const char *src);
void test_token_lexeme_accuracy(const char *src);
```

### 当前状态
✅ Lexer实现完成，token生成正确
✅ Parser正常消费token流
⏳ 需要编写完整的edge case测试

---

## 2. Parser → ScopeManager: AST Structure Interface

### 接口定义
```c
typedef struct AstNode {
    AstNodeType type;           // ESTree兼容的节点类型
    Position start;             // 源文件中的起始位置
    Position end;               // 源文件中的结束位置
    int refcount;              // 引用计数（支持结构共享）
    void *data;                // 类型特定的数据
} AstNode;
```

### 契约要点
- **类型完整性**: 所有AstNode.type必须是有效的AstNodeType枚举值
- **位置一致性**: start <= end，列号递增
- **数据有效性**: data指针与type一致（无类型混淆）
- **层次结构**: 所有子节点的位置包含于父节点
- **ESTree兼容**: 节点类型和结构符合ESTree 5.0规范

### 验证方式
```c
// ast_verify.c
int ast_verify(const AstNode *node);        // 验证单个节点
int ast_verify_tree(const AstNode *root);   // 递归验证整棵树
```

### 当前状态
✅ AST节点结构定义完成
✅ Parser生成基本节点（Var, Identifier, Literal等）
⏳ 需要完整的节点类型支持（Function, Class, Arrow等）

---

## 3. ScopeManager → Edit API: Scope Query Interface

### 接口定义
```c
// 作用域查询（只读）
Binding *scope_lookup_local(Scope *scope, const char *name);
Binding *scope_resolve(Scope *scope, const char *name);
Scope *scope_of_node(const ScopeManager *sm, const AstNode *node);
```

### 契约要点
- **链式解析**: 变量查询必须遍历完整的作用域链
- **遮蔽处理**: 返回最近的绑定，记录被遮蔽的绑定
- **绑定完整性**: 所有声明（var/let/const/function/param）必须形成binding
- **引用链接**: 所有identifier引用必须链接到对应的binding
- **TDZ处理**: let/const的temporal dead zone必须被正确标记

### 验证方式
```c
// test_scope_queries.c
void test_scope_lookup(const AstNode *ast);
void test_scope_chain(const AstNode *ast);
void test_variable_shadowing(const char *src);
void test_tdz_detection(const char *src);
```

### 当前状态
⏳ ScopeManager框架已建立，但完整实现待完成

---

## 4. Edit API → Codegen: AST Immutability Contract

### 接口定义
```c
// 编辑操作（未来实现）
AstNode *ast_replace_node(const AstNode *parent, 
                          const AstNode *old_child,
                          const AstNode *new_child);
AstNode *ast_remove_node(const AstNode *parent,
                         const AstNode *child);
AstNode *ast_insert_node(const AstNode *parent,
                         size_t index,
                         const AstNode *new_child);
```

### 契约要点
- **不可变性**: 所有编辑操作返回新的AST副本，原AST保持不变
- **结构保证**: 修改后的AST结构与编辑前等价（除修改点外）
- **引用计数**: 使用refcount进行结构共享，避免深拷贝
- **位置更新**: 修改后的节点位置信息保持一致

### 验证方式
```c
// test_edit_immutability.c
void test_replace_preserves_original(void);
void test_edit_creates_new_refcount(void);
void test_edit_preserves_unmodified(void);
```

### 当前状态
⏳ 设计完成，实现待启动（MVP不包含编辑API）

---

## 5. Codegen Interface

### 接口定义
```c
CodegenResult codegen_generate(const AstNode *root,
                               const CodegenOptions *options);

typedef struct {
    char *code;        // 生成的JavaScript代码
    char *source_map;  // 可选的SourceMap JSON
} CodegenResult;
```

### 契约要点
- **确定性**: 相同的AST必须生成相同的代码
- **名称保留**: 所有标识符名称保持不变
- **字面值保留**: 所有字面值的语义保持不变
- **注释保留**: 源代码注释尽量保留在输出中
- **SourceMap有效**: 生成的SourceMap必须是有效的JSON，且映射准确

### 验证方式
```c
// test_codegen_contract.c
void test_deterministic_generation(const AstNode *ast);
void test_identifier_preservation(const AstNode *ast);
void test_sourcemap_validity(const CodegenResult *result);
```

### 当前状态
✅ Codegen框架已建立
⏳ 格式化逻辑和SourceMap生成待完成

---

## 6. End-to-End Round-Trip Test

### 验证流程
```
源码 (Source)
  ↓
Lexer → Token流
  ↓
Parser → AST1
  ↓
Codegen → 生成码
  ↓
Parser → AST2
  ↓
比较 AST1 ≈ AST2 (结构等价性)
```

### 验证标准
1. **AST结构等价**: 递归比较节点类型、子节点数、绑定关系
2. **节点计数相同**: statement和expression数量保持一致
3. **标识符名称相同**: 所有变量名保持不变
4. **字面值相同**: 所有数字、字符串字面值保持不变

### 测试覆盖
- 基础变量声明（var/let/const）
- 表达式（二元、一元、赋值、更新）
- 控制流（if/else、for、while、switch）
- 函数（声明、表达式、参数）
- 对象和数组
- 复杂嵌套结构

### 当前状态
✅ Round-trip测试框架已建立
⏳ 测试用例库待完善（目标500+用例）

---

## 版本管理

### 接口版本
```c
ASTVersion ast_get_version(void);           // 返回 1.0.0
ScopeInterfaceVersion scope_interface_version(void);
EditInterfaceVersion edit_interface_version(void);
CodegenInterfaceVersion codegen_interface_version(void);
```

### 兼容性保证
- **向后兼容**: 新增功能不破坏现有API
- **新节点类型**: 添加到AstNodeType枚举，现有代码继续有效
- **新token类型**: 扩展TokenType，parser可忽略未知token
- **可选字段**: 结构体中新增字段必须有合理默认值

---

## 快照和基线

### 快照存储
```
test/snapshots/
├── simple_var_decl.snapshot
├── multi_statements.snapshot
├── string_literal.snapshot
└── ...
```

### 快照内容
每个快照文件包含：
```json
{
  "source": "var x = 42;",
  "expected_ast": { /* AST structure */ },
  "expected_code": "var x = 42;",
  "expected_bindings": ["x"]
}
```

### 基线更新
```bash
# 自动更新快照
SNAPSHOT_UPDATE=1 make test

# 查看差异
diff test/snapshots/test.snapshot test/snapshots/test.actual
```

---

## Mock工具和测试工具库

### 可用Mock
1. **MockLexer**: 预定义token序列
2. **MockParser**: 合成AST结构
3. **MockCodegen**: 固定输出模式
4. **MockScopeManager**: 预定义绑定关系

### 使用示例
```c
// 测试Parser对特定token序列的处理
Token tokens[] = {
    token_create(TOKEN_IDENTIFIER, "var", 1, 0),
    token_create(TOKEN_IDENTIFIER, "x", 1, 4),
    // ...
};
MockLexer *mock = mock_lexer_create(tokens, sizeof(tokens) / sizeof(tokens[0]));

// 测试ScopeManager的绑定查询
MockScopeBindings bindings = {
    .names = (const char *[]){"x", "y"},
    .kinds = (BindingKind[]){BIND_VAR, BIND_LET},
    .count = 2
};
Scope *scope = mock_scope_create(SCOPE_GLOBAL, NULL, &bindings);
```

---

## CI/CD集成

### 自动化检查
1. **编译检查**: 所有模块独立编译
2. **单元测试**: 各模块单独测试
3. **集成测试**: 模块边界测试
4. **Round-trip**: 500+用例覆盖
5. **覆盖率**: 目标 >80% 代码覆盖

### 构建命令
```bash
make clean
make test           # 编译并运行所有测试
make test-coverage  # 生成覆盖率报告
make test-roundtrip # 运行round-trip测试
```

---

## 检查清单

- [ ] 所有接口定义完整（module_interfaces.h）
- [ ] Mock工具实现完成（mock_modules.h/c）
- [ ] Round-trip测试框架完成（test_roundtrip_extended.c）
- [ ] 集成测试用例库完成（test_integration_comprehensive.c）
- [ ] 快照基线建立（50+基线用例）
- [ ] 文档完整（本文件）
- [ ] CI集成完成（GitHub Actions）
- [ ] 覆盖率报告集成

---

## 参考
- ESTree Spec: https://github.com/estree/estree
- 源文件: include/quickjsflow/module_interfaces.h
- 测试: test/test_integration_comprehensive.c
- Mock: test/mock_modules.h/c
