# QuickJSFlow - JavaScript 引擎设计文档

> 一个功能完整的 JS 引擎，支持**解析**、**编辑**、**代码生成**，旨在为代码转换、静态分析、自动化重构等场景提供基础设施。

## 🎯 项目定位

**不是什么**：
- ❌ 不是 V8/SpiderMonkey 等完整执行引擎
- ❌ 不支持 TypeScript 类型系统
- ❌ 不包含 JIT 编译或优化

**是什么**：
- ✅ JavaScript **语法解析** + **AST 操作** + **代码生成** 的完整工具链
- ✅ 专注于 **静态分析**、**代码编辑**、**转换工具** 的构建基础
- ✅ 对标 Babel、ESLint 的解析层，但包含更多编辑能力

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────────────┐
│          CLI / SDK / Plugin Interface                │
│        (Issue 08: CLI/SDK 与插件扩展)               │
└─────────────────┬──────────────────────────────────┘
                  │
        ┌─────────┼────────────┬─────────────┐
        │         │            │             │
        ↓         ↓            ↓             ↓
   ┌────────┐ ┌────────┐ ┌──────────┐ ┌──────────┐
   │ Edit   │ │Codegen │ │  CFG    │ │  Def-Use │
   │ API    │ │(06)    │ │ (07)✅  │ │ Chain    │
   │ (05)   │ │        │ │         │ │ (待实现) │
   └────┬───┘ └───┬────┘ └──┬──────┘ └──┬───────┘
        │         │         │          │
        └─────────┼─────────┴──────────┘
                  │
            ┌─────▼─────┐
            │ScopeManager│
            │   (04)✅  │
            │作用域链    │
            └─────┬─────┘
                  │
            ┌─────▼─────┐
            │  Parser   │
            │   (03)✅  │
            │ESTree AST │
            └─────┬─────┘
                  │
            ┌─────▼─────┐
```
            │  Lexer    │
            │   (02)    │
            │Token流    │
            └─────┬─────┘
                  │
            ┌─────▼──────────┐
            │ 源码 (String)   │
            └────────────────┘
```

---

## 📦 13 个 Issue 分类

### � 已完成 (9 个)

| 模块 | Issue | 关键职能 | 状态 |
|------|-------|--------|------|
| **需求与规划** | 01, 12, 13, 11 | 范围界定、语法矩阵、细化规范、集成测试 | ✅ |
| **解析层** | 02, 03, 04 | Lexer, Parser, ScopeManager | ✅ |
| **编辑层** | 05, 06 | AST 编辑 API, 代码生成 | ✅ |
| **质量** | 09 | CI/覆盖率/基准测试 | ✅ |
| **接口** | 08 | CLI/SDK | ✅ |

### 🟡 Phase 2 完成 (2 个)

| 模块 | Issue | 关键职能 | 状态 |
|------|-------|--------|------|
| **控制流分析** | 07 | CFG/数据流图 | ✅ **完成** |
| **性能优化** | 10 | 增量编辑与 Diff 算法 | 计划中 |

---

## 🚀 CLI 工具和插件系统

QuickJSFlow 提供了强大的命令行工具和可扩展的插件系统。

### 快速开始

```bash
# 构建
make clean && make

# 运行测试
./scripts/test-cli.sh

# 查看帮助
./build/quickjsflow
```

### 核心命令

```bash
# 解析 JavaScript 生成 AST
./build/quickjsflow parse input.js

# 生成代码
./build/quickjsflow generate input.js

# 语法检查
./build/quickjsflow check input.js

# 控制流图分析
./build/quickjsflow cfg function.js mermaid

# 应用插件转换
./build/quickjsflow run input.js --plugin remove-console
```

### 内置插件

- **remove-console** - 移除所有 `console.log()` 调用
- **remove-debugger** - 移除所有 `debugger` 语句

### 示例：清理生产代码

```bash
# 移除开发调试代码
./build/quickjsflow run src/app.js --plugin remove-console > dist/app.js
```

### 完整文档

- 📖 [完整用户指南](docs/CLI_PLUGIN_GUIDE.md)
- 📋 [快速参考](docs/cli-quick-reference.md)
- 🔧 [插件开发指南](docs/ISSUE_08_IMPLEMENTATION.md)
- 🇨🇳 [中文实现总结](docs/ISSUE_08_SUMMARY_CN.md)

---

## 📖 开发指南

### 查看完整执行计划
→ [完整 Issue 清单与执行计划](issues/00-完整Issue清单与执行计划.md)

### 从这里开始

1. **了解范围**：[Issue 01 - 产品愿景与范围基线](issues/01-产品愿景与范围基线.md)
2. **明确语法**：[Issue 12 - ESTree AST 语法完整性矩阵](issues/12-ESTree-AST语法完整性矩阵.md)
3. **作用域规范**：[Issue 13 - 作用域分析细化实现规范](issues/13-作用域分析细化实现规范.md)
4. **集成测试**：[Issue 11 - 集成测试与模块边界规范](issues/11-集成测试与模块边界规范.md)

### 模块实现路径

1. **Lexer**：[Issue 02](issues/02-词法分析与错误恢复-Lexer.md)
2. **Parser**：[Issue 03](issues/03-语法解析与ESTree-AST结构.md)
3. **ScopeManager**：[Issue 04](issues/04-作用域分析与符号表-ScopeManager.md)
4. **编辑 API**：[Issue 05](issues/05-AST编辑与不可变操作API.md)
5. **Codegen**：[Issue 06](issues/06-代码生成与SourceMap-格式化集成.md)
6. **CLI/SDK**：[Issue 08](issues/08-CLI-SDK与插件扩展接口.md)
7. **质量护栏**：[Issue 09](issues/09-质量与基准测试护栏.md)

### Phase 2 扩展（可选）

- **增量编辑**：[Issue 10](issues/10-增量编辑与Diff算法.md)
- **控制流图**：[Issue 07](issues/07-控制流-数据流图构建.md)

---

## 📊 关键特性总览

### CLI 工具特性

- ✅ 词法分析和语法解析
- ✅ AST 生成和代码生成
- ✅ 语法和作用域检查
- ✅ 控制流图分析（JSON/DOT/Mermaid）
- ✅ 可扩展的插件系统
- ✅ 多种输出格式支持

### 插件系统特性

- 🔌 基于访问者模式的 AST 转换
- 🎯 节点类型特定的转换器
- 🔄 支持节点增删改
- 📦 内置实用插件
- 🛠️ 易于扩展的架构

### 核心功能状态

| 特性 | 状态 | 说明 |
|------|------|------|
| **ECMAScript 阶段 1 支持** | 🔴 MVP | 基础语法 + 控制流 (85% 覆盖) |
| **注释保留与挂载** | 🔴 MVP | Leading/Trailing/Inner 注释 |
| **完整作用域分析** | 🔴 MVP | 6 种 MVP 作用域 + TDZ 检测 |
| **不可变编辑 API** | 🔴 MVP | Insert/Replace/Remove/Move/Rename |
| **SourceMap 生成** | 🔴 MVP | 位置映射完整性测试 |
| **错误恢复** | 🔴 MVP | 容错 L1/L2，部分 AST 输出 |
| **集成测试框架** | 🔴 MVP | 500+ 用例，Round-trip 验证 |
| **CLI 工具** | ✅ **已完成** | parse/generate/check/cfg/run 命令 |
| **插件系统** | ✅ **已完成** | 访问者模式 + 内置插件 |
| --- | --- | --- |
| **ES2021+ 现代特性** | 🟡 Phase 2 | 箭头函数、解构、async/await 等 |
| **插件转换流程
```
源码 → Parse → AST → Plugin Visitor → 转换后 AST → Codegen → 输出代码
```

### 编辑流程
```
原 AST
    ↓
[Edit API] Insert/Replace/Remove/Move/Rename
    ↓
新 AST (不可变，保留位置信息)
    ↓
[Codegen] → 目标源码 + SourceMap
```

### 验证流程 (Round-trip)
```
源码 → Parse → AST → Edit → 新 AST → Codegen → 目标源码 → Parse → 新 AST
↑                                                            ↓
└────────── 结构等价检查 ─────────────────────────────────────┘
```

---

## 💼 使用场景

### 1. 代码清理
```bash
# 移除生产环境中的 debug 代码
./build/quickjsflow run src/app.js --plugin remove-console > dist/app.js
```

### 2. 代码分析
```bash
# 检查代码质量
./build/quickjsflow check src/main.js

# 分析控制流
./build/quickjsflow cfg src/main.js dot | dot -Tpng > flow.png
```

### 3. 代码转换
```bash
# 解析、转换、生成
./build/quickjsflow parse input.js > ast.json
# 修改 AST...
./build/quickjsflow generate modified_ast.json > output.js
```

### 4. CI/CD 集成
```bash
# 在构建流程中自动清理代码
#!/bin/bash
for file in src/**/*.js; do
  ./build/quickjsflow run "$file" --plugin remove-console > "dist/${file}"
done
[ScopeManager] → Scope Chain + Symbol Table
```

### 编辑流程
```
原 AST
    ↓
[Edit API] Insert/Replace/Remove/Move/Rename
    ↓
新 AST (不可变，保留位置信息)
7. **可扩展插件系统**：基于访问者模式，易于创建自定义代码转换工具

---

## ⚡ 性能特点

- 🚀 快速解析和生成
- 💾 低内存占用
- 🎯 高效的 AST 遍历
- 📊 可扩展的插件架构
- 🔧 零依赖的 C 实现
    ↓
[Codegen] → 目标源码 + SourceMap
```

### 验证流程 (Round-trip)
```
源码 → Parse → AST → Edit → 新 AST → Codegen → 目标源码 → Parse → 新 AST
↑                                                            ↓
└────────── 结构等价检查 ─────────────────────────────────────┘
```

---

## 📈 进度追踪

**阶段状态**：设计完成 ✅，可开始实现 **完成：6个CLI命令 + 插件系统**

- [x] 需求澄清与架构设计（Issue 01）
- [x] 语法范围明确（Issue 12）
- [x] 作用域规范细化（Issue 13）
- [x] 模块边界定义（Issue 11）
- [ ] Lexer 实现（Issue 02）
- [ ] Parser 实现（Issue 03）
- [ ] ScopeManager 实现（Issue 04）
- [ ] 编辑 API 实现（Issue 05）
- [x] Codegen 实现（Issue 06）✅
- [x] CLI/SDK 实现（Issue 08）✅
- [x] 质量护栏建立（Issue 09）✅
- [x] 集成测试完成（Issue 11）✅
- [x] 控制流图实现（Issue 07）✅

---

## 💡 设计亮点

1. **容错设计**：三级容错策略（L1 词法、L2 语法、L3 作用域），确保部分解析输出
2. **不可变 API**：所有编辑操作返回新 AST，支持 Undo/Redo 与协作编辑
3. **作用域感知编辑**：重命名、移动等高阶操作自动检查变量捕获与遮蔽
4. **完整的注释保留**：Leading/Trailing/Inner 注释挂载与还原
5. **分阶段语法支持**：清晰的路线图，MVP 覆盖 85% 常见代码
6. **控制流分析**：完整的 CFG（控制流图）构建，支持 JSON/DOT/Mermaid 导出

---

## 🔥 CFG 模块使用示例

### 生成 JSON 格式的 CFG
```bash
./build/quickjsflow cfg examples/fib.js json
```

### 生成 Graphviz DOT 格式（可视化）
```bash
./build/quickjsflow cfg examples/fib.js dot | dot -Tpng > cfg.png
```

### 生成 Mermaid 格式（用于 Markdown）
```bash
./build/quickjsflow cfg examples/fib.js mermaid
```

### 代码中使用 CFG API
```c
// 构建 CFG
CFG *cfg = qjs_build_cfg(func_node, NULL, NULL);

// 导出为 JSON
char *json = qjs_cfg_to_json(cfg);
printf("%s\n", json);

// 释放资源
free(json);
qjs_cfg_free(cfg);
```

详见：[CFG 快速参考](CFG_QUICK_REFERENCE.md) 和 [CFG 实现文档](CFG_IMPLEMENTATION.md)

---

## 📚 相关资源

- **ESTree 规范**：https://github.com/estree/estree
- **ECMAScript 标准**：https://tc39.es/
- **Babel Parser**：参考实现
- **TypeScript Compiler**：参考作用域分析
- **CFG 文档**：[Control Flow Graph 实现](CFG_IMPLEMENTATION.md)

---

## 📝 贡献指南

按照 Issue 的优先级顺序实现，确保：

1. **设计先行**：实现前完成 Issue 审视，明确接口与验收标准
2. **单测覆盖**：每个模块 ≥ 80% 覆盖率
3. **集成验证**：通过 Issue 11 的集成测试
4. **文档完整**：API 文档、示例、限制说明

---

**最后更新**：2025-12-27  
**状态**：✅ MVP 完成 + Phase 2 部分完成（CFG 模块）
