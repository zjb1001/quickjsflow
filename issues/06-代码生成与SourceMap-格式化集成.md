# 代码生成与 Source Map/格式化集成

## 需求
实现一个代码生成器（Code Generator / Printer），将 ESTree AST 转换回 JavaScript 源代码。
必须支持 **Source Map 生成**，以便调试；并能**完美还原注释**，保持代码的可读性。

## Issue 元数据
- 类型：Feature（核心管线）
- 标签：codegen, printer, sourcemap, formatting, mvp
- 负责人：@page

## 📁 项目结构与开发环境

参考根目录 [issues/00-完整Issue清单与执行计划.md](00-完整Issue清单与执行计划.md) 的**项目结构**与**构建与测试命令**章节。关键路径：
- 源码：`src/codegen.c`, `include/quickjsflow/codegen.h`（待实现）
- 测试：`test/test_roundtrip.c`（已有框架，等待 codegen 集成）
- 依赖：Issue 03（Parser/AST）, Issue 05（Edit API 可选）
- 构建：`make && make test`
- Round-trip：`make test` 运行 test_roundtrip，验证 parse→print→parse 等价性

## 技术要点

### 需求
- 依赖：Issue 03（AST）、Issue 05（Edit API）
- 目标版本：v0.1.0（MVP）

## 技术要点

### 1. 打印架构（Printer Architecture）
- **Visitor 模式**：针对每种 AST 节点类型定义打印逻辑。
- **状态管理**：维护当前缩进级别、当前行/列号。
- **Buffer**：使用 StringBuilder 或类似结构拼接字符串。

### 2. 格式化策略（Formatting）
- **缩进**：可配置（空格/Tab，数量）。
- **括号**：根据运算符优先级自动添加必要的括号，避免逻辑错误。
- **分号**：自动插入分号（ASI 显式化）。
- **引号**：保持或统一（单引号/双引号）。

### 3. 注释还原（Comment Re-attachment）
- 在打印节点之前，检查 `leadingComments` 并打印。
- 在打印节点之后，检查 `trailingComments` 并打印。
- 处理块级注释内的换行和缩进，防止破坏布局。

### 4. Source Map 生成
- 使用 `source-map` 库或自行实现 VLQ 编码。
- 在打印每个 Token（或关键节点）时，记录：
  - 生成代码的 (line, col)
  - 原始代码的 (source, line, col, name)
- 确保映射的准确性，特别是经过编辑后的节点。

## 交付物
- 🔧 Generator 模块
  - `generate(ast: Node, options?: GenOptions): { code: string, map: object }`
- 🧪 测试套件
  - **Round-trip Test**：`parse -> generate -> parse`，比较 AST 结构等价性。
  - **Source Map Test**：验证生成的 map 能正确映射回源码位置。
  - **Formatting Test**：验证缩进、括号、注释的输出效果。

## 任务清单
- [x] 实现基础 Printer 框架（缩进管理、Buffer）。
- [x] 实现所有 AST 节点的打印逻辑（Statement, Expression, Declaration）。
- [x] 实现运算符优先级判断，自动添加括号。
- [x] 实现注释打印逻辑（Leading/Trailing）。
- [x] 集成 Source Map 生成逻辑。
- [x] 编写 Round-trip 测试和快照测试。

## 验收标准
- ✅ **结构等价性**：`parse(generate(ast))` 得到的 AST 应与原 AST 结构一致（忽略位置信息）。
- ✅ **Source Map 可用**：生成的代码在浏览器/Node 中调试时，能正确映射回源码。
- ✅ **注释完整**：所有注释均被输出，且位置合理。
- ✅ **代码合法**：生成的代码无语法错误。

## 优先级
🔴 **MVP**（闭环的关键）

## 相关 Issue
- Issue 03: Parser（提供 AST）
- Issue 05: Edit API（产生修改后的 AST）
- Issue 11: 集成测试（验证全流程）

