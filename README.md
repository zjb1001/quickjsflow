# QuickJSFlow

> 功能完整的 JavaScript 解析、编辑、代码生成工具。为代码转换、静态分析、自动化重构等场景提供开箱即用的解决方案。

## 核心功能

- 🔍 **解析** JavaScript 代码生成 ESTree 兼容的 AST
- ✏️ **编辑** AST（插入、替换、删除、移动、重命名）
- 💾 **生成** 代码：将 AST 转换回可执行的 JavaScript
- 📊 **分析** 代码：作用域追踪、符号表、控制流图（CFG）
- 🔌 **扩展** 功能：灵活的插件系统实现代码转换

## 安装与编译

### 要求

- GCC 或 Clang 编译器
- Make 构建工具
- Linux/macOS/Windows（需要 WSL）

### 快速编译

```bash
cd quickjsflow

# 清理旧构建物并编译
make clean && make

# 验证安装
./build/quickjsflow --help
```

### 可选依赖（测试）

```bash
# Ubuntu/Debian
sudo apt-get install lcov valgrind afl++

# macOS
brew install lcov afl++
```

## 快速开始

### 1. 解析代码生成 AST

```bash
./build/quickjsflow parse examples/sample.js

# 导出为 JSON
./build/quickjsflow parse examples/sample.js > ast.json
```

### 2. 生成代码

```bash
./build/quickjsflow generate examples/sample.js
```

### 3. 语法检查

```bash
./build/quickjsflow check examples/sample.js
```

### 4. 控制流分析

```bash
# JSON 格式
./build/quickjsflow cfg examples/fib.js json

# Graphviz DOT 格式（用于可视化）
./build/quickjsflow cfg examples/fib.js dot | dot -Tpng > cfg.png

# Mermaid 格式（用于 Markdown）
./build/quickjsflow cfg examples/fib.js mermaid
```

### 5. 应用插件转换代码

```bash
# 移除所有 console.log
./build/quickjsflow run examples/with_console.js --plugin remove-console

# 移除所有 debugger 语句
./build/quickjsflow run examples/with_debugger.js --plugin remove-debugger

# 保存到文件
./build/quickjsflow run src/app.js --plugin remove-console > dist/app.js
```

## 完整命令参考

| 命令 | 说明 | 示例 |
|------|------|------|
| `parse` | 生成 AST | `./build/quickjsflow parse file.js` |
| `generate` | 生成代码 | `./build/quickjsflow generate file.js` |
| `check` | 检查语法和作用域 | `./build/quickjsflow check file.js` |
| `cfg` | 生成控制流图 | `./build/quickjsflow cfg file.js [json\|dot\|mermaid]` |
| `lex` | 分词 | `./build/quickjsflow lex file.js` |
| `run` | 应用插件 | `./build/quickjsflow run file.js --plugin <name>` |

## 使用场景

### 场景 1：代码清理与优化

```bash
# 移除生产环境中的调试代码
for file in src/**/*.js; do
  ./build/quickjsflow run "$file" \
    --plugin remove-console \
    --plugin remove-debugger > "dist/${file}"
done
```

### 场景 2：代码分析与检查

```bash
# 检查代码质量
./build/quickjsflow check src/main.js

# 分析函数的控制流
./build/quickjsflow cfg src/main.js mermaid > flow.md
```

### 场景 3：代码转换与重构

```bash
# 解析代码
./build/quickjsflow parse input.js > ast.json

# （用外部工具修改 AST）

# 生成新代码
./build/quickjsflow generate modified_ast.json > output.js
```

### 场景 4：CI/CD 集成

在自动化构建流程中集成：

```bash
#!/bin/bash
set -e

# 1. 构建
make clean && make

# 2. 清理代码
./build/quickjsflow run src/app.js --plugin remove-console > dist/app.js

# 3. 验证输出
./build/quickjsflow check dist/app.js

echo "✅ Build and cleanup succeeded"
```

## 测试

### 运行所有测试

```bash
make test
```

### 运行特定测试集

```bash
./build/test_parser        # 解析器测试
./build/test_lexer_only    # 词法分析测试
./build/test_scope         # 作用域分析测试
./build/test_expressions   # 表达式测试
./build/test_statements    # 语句测试
./build/test_cfg           # 控制流图测试
```

### 生成覆盖率报告

```bash
make coverage-report
```

## 内置插件

### remove-console

移除所有 `console.*` 调用

```bash
./build/quickjsflow run input.js --plugin remove-console
```

**示例**：
```javascript
// 输入
console.log("debug");
const x = 42;

// 输出
const x = 42;
```

### remove-debugger

移除所有 `debugger` 语句

```bash
./build/quickjsflow run input.js --plugin remove-debugger
```

## 支持的 JavaScript 语法

### Phase 1（MVP）- 基础语法 ✅

- 变量声明（`var`, `let`, `const`）
- 基础表达式（字面量、标识符、成员访问、调用）
- 运算符（算术、逻辑、位运算、赋值）
- 控制流（`if/else`, `switch`, `for`, `while`, `do/while`）
- 函数声明和表达式
- 对象和数组字面量
- `try/catch/finally`
- `import`/`export`

**覆盖率**：~85% 的常见 JavaScript 代码

### Phase 2（现代 ES6+）- 部分支持 🟡

- 箭头函数：`() => expr`
- 模板字面量：`` `Hello ${name}` ``
- for-of / for-in 循环
- 类声明和继承
- 解构赋值
- async/await 表达式

## 常见问题

### Q：支持 TypeScript 吗？

A：不支持。QuickJSFlow 只处理纯 JavaScript。对于 TypeScript，请先转译为 JavaScript。

### Q：可以作为执行引擎吗？

A：不可以。这是一个**静态分析和代码转换工具**，不能执行代码。

### Q：支持的最新 JavaScript 特性是什么？

A：目前支持到 ES2020 的大部分特性。详见上面的语法支持列表。

### Q：如何创建自定义插件？

A：参考 [docs/cli-quick-reference.md](docs/cli-quick-reference.md) 中的插件开发指南。

## 文档

快速参考和详细指南：

- [CLI 快速参考](docs/cli-quick-reference.md) - 命令和选项速查
- [集成测试指南](docs/integration-tests-quick-reference.md) - 测试框架说明
- [模块接口规范](docs/module-interface-quick-reference.md) - 架构和模块
- [Phase 2 实现](docs/phase2-implementation-quick-reference.md) - ES6+ 支持进展
- [完整 Issue 清单](issues/00-完整Issue清单与执行计划.md) - 完整设计文档

## 许可证

MIT License - 详见 [LICENSE](LICENSE)

## 贡献

欢迎提交 Issue 和 Pull Request。

---

**最后更新**：2025-12-28  
**版本**：1.0.0 MVP + Phase 2 部分支持
