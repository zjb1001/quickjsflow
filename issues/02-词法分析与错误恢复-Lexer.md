# 词法分析与错误恢复 Lexer

## 需求
实现可流式的 tokenizer，支持注释、字符串、模板字面量、正则字面量；提供错误恢复策略（在异常输入下继续产出 token 流）。根据 Issue 01 的容错级别 L1，Lexer 应支持在词法错误时输出有效的 token 流。

## Issue 元数据
- 类型：Feature（核心管线）
- 标签：lexer, parsing, fault-tolerance, mvp
- 负责人：@page（或指定实现者）
- 依赖：Issue 01（容错策略）、Issue 12（语法矩阵）
- 目标版本：v0.1.0（MVP）

## 📁 项目结构与开发环境

参考根目录 [issues/00-完整Issue清单与执行计划.md](00-完整Issue清单与执行计划.md) 的**项目结构**与**构建与测试命令**章节。关键路径：
- 源码：`src/lexer.c`, `include/quickjsflow/lexer.h`
- 测试：`test/test_integration.c`（Lexer+Parser 端到端）
- 示例：`examples/` 目录含容错测试用例
- 构建：`make` 编译，`make test` 运行测试

## 技术要点

### 核心 Token 类型（基于 ESTree 与常见编程语言惯例）
| Token 类型 | 示例 | 优先级 |
|----------|------|----------|
| Keyword | `var`, `function`, `if`, `async` 等 | 🔴 |
| Identifier | `myVar`, `_private`, `$jquery` | 🔴 |
| Literal | 数字、字符串、正则、模板 | 🔴 |
| Operator | `+`, `-`, `&&`, `?.`, `??` 等 | 🔴 |
| Punctuation | `{`, `}`, `(`, `)`, `;` 等 | 🔴 |
| Comment | `// ...` 或 `/* ... */` | 🔴 |
| Whitespace | 空格、制表符、换行 | 🔴 |

### 流式接口
```javascript
interface TokenStream {
  next(): Token | EOF          // 获取下一个 token
  peek(): Token | EOF          // 预看下一个 token（不消费）
}

interface Token {
  type: TokenType
  value: string
  loc: { start: { line, column }, end: { line, column } }
  isError?: boolean  // 标记错误 token
}
```

### 错误恢复策略（容错 L1）

场景 1：未闭合字符串 → 读到行尾后继续
场景 2：无效转义序列 → 标记为 Error，继续解析
场景 3：未闭合注释 → 消费到文件尾，标记为 Error
场景 4：未闭合正则字面量 → 降级处理或错误标记

### 辅助工具与可视化
- Token Dump：输出 token 流供调试（含错误标记）
- 性能计时包装：记录扫描耗时与内存占用

### 性能约束
- 流式扫描 1MB 文件：< 100ms（对标准 JS 引擎基准）
- 内存占用：≤ 源文件大小 × 3
- 不使用正则表达式（直接字符处理）

## 交付物
- 🔧 Lexer 核心模块（TokenStream 实现）
- 📚 Token 类型枚举与定义
- 🧪 单测套件（100+ 用例）
  - 基础 token 覆盖
  - 错误恢复场景
  - 注释与特殊字面量
- 📄 API 文档（公共接口）

## 任务清单（实现步骤）
- [ ] 定义 `TokenType` 枚举与 `Token` 结构（含 `loc` 与 `isError`）
- [ ] 实现字符级扫描器（避免大正则），支持换行与列计数
- [ ] 实现注释、字符串、模板字面量、正则字面量的识别
- [ ] 实现流式接口 `next/peek` 与 EOF 处理
- [ ] 实现 L1 错误恢复策略并覆盖典型场景
- [ ] 提供 Token Dump 工具与性能计时包装
- [ ] 编写 100+ 单测（基础/错误恢复/注释/边界），集成 CI
- [ ] 文档化公共接口与错误标记约定

## 验收标准
- ✅ 所有 MVP 阶段的 token 类型通过测试
- ✅ 错误 token 准确识别且流继续输出（无异常中断）
- ✅ 注释保留完整（用于 Parser 挂载）
- ✅ 位置信息精确到行列
- ✅ 集成 Issue 11（端到端 Round-trip 测试）
- ✅ 公开 API 文档化

## Definition of Done
- 单测与 CI 全绿，覆盖率达到既定目标（≥80% 针对 Lexer）
- 与 Parser（Issue 03）联调通过基础语法用例
- Token Dump 输出与性能基线报告可用

## 优先级
🔴 **MVP**（Pipeline 基础）

## 相关 Issue
- Issue 03: Parser（消费 Token 流）
- Issue 11: 集成测试（Lexer + Parser 协作）
- Issue 12: 语法矩阵（指导支持的 token 类型）
