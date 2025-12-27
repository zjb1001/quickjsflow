# QuickJSFlow - 快速参考卡片

## 📍 项目地图

```
QuickJSFlow/
├── README.md                          ← 项目总览（从这里开始）
├── DESIGN_COMPLETION_REPORT.md        ← 设计完成报告
├── issues/
│   ├── 00-完整Issue清单与执行计划.md   ← 全景视图 + 时间表
│   ├── 01-产品愿景与范围基线.md        ← MVP vs Phase 2
│   ├── 02-词法分析与错误恢复-Lexer.md
│   ├── 03-语法解析与ESTree-AST结构.md
│   ├── 04-作用域分析与符号表-ScopeManager.md
│   ├── 05-AST编辑与不可变操作API.md
│   ├── 06-代码生成与SourceMap-格式化集成.md
│   ├── 07-控制流-数据流图构建.md       ← [可选] Phase 2+
│   ├── 08-CLI-SDK与插件扩展接口.md
│   ├── 09-质量与基准测试护栏.md
│   ├── 10-增量编辑与Diff算法.md        ← [可选] Phase 2
│   ├── 11-集成测试与模块边界规范.md
│   ├── 12-ESTree-AST语法完整性矩阵.md
│   └── 13-作用域分析细化实现规范.md
└── src/                               ← [待实现] 源码目录
```

---

## 🎯 核心概念速览

### 系统输入/输出
```
输入：JavaScript 源代码（String）
  ↓
处理：Lexer → Parser → ScopeManager → Edit API → Codegen
  ↓
输出：修改后的源代码 + SourceMap + 作用域信息
```

### 关键特性
| 特性 | MVP | Phase 2 | 排除 |
|------|-----|---------|------|
| ES2020 基础语法 | ✅ | | |
| 现代特性（箭头/async） | | ✅ | |
| 注释保留 | ✅ | | |
| 作用域分析 | ✅ | | |
| 不可变编辑 API | ✅ | | |
| 增量编辑优化 | | ✅ | |
| 数据流图 | | ✅ | |
| TypeScript | | | 🚫 |

---

## 📊 Issue 优先级速查

### 🔴 必须（MVP）
```
1️⃣ Issue 01: 需求澄清
2️⃣ Issue 12: 语法矩阵 } 设计前置
3️⃣ Issue 13: 作用域规范
4️⃣ Issue 11: 集成测试

5️⃣ Issue 02: Lexer       } 核心实现
6️⃣ Issue 03: Parser       }（按顺序）
7️⃣ Issue 04: ScopeManager }
8️⃣ Issue 05: Edit API     }
9️⃣ Issue 06: Codegen      }

🔟 Issue 08: CLI/SDK   } 上层应用
1️⃣1️⃣ Issue 09: 质量护栏
```

### 🟡 可选（Phase 2）
```
Issue 10: 增量编辑优化
Issue 07: CFG 数据流
```

---

## ⏱️ 实施时间表

### 快速版（2 人团队，16 周）
```
Week 1-2:   设计评审 + 环境准备
Week 3-4:   Lexer (Issue 02)
Week 5-6:   Parser (Issue 03)
Week 7-8:   ScopeManager (Issue 04)
Week 9-10:  Edit API (Issue 05)
Week 11-12: Codegen (Issue 06)
Week 13-14: CLI/SDK + 质量护栏 (Issues 08/09)
Week 15-16: 集成测试 + 文档 (Issue 11)
```

### 最小时间表（3 人团队，并行）
```
Week 1-2:   设计评审
Week 3-6:   并行实现 02/03/04/05/06
Week 7-8:   并行实现 08/09/11
Week 9-10:  集成与优化
```

---

## 🚀 快速开始（开发者）

### Step 1: 理解整体架构（1 小时）
```
1. 读 README.md（5 分钟）
2. 读 Issue 00（完整 Issue 清单）（15 分钟）
3. 读 Issue 01（产品愿景）（15 分钟）
4. 快速浏览其他 Issue 标题（10 分钟）
5. 查看本文件：快速参考卡片（15 分钟）
```

### Step 2: 了解你的任务（15 分钟）
```
1. 找到你的 Issue（e.g., Issue 02 Lexer）
2. 读完整的 Issue 文档
3. 列出所有验收标准
4. 理解依赖关系
```

### Step 3: 准备开发（1 小时）
```
1. 初始化项目结构
2. 配置构建/测试工具
3. 创建单测框架
4. 准备 mock 和 fixture
```

### Step 4: 实现（按 Issue 指导）
```
1. 实现核心模块
2. 编写单测（≥80% 覆盖）
3. 运行集成测试（Issue 11）
4. 提交 PR 等待 review
```

---

## 📋 验收标准快速查

### Lexer (Issue 02)
- ✅ 所有 MVP token 类型通过测试
- ✅ 错误恢复（容错 L1）生效
- ✅ 100+ 单测通过

### Parser (Issue 03)
- ✅ 阶段 1 语法全支持
- ✅ 注释正确挂载
- ✅ 快照测试稳定

### ScopeManager (Issue 04)
- ✅ 6 种 MVP 作用域准确
- ✅ 变量提升规则正确
- ✅ TDZ 检测有效

### Edit API (Issue 05)
- ✅ 5 个原子操作可用
- ✅ 不可变性保证
- ✅ 作用域检查集成

### Codegen (Issue 06)
- ✅ Parse → Print → Parse 等价
- ✅ SourceMap 映射完整
- ✅ 注释完全还原

### CLI/SDK (Issue 08)
- ✅ 4 个核心命令可用
- ✅ 示例工程可运行
- ✅ 文档完整

### 质量护栏 (Issue 09)
- ✅ CI 全绿
- ✅ 覆盖率 ≥ 80%
- ✅ 基准测试建立

### 集成测试 (Issue 11)
- ✅ 500+ 用例通过
- ✅ Round-trip 验证通过

---

## 🔍 关键决策参考

### 问：为什么阶段化语法支持？
**答**：聚焦 MVP，85% 的代码用 20% 的语法。Phase 2 再加现代特性。

### 问：为什么要作用域分析？
**答**：重命名、移动等编辑操作需要知道变量作用域，确保正确性。

### 问：为什么不支持 TypeScript？
**答**：超出范围。JS 引擎应先做好，TS 可作为后续独立专项。

### 问：增量编辑为什么 Phase 2？
**答**：MVP 用全量重解析已足够。Phase 2 优化长文件场景。

### 问：如何处理错误代码？
**答**：三级容错（词法 L1、语法 L2、作用域 L3），力求输出最大可用 AST。

---

## 🛠️ 常用 Issue 链接

### 我是 Lexer 开发者
→ [Issue 02](issues/02-词法分析与错误恢复-Lexer.md)
→ 依赖：Issue 01, 12
→ 被依赖：Issue 03

### 我是 Parser 开发者
→ [Issue 03](issues/03-语法解析与ESTree-AST结构.md)
→ 依赖：Issue 01, 12, 02
→ 被依赖：Issue 04, 05, 06, 07

### 我是 ScopeManager 开发者
→ [Issue 04](issues/04-作用域分析与符号表-ScopeManager.md)
→ 依赖：Issue 03, 13
→ 被依赖：Issue 05, 07

### 我是 Edit API 开发者
→ [Issue 05](issues/05-AST编辑与不可变操作API.md)
→ 依赖：Issue 03, 04
→ 被依赖：Issue 06, 08

### 我是 Codegen 开发者
→ [Issue 06](issues/06-代码生成与SourceMap-格式化集成.md)
→ 依赖：Issue 03, 05
→ 被依赖：Issue 08, 11

### 我负责测试/质量
→ [Issue 11 集成测试](issues/11-集成测试与模块边界规范.md)
→ [Issue 09 质量护栏](issues/09-质量与基准测试护栏.md)
→ 依赖：Issue 02-08

### 我是项目经理
→ [Issue 00 执行计划](issues/00-完整Issue清单与执行计划.md)
→ [Issue 01 需求基线](issues/01-产品愿景与范围基线.md)

---

## 📞 常见问题

**Q: 我应该从哪个 Issue 开始？**
A: 如果你是：
- 新手：读 README → Issue 01 → Issue 12 → Issue 13
- 开发者：读 Issue 01 → 找你的 Issue → 开始编码
- 测试：读 Issue 11 → Issue 09
- PM：读 Issue 00 + Issue 01

**Q: 为什么有这么多 Issue？**
A: 每个 Issue 独立、完整、可追踪。便于并行开发和质量控制。

**Q: 可以跳过某些 Issue 吗？**
A: 不建议。Issue 12/13 是设计基础。Issue 02-06 按顺序实现。08/09/11 最后收尾。

**Q: 时间紧张，能加快吗？**
A: 可以：
- 增加人力（并行实现 02/03/04/05/06）
- 减少 Phase 2 需求
- 简化初期测试（后补全）

**Q: 完成后可以扩展吗？**
A: 完全可以！所有设计都预留了扩展空间（Phase 2, Phase 3）。

---

## 📚 文档导航

```
快速查看：
  ├─ 本文件（快速参考卡片）
  ├─ README.md（项目总览）
  └─ DESIGN_COMPLETION_REPORT.md（设计报告）

深入学习：
  ├─ Issue 00（完整 Issue 清单 + 执行计划）
  ├─ Issue 01（需求与范围）
  ├─ Issue 12（语法支持矩阵）
  └─ Issue 13（作用域实现规范）

开发指南：
  ├─ Issue 02-06（核心模块）
  ├─ Issue 08-09（上层应用与质量）
  └─ Issue 11（集成测试）

进阶主题：
  ├─ Issue 07（CFG 数据流）
  └─ Issue 10（增量编辑优化）
```

---

## ✅ 团队检查清单

在启动实现前，确保：

- [ ] 所有成员读过 README.md
- [ ] 所有成员理解 Issue 01（需求）
- [ ] 各模块负责人读过对应 Issue 全文
- [ ] 确认了时间表和里程碑
- [ ] 建立了 CI/CD 基础设施
- [ ] 明确了 PR review 流程
- [ ] 准备好了开发环境

---

## 🎯 核心指标

成功的 MVP 应该达到：

| 指标 | 目标 | 验证方法 |
|------|------|--------|
| 功能完成度 | 100% 阶段 1 语法 | Issue 12 验收 |
| 测试覆盖率 | ≥ 80% | Issue 09 报告 |
| 集成测试 | 500+ 用例通过 | Issue 11 验收 |
| 文档完整度 | API 文档 + 示例 | Issue 08 交付 |
| 可用性 | CLI 命令可运行 | Issue 08 演示 |

---

**最后更新**：2025-12-27  
**版本**：1.0（设计完成）  
**下一版本**：0.1-MVP（实现完成）

有问题？查看对应的 Issue 或咨询项目经理！🚀

