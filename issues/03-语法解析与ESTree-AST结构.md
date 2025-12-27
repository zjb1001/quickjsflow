# 语法解析与 ESTree AST 结构

## 需求
实现一个递归下降解析器（Recursive Descent Parser），将 Token 流转换为符合 ESTree 标准的 AST。
解析器必须具备 **L2 级容错能力**（语法错误不中断，输出 Partial AST），并实现**精准的注释挂载**。

## Issue 元数据
- 类型：Feature（核心管线）
- 标签：parser, ast, estree, mvp
- 负责人：@page
- 依赖：Issue 02（Lexer）、Issue 12（语法矩阵）
- 目标版本：v0.1.0（MVP）

## 📁 项目结构与开发环境

参考根目录 [issues/00-完整Issue清单与执行计划.md](00-完整Issue清单与执行计划.md) 的**项目结构**与**构建与测试命令**章节。关键路径：
- 源码：`src/parser.c`, `include/quickjsflow/parser.h`, `src/ast_print.c`, `include/quickjsflow/ast.h`
- 测试：`test/test_integration.c`, `test/test_roundtrip.c`
- 示例：`examples/` 目录含各种 JS 样本
- 构建：`make` 编译，`make test` 运行端到端测试
- 命令：`./build/quickjsflow parse <file>` 查看 AST JSON 输出

## 技术要点

### 1. 解析架构
- **输入**：TokenStream (from Issue 02)
- **输出**：ESTree Program Node
- **算法**：递归下降（Recursive Descent）
  - 每个语法规则对应一个解析函数（e.g., `parseStatement`, `parseExpression`）
  - 利用 `lookahead` (peek) 决定分支

### 2. 错误恢复策略（L2 容错）
目标：遇到语法错误时，不抛出异常终止，而是恢复到同步点继续解析。
- **同步点（Synchronization Points）**：通常是语句结束符 `;` 或块结束符 `}`。
- **Error Node**：在 AST 中插入自定义的 `ErrorNode` 或保留部分结构，标记 `error: true`。
- **策略**：
  - 遇到意外 Token：报错 -> 丢弃 Token 直到遇到同步点 -> 恢复解析下一条语句。
  - 缺失 Token（如分号）：报错 -> 自动补全（逻辑上）-> 继续。

### 3. 注释挂载（Comment Attachment）
注释不是标准 ESTree 的一部分，但对编辑至关重要。
- **存储**：在 AST 节点上的 `leadingComments` (前置), `trailingComments` (后置), `innerComments` (内部) 属性。
- **算法**：
  - 维护一个待处理注释列表。
  - 节点解析开始前，收集之前的注释作为 `leading`。
  - 节点解析结束后，收集紧随其后的注释作为 `trailing`。
  - 对于 BlockStatement 或 ObjectExpression，处理大括号内的 `inner` 注释。

### 4. 优先级与结合性（Pratt Parsing 或 优先级表）
- 处理二元表达式（`+`, `*`, `&&`）的优先级。
- 建议使用 **Pratt Parsing** (Top-Down Operator Precedence) 处理表达式，简化递归逻辑。

## 交付物
- 🔧 Parser 核心模块
  - `parse(source: string, options?: ParserOptions): Program`
- 📚 AST 类型定义（扩展 ESTree 以支持 ErrorNode）
- 🧪 测试套件
  - **Snapshot Tests**：针对 Issue 12 中的所有 MVP 语法。
  - **Error Recovery Tests**：输入错误代码，验证 AST 结构和错误信息。
  - **Comment Tests**：验证注释是否挂载到正确的节点。

## 任务清单
- [ ] 定义 AST 节点类型（TypeScript 接口或 JSDoc），包含 `loc`, `range`, `comments`。
- [ ] 实现基础解析框架（`Parser` 类，`next()`, `expect()` 等工具方法）。
- [ ] 实现 **Statement** 解析（Variable, If, Return, Block, etc.）。
- [ ] 实现 **Expression** 解析（Binary, Unary, Call, Member, etc.）。
- [ ] 实现 **Pratt Parsing** 处理表达式优先级。
- [ ] 实现 **L2 错误恢复**机制（`synchronize()` 方法）。
- [ ] 实现 **注释挂载** 逻辑。
- [ ] 集成 Issue 12 的所有 MVP 语法特性。
- [ ] 编写快照测试与容错测试。

## 验收标准
- ✅ 100% 覆盖 Issue 12 定义的 MVP 语法。
- ✅ 语法错误时，Parser 不抛出异常，而是返回包含错误信息的 AST。
- ✅ 注释（单行/多行）准确挂载，不丢失，不漂移。
- ✅ 位置信息（Line/Column）精确，包含起始和结束。
- ✅ 通过 Test262 基础测试集（可选，作为高质量标准）。

## 优先级
🔴 **MVP**（核心中的核心）

## 相关 Issue
- Issue 02: Lexer（上游）
- Issue 04: ScopeManager（下游）
- Issue 12: 语法矩阵（规范）
