# 作用域分析与符号表 ScopeManager

## 需求
在 AST 基础上构建作用域链（Scope Chain）与符号表（Symbol Table），解析变量定义与引用关系。这是实现安全代码编辑（如重命名）和深度代码分析（如数据流）的基础。

**详细实现规范见 Issue 13：作用域分析细化实现规范**

## 📁 项目结构与开发环境

参考根目录 [issues/00-完整Issue清单与执行计划.md](00-完整Issue清单与执行计划.md) 的**项目结构**与**构建与测试命令**章节。关键路径：
- 源码：`src/scope.c`, `include/quickjsflow/scope.h`（待实现）
- 测试：`test/test_scope.c`（待实现）
- 依赖：Issue 03（Parser）产出的 AST
- 构建：`make && make test`
- 集成：Issue 11（集成测试）框架已就位

## 技术要点

### MVP 作用域类型（Issue 13 优先级区分）
- **Global**：全局作用域
- **Module**：ESM 模块顶层
- **Function**：函数级作用域
- **Block**：块级作用域（if/for/while 等）
- **Catch**：catch 块作用域
- **For**：for 循环初始化作用域

### 变量提升与 TDZ（Issue 13 详细规则）
- **var 提升**：声明提升，初值 undefined（函数作用域）
- **function 声明提升**：整个函数声明提升
- **let/const TDZ**：块级作用域，进入 TDZ；访问前会报错（可检测）

### 引用解析与绑定（Issue 13 细化）
- 建立 Identifier 引用到 Binding 的链接
- 区分读/写引用
- 处理全局预定义对象（Phase 2）
- 识别隐式全局变量

### 遮蔽检测（Issue 13）
- 检测变量遮蔽（shadowing）情况
- 可视化遮蔽关系

## 交付物
- 🔧 ScopeManager 核心模块
  - Scope 数据结构与作用域链
  - Binding 与 Reference 管理
  - 查询接口（resolveBinding / getScope）
- 🧪 分类单测套件（详见 Issue 13）
  - MVP 作用域覆盖
  - 提升与 TDZ 规则
  - 遮蔽检测
  - 引用解析准确性
- 🔍 调试工具
  - Scope Dump（字符串/JSON 导出）
  - 可视化（可选）
- 📄 API 文档

## 验收标准
- ✅ 准确识别 MVP 的所有作用域结构
- ✅ 变量提升规则正确处理（var & function declaration）
- ✅ TDZ 检测准确（嵌套、复杂场景）
- ✅ 变量引用能正确指向其 binding（考虑 Shadowing）
- ✅ 区分读/写引用
- ✅ Dump 工具可用于调试
- ✅ 集成 Issue 11（端到端测试）与 Issue 05（编辑操作的作用域检查）

## 实现状态
✅ **已完成** (2025-12-27)
- 实现：src/scope.c (824行)
- 测试：test_scope.c 47个测试全部通过
- 功能：作用域链、符号表、TDZ检测、遮蔽分析、引用解析

## 优先级
🔴 **MVP**（Issue 05 编辑 API 的核心依赖）

## 相关 Issue
- **Issue 13**：作用域分析细化实现规范（本 Issue 的完整设计文档）
- Issue 03：Parser（提供完整 AST）
- Issue 05：编辑 API（需要作用域感知）
- Issue 11：集成测试（作用域准确性验证）
- Issue 07：CFG 构建（需要作用域信息进行 Def-Use 分析）
