# QuickJSFlow - 完整 Issue 清单与执行计划

## 📋 项目概览

**目标**：构建一个功能完整的 JavaScript 引擎（解析+编辑），支持 ESTree AST 操作、作用域分析、代码生成，并提供 CLI/SDK 接口。

**核心流程**：
```
源码 → Lexer → Parser → ScopeManager → Edit API → Codegen → 输出源码
                ↓                         ↓
             Token流                  作用域链
```

## 🧭 Issue 元数据（Epic）
- 类型：Epic（MVP 总览）
- 标签：planning, epic, mvp
- 目标版本：v0.1.0（MVP）
- 负责人：@page（可调整）
- 预计周期：3–4 周（含并行项）
- 相关文档：README、QUICK_REFERENCE、Issue 12/13

## � 项目结构与开发环境

### 目录组织
```
quickjsflow/
├── src/                          # 核心源码
│   ├── main.c                   # CLI 入口
│   ├── lexer.c                  # 词法分析实现
│   ├── parser.c                 # 语法分析实现
│   └── ast_print.c              # AST JSON 打印与内存管理
├── include/quickjsflow/          # 公开头文件
│   ├── lexer.h                  # Lexer API
│   ├── parser.h                 # Parser API
│   └── ast.h                    # AST 定义与构造器
├── test/                         # 集成测试（Issue 11）
│   ├── test_framework.h         # C 单测框架（断言、统计）
│   ├── test_integration.c       # Lexer+Parser 端到端测试
│   └── test_roundtrip.c         # Round-trip 验证（parse→print→parse）
├── examples/                     # 示例与烟雾测试
│   ├── sample.js                # 含注释、错误的复杂样本
│   ├── mini.js                  # 简单变量声明
│   ├── bad.js                   # 未闭合字符串（L1 容错）
│   └── badblock.js              # 未闭合块注释（L1 容错）
├── Makefile                      # C11 编译配置，产物归拢到 build/
├── .gitignore                    # 忽略 build/ 本地编译产物
└── issues/                       # 设计文档（本清单与各 Issue）
    ├── 00-完整Issue清单与执行计划.md
    ├── 01-产品愿景与范围基线.md
    ├── 02-词法分析与错误恢复-Lexer.md
    ├── 03-语法解析与ESTree-AST结构.md
    ├── 04-作用域分析与符号表-ScopeManager.md
    ├── 05-AST编辑与不可变操作API.md
    ├── 06-代码生成与SourceMap-格式化集成.md
    ├── 07-控制流-数据流图构建.md
    ├── 08-CLI-SDK与插件扩展接口.md
    ├── 09-质量与基准测试护栏.md
    ├── 10-增量编辑与Diff算法.md
    ├── 11-集成测试与模块边界规范.md
    ├── 12-ESTree-AST语法完整性矩阵.md
    └── 13-作用域分析细化实现规范.md
```

### 构建与测试命令
```bash
# 编译 CLI 工具
make

# 运行所有集成测试
make test

# 清理构建产物
make clean

# 快速测试 CLI
./build/quickjsflow lex examples/sample.js
./build/quickjsflow parse examples/mini.js
```

### 模块职责与依赖
| 模块 | 文件 | 职责 | 依赖 |
|------|------|------|------|
| **Lexer** | lexer.{c,h} | Token 流生成，L1 容错 | 无 |
| **Parser** | parser.{c,h}, ast.h, ast_print.c | AST 构建，Stage-1 语法，JSON 输出 | Lexer |
| **CLI** | main.c | `lex` 和 `parse` 命令 | Lexer, Parser |
| **Test** | test/*.c | Lexer+Parser 端到端与 Round-trip 验证 | Lexer, Parser |
| **ScopeManager** | scope.{c,h} (待实现) | 作用域链与符号表（Issue 04） | Parser |
| **Edit API** | edit.{c,h} (待实现) | 不可变编辑操作（Issue 05） | ScopeManager |
| **Codegen** | codegen.{c,h} (待实现) | AST→源码与 SourceMap（Issue 06） | Parser, Edit API |

## 🗺️ 路线图与里程碑（建议）
- Sprint 1（Week 1）：Issue 01, 12, 13（愿景/语法矩阵/作用域规范）
- Sprint 2（Week 2）：Issue 02, 03（Lexer/Parser 初版 + 基础测试）✅ 完成
- Sprint 3（Week 3）：Issue 04, 05（ScopeManager + 编辑 API）
- Sprint 4（Week 4）：Issue 06, 08, 11, 09（Codegen、CLI/SDK、集成测试、质量护栏）
- Phase 2（MVP 后）：Issue 10, 07（增量编辑、CFG 可选）

## ✅ Epic Definition of Done（MVP）
- 解析→编辑→生成→再解析 Round-trip 在 500+ 用例中结构等价
- 词法/语法错误具备 L1/L2 容错；注释保留、位置精确
- 作用域分析支持 Global/Module/Function/Block/Catch/For；TDZ与提升规则正确
- 不可变编辑 API（Insert/Replace/Remove/Move/Rename）可用且作用域安全
- Codegen 支持 SourceMap，命中率通过设定阈值；CLI/SDK 可运行
- CI 全绿、≥80% 覆盖率、性能基线报告产出

## 📌 任务清单（子 Issue，可直接作为 GitHub 子任务）
- [x] 01-产品愿景与范围基线（定义目标、容错、编辑原子操作）✅ 完成
- [x] 12-ESTree-AST 语法完整性矩阵（阶段划分与覆盖）✅ 完成
- [x] 13-作用域分析细化实现规范（规则与算法）✅ 完成
- [x] 02-词法分析与错误恢复-Lexer（TokenStream + L1 容错）✅ 完成
- [x] 03-语法解析与ESTree-AST结构（阶段1语法 + 注释挂载 + L2 容错）✅ 完成
- [x] 11-集成测试与模块边界规范（契约定义 + Round-trip）✅ 完成（基础框架）
- [ ] 04-作用域分析与符号表-ScopeManager（绑定/引用/遮蔽/TDZ）
- [ ] 05-AST编辑与不可变操作API（不可变 + 作用域感知）
- [ ] 06-代码生成与SourceMap-格式化集成（注释还原 + SourceMap）
- [ ] 08-CLI-SDK与插件扩展接口（基础命令 + SDK）
- [ ] 09-质量与基准测试护栏（CI/覆盖率/基准）
- [ ] 10-增量编辑与Diff算法（Phase 2）
- [ ] 07-控制流-数据流图构建（Phase 2，可选）

## 🔗 依赖关系图（简述）
- Parser（03）⇐ Lexer（02） & 语法矩阵（12）
- ScopeManager（04）⇐ Parser（03） & 作用域规范（13）
- Edit API（05）⇐ AST（03） & Scope（04）
- Codegen（06）⇐ AST（03） & Edit API（05）
- CLI/SDK（08）⇐ 02–06
- 集成测试（11）⇐ 02–06；覆盖全流程
- 质量护栏（09）⇐ 全模块并行
- 增量编辑（10）⇐ Lexer/Parser/Scope（02–04）
- CFG（07）⇐ AST（03） & Scope（04）

---

## 🎯 Issue 优先级分布

### 🔴 MVP 必须（阻塞项）

| # | Issue | 说明 | 依赖 |
|---|-------|------|------|
| **01** | 产品愿景与范围基线 | 需求澄清、架构决策 | 无 |
| **02** | Lexer - 词法分析与错误恢复 | Token 流生成 | 01 |
| **03** | Parser - 语法解析与 ESTree AST | AST 构建 | 01, 12 |
| **04** | ScopeManager - 作用域分析 | 作用域链与符号表 | 03, 13 |
| **05** | AST 编辑与不可变操作 API | 编辑能力（Insert/Replace/Move/Rename） | 03, 04 |
| **06** | Codegen - 代码生成与 SourceMap | 源码输出 | 03, 05 |
| **11** | 集成测试与模块边界规范 | 模块间契约、端到端测试 | 02-06 |
| **12** | ESTree AST 语法完整性矩阵 | 语法范围定义 | 01 |
| **13** | 作用域分析细化实现规范 | 作用域实现细节 | 01, 04 |
| **09** | 质量与基准测试护栏 | CI/覆盖率/基准 | 02-06 |

**MVP 预期**：9 个 Issue，交付基础的解析+编辑能力

---

### 🟡 Phase 2 优化（可选但推荐）

| # | Issue | 说明 | MVP 后实施 |
|---|-------|------|----------|
| **10** | 增量编辑与 Diff 算法 | 性能优化（长文件编辑） | ✅ |
| **07** | CFG 构建（高级） | 数据流分析（可选） | 🔄 探索 |
| **08** | CLI/SDK 与插件扩展 | 使用层接口（依赖 MVP） | ✅ |

---

## 📊 完整 Issue 清单与验收标准

### 01️⃣ **产品愿景与范围基线** 🔴 MVP

**目的**：确立项目范围、错误处理策略、编辑原子操作、MVP vs 延后范围

**关键补充**：
- 错误处理容错级别（L1/L2/L3）
- 编辑原子操作集（Insert/Replace/Remove/Move/Rename）
- ES 版本分阶段支持（阶段 1/2/3）

**验收**：
- ✅ 范围明确，相关 Issue 链接完整
- ✅ MVP vs Phase 2 清晰区分
- ✅ 团队共识达成

**状态**：✅ 已优化

---

### 02️⃣ **Lexer - 词法分析与错误恢复** 🔴 MVP

**目的**：实现流式 tokenizer，支持错误恢复（容错 L1）

**关键补充**：
- Token 类型完整枚举
- 流式接口定义（next/peek）
- 错误恢复场景覆盖（未闭合字符串、注释等）

**交付物**：
- Lexer 核心模块
- 100+ 单测（基础+错误恢复+注释）
- API 文档

**验收**：
- ✅ 所有 MVP token 类型通过测试
- ✅ 错误输入不中断流
- ✅ 注释保留完整

**状态**：✅ 已优化，等待实现

---

### 03️⃣ **Parser - 语法解析与 ESTree AST** 🔴 MVP

**目的**：构建 ESTree 兼容的完整 AST，包含位置信息与注释

**关键点**：
- 递归下降或表驱动解析
- 注释挂载（Leading/Trailing/Inner）
- 错误定位与恢复（容错 L2）
- 按 Issue 12 的阶段 1 语法支持

**交付物**：
- Parser 核心模块
- 快照测试（阶段 1 语法）
- 错误定位测试

**验收**：
- ✅ 阶段 1 语法全覆盖
- ✅ 注释正确挂载
- ✅ 错误信息精确到行列
- ✅ Round-trip 测试 500+ 用例

**依赖**：Issue 12（语法矩阵）

**状态**：等待 Issue 12 明确，然后实现

---

### 04️⃣ **ScopeManager - 作用域分析与符号表** 🔴 MVP

**目的**：构建作用域链与符号表，支持变量绑定、引用解析、重命名等

**关键点**：
- MVP 作用域类型：Global, Module, Function, Block, Catch, For
- 变量提升规则（var & function declaration）
- TDZ 检测与处理
- 引用解析（读/写）与遮蔽检测

**交付物**：
- ScopeManager 核心模块
- 分类单测（30+ 文件，覆盖各作用域/规则）
- Scope Dump 工具

**验收**：
- ✅ 所有 MVP 作用域类型准确识别
- ✅ 变量提升与 TDZ 规则正确
- ✅ 引用指向准确（考虑遮蔽）
- ✅ Dump 工具可用

**依赖**：Issue 13（细化规范）, Issue 03（AST）

**状态**：等待 Issue 13 明确，然后实现

---

### 05️⃣ **AST 编辑与不可变操作 API** 🔴 MVP

**目的**：提供原子编辑操作，返回不可变新 AST，集成作用域检查

**关键操作**：
- **Insert**：插入子节点（不需作用域感知）
- **Replace**：替换节点（检查变量捕获）
- **Remove**：删除节点（检查未使用）
- **Move**：移动节点（检查遮蔽）
- **Rename**：重命名标识符（必须作用域感知）

**交付物**：
- 编辑 API 模块
- 示例与单测
- 文档与限制说明

**验收**：
- ✅ 编辑后 AST 结构正确、位置连续
- ✅ Visitor/Transform API 可组合
- ✅ 作用域感知操作安全

**依赖**：Issue 04（ScopeManager），Issue 03（AST）

**状态**：等待 Issue 04，然后实现

---

### 06️⃣ **Codegen - 代码生成与 SourceMap** 🔴 MVP

**目的**：AST 转回源码，保留注释，输出 SourceMap

**关键点**：
- 可配置打印选项（缩进、括号等）
- 注释还原（Leading/Trailing）
- SourceMap 生成与映射覆盖率
- 再解析一致性（parse → print → parse）

**交付物**：
- Codegen 核心模块
- SourceMap 生成逻辑
- 格式化规则与快照测试
- 再解析一致性测试

**验收**：
- ✅ parse → print → parse 结构等价
- ✅ SourceMap 命中率测试通过
- ✅ 注释完整还原

**依赖**：Issue 03（AST）, Issue 05（编辑 API）

**状态**：等待 Issue 03/05，然后实现

---

### 07️⃣ **CFG 构建 - 控制流/数据流图** 🟡 Phase 2

**目的**：构建函数级 CFG，支持数据流标签（可选）

**关键点**：
- 基本块与边类型（顺序、条件、异常等）
- 覆盖 if/loop/try/async/await
- 结合 ScopeManager 进行 Def-Use 分析
- JSON 导出与可视化

**交付物**：
- CFG 构建器
- 典型结构单测
- 导出与可视化示例

**验收**：
- ✅ 典型控制流结构通过
- ✅ 导出结果可消费

**优先级**：Phase 2（MVP 后可选）

**依赖**：Issue 04（ScopeManager）, Issue 03（AST）

**状态**：MVP 后探索

---

### 08️⃣ **CLI/SDK 与插件扩展接口** 🔴 MVP

**目的**：提供命令行与编程接口，支持基础插件机制

**关键命令**：
- `parse` - 解析源文件
- `print` - 代码生成
- `edit` - 编辑脚本
- `cfg` - 导出控制流图

**交付物**：
- CLI 命令实现
- Node SDK 同步/流式接口
- 插件加载机制
- 示例工程

**验收**：
- ✅ CLI 示例可运行
- ✅ SDK 接口文档完整
- ✅ 插件示例可加载执行

**依赖**：Issue 02-06（核心模块）

**状态**：MVP 后期实现

---

### 09️⃣ **质量与基准测试护栏** 🔴 MVP

**目的**：建立 CI/覆盖率/性能基准、保证质量

**关键要素**：
- CI 流程（Jest/GitHub Actions）
- 覆盖率收集（目标 ≥ 80%）
- 性能基准（对标准 JS 引擎）
- 错误恢复基准（大文件、深嵌套、错误输入）
- 回归用例库

**交付物**：
- CI 配置文件（.github/workflows）
- 基准脚本与报告模板
- 覆盖率配置
- 回归用例集

**验收**：
- ✅ CI 全绿
- ✅ 覆盖率达成目标
- ✅ 性能基线产出
- ✅ 边界/错误恢复用例稳定

**依赖**：Issue 02-08（所有模块）

**状态**：MVP 期间并行建立

---

### 🔟 **增量编辑与 Diff 算法** 🟡 Phase 2

**目的**：支持增量编辑，优化长文件场景性能

**关键点**：
- Myers/Patience diff 计算变更范围
- Range-based 重解析（仅重解析变更部分）
- AST 节点复用与位置映射
- 作用域增量更新

**交付物**：
- Incremental Parser 模块
- Diff 引擎实现
- 增量更新策略单测
- 性能基准

**验收**：
- ✅ 行级编辑不触发全文重解析
- ✅ AST 节点引用稳定
- ✅ 位置映射正确
- ✅ 延迟在可接受范围

**优先级**：Phase 2（MVP 用全量重解析）

**依赖**：Issue 02-04（基础模块）

**状态**：MVP 后优化

---

### 1️⃣1️⃣ **集成测试与模块边界规范** 🔴 MVP

**目的**：定义模块间 API 契约，确保端到端流程正确性

**关键内容**：
- 模块接口规范（Lexer→Parser、Parser→Scope、...）
- Round-trip 测试（parse → edit → print → parse）
- 快照与回归用例库（500+ 用例）
- 模块 Mock/Stub 工具

**交付物**：
- 接口定义文档（.d.ts 或 markdown）
- 集成测试框架（Jest 配置）
- 快照基线文件
- Mock 工具类

**验收**：
- ✅ 各模块单测 + 集成测试全绿
- ✅ Round-trip 500+ 用例通过
- ✅ 快照稳定、版本可追踪
- ✅ 接口文档完整

**优先级**：MVP 期间并行（设计评审+开发期间）

**依赖**：Issue 01-09 的设计完成

**状态**：贯穿 MVP 全程

---

### 1️⃣2️⃣ **ESTree AST 语法完整性矩阵** 🔴 MVP

**目的**：明确语法支持范围，指导 Parser 分阶段实现

**关键矩阵**：
- **阶段 1（MVP）**：基础特性（变量、函数、控制流、模块）
  - 85% 覆盖常见代码
- **阶段 2（Phase 2）**：现代特性（箭头函数、解构、async/await 等）
  - 95% 覆盖现代代码
- **阶段 3+**：可选特性（Generator、动态 import、可选链等）

**交付物**：
- 完整的语法支持矩阵表
- 阶段性单测集合（30+ 文件）
- 语法实现优先级与迁移指南

**验收**：
- ✅ 矩阵明确已实现/规划/排除的特性
- ✅ 各阶段语法通过相应单测
- ✅ 文档可用于指导新语法添加

**优先级**：MVP 前期明确（指导 Parser 实现）

**依赖**：Issue 01（需求明确）

**状态**：已完成，指导 Issue 03

---

### 1️⃣3️⃣ **作用域分析细化实现规范** 🔴 MVP

**目的**：细化 Issue 04 的设计，提供明确的实现路线

**关键内容**：
- 作用域类型完整表（优先级区分）
- 变量提升规则详解（var/function/let/const）
- TDZ 处理方案
- 引用解析算法
- 遮蔽检测与预定义全局对象

**交付物**：
- 详细实现规范文档
- 分类单测套件（10+ 文件）
- Scope Dump/可视化工具
- 准确性指标测试

**验收**：
- ✅ 支持 MVP 所有作用域类型
- ✅ 变量提升与 TDZ 准确
- ✅ 引用解析正确（考虑遮蔽）
- ✅ 工具可用于调试

**优先级**：MVP 前期明确（指导 ScopeManager 实现）

**依赖**：Issue 01, 04（需求明确）

**状态**：已完成，指导 Issue 04

---

## 🔗 依赖关系图

```
01 (需求澄清)
  ├─→ 12 (语法矩阵)
  │    └─→ 03 (Parser)
  │         └─→ 05 (Edit API)
  │              └─→ 08 (CLI/SDK)
  ├─→ 13 (作用域规范)
  │    └─→ 04 (ScopeManager)
  │         └─→ 05 (Edit API)
  │              └─→ 08 (CLI/SDK)
  ├─→ 02 (Lexer)
  │    └─→ 03 (Parser)
  │         └─→ 06 (Codegen)
  └─→ 11 (集成测试) ← 贯穿 02-09
  └─→ 09 (质量护栏) ← 贯穿 02-08
  └─→ 07 (CFG) [可选，Phase 2]
  └─→ 10 (增量编辑) [可选，Phase 2]
```

---

## 📅 建议实施顺序（MVP 阶段）

### 第一阶段：需求与设计（1-2 周）
1. **Issue 01**：需求澄清与架构决策
2. **Issue 12**：语法矩阵确定
3. **Issue 13**：作用域实现规范
4. **Issue 11**：模块接口契约定义

### 第二阶段：核心模块实现（4-6 周）
5. **Issue 02**：Lexer 实现
6. **Issue 03**：Parser 实现（按 Issue 12 分阶段）
7. **Issue 04**：ScopeManager 实现（按 Issue 13）
8. **Issue 05**：编辑 API 实现

### 第三阶段：高层模块与集成（2-3 周）
9. **Issue 06**：Codegen 实现
10. **Issue 08**：CLI/SDK 实现
11. **Issue 09**：质量护栏建立
12. **Issue 11**：集成测试完成

### Phase 2（可选，稍后）
- **Issue 10**：增量编辑优化
- **Issue 07**：CFG 数据流分析

---

## ✅ MVP 验收标准（整体）

| 层面 | 标准 |
|------|------|
| **功能** | ✅ 支持阶段 1 语法的解析+编辑+代码生成 |
| **容错** | ✅ 容错 L1+L2，错误输入不中断流 |
| **测试** | ✅ 整体覆盖率 ≥ 80%，集成测试 500+ 用例 |
| **性能** | ✅ 与标准 JS 引擎对标（可不优化） |
| **文档** | ✅ API 完整、Issue 清单清晰、示例可运行 |
| **架构** | ✅ 模块独立、接口清晰、易于扩展 |

---

## 📌 关键决策与权衡

| 决策 | 选项 | 建议 |
|------|------|------|
| **增量编辑** | MVP 做 vs Phase 2 | **Phase 2**（MVP 用全量重解析） |
| **CFG 构建** | MVP 做 vs Phase 2+ | **Phase 2+**（可选，高级用户） |
| **TypeScript** | 支持 vs 不支持 | **不支持**（超出范围） |
| **JSDoc** | 支持 vs 不支持 | **Phase 2**（保留空间，暂不实现） |
| **错误恢复级别** | L1/L2/L3 都做 vs 分阶段 | **L1+L2 在 MVP，L3 在 Phase 2** |

---

## 🚀 快速导航

- 📄 [Issue 01 - 产品愿景与范围基线](01-产品愿景与范围基线.md)
- 🔤 [Issue 02 - Lexer](02-词法分析与错误恢复-Lexer.md)
- 🌳 [Issue 03 - Parser](03-语法解析与ESTree-AST结构.md)
- 🔗 [Issue 04 - ScopeManager](04-作用域分析与符号表-ScopeManager.md)
- ✏️ [Issue 05 - Edit API](05-AST编辑与不可变操作API.md)
- 📝 [Issue 06 - Codegen](06-代码生成与SourceMap-格式化集成.md)
- 📊 [Issue 07 - CFG](07-控制流-数据流图构建.md)
- 💻 [Issue 08 - CLI/SDK](08-CLI-SDK与插件扩展接口.md)
- 🛡️ [Issue 09 - Quality](09-质量与基准测试护栏.md)
- 🔄 [Issue 10 - 增量编辑与 Diff](10-增量编辑与Diff算法.md)
- 🧪 [Issue 11 - 集成测试](11-集成测试与模块边界规范.md)
- 📋 [Issue 12 - 语法矩阵](12-ESTree-AST语法完整性矩阵.md)
- 🔍 [Issue 13 - 作用域细化](13-作用域分析细化实现规范.md)

---

**状态**：✅ 完整的 Issue 体系已就位，可开始设计评审与实现规划

