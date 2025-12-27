# AST 编辑与不可变操作 API

## 需求
提供一套**不可变（Immutable）**的 AST 编辑 API，支持节点的增删改查。
核心特性是**作用域感知（Scope-Aware）**，在进行重命名或移动操作时，自动检查变量捕获（Capture）和遮蔽（Shadowing），防止破坏代码逻辑。

## Issue 元数据
- 类型：Feature（核心能力）
- 标签：ast-manipulation, immutable, scope-aware, mvp
- 负责人：@page
- 依赖：Issue 03（AST）、Issue 04（ScopeManager）
- 目标版本：v0.1.0（MVP）

## 技术要点

### 1. 不可变性设计（Immutability）
- **Path Copying / Structural Sharing**：当修改一个节点时，必须复制该节点及其所有祖先节点，直到根节点。未修改的分支保持引用共享。
- **API 风格**：
  ```javascript
  const newAst = edit.replace(ast, targetNode, replacementNode);
  ```

### 2. 核心原子操作（Atomic Operations）
- **Insert**：`insert(parent, index, node)`
- **Replace**：`replace(target, replacement)`
- **Remove**：`remove(target)`
- **Move**：`move(target, newParent, index)`
- **Rename**：`rename(bindingIdentifier, newName)`

### 3. 作用域感知（Scope Integration）
- **Rename Safety**：
  - 检查新名字是否在当前作用域或子作用域中已存在（冲突检测）。
  - 自动更新该 Binding 的所有 Reference。
- **Move Safety**：
  - 检查被移动的节点中引用的变量，在目标位置是否可见。
  - 检查是否会被目标位置的变量意外捕获（Capture）。

### 4. 批量编辑与事务（Batch / Transaction）
- 提供 `transform(ast, visitor)` 接口，允许一次遍历执行多个修改。
- 确保多次修改的顺序性和一致性。

## 交付物
- 🔧 Edit 模块
  - `Editor` 类或函数集
  - `Visitor` 遍历器
- 📚 API 文档与示例
- 🧪 测试套件
  - 基础增删改测试
  - **复杂重命名测试**（测试遮蔽、嵌套作用域）
  - **非法移动测试**（测试作用域逃逸）

## 任务清单
- [ ] 实现 AST 遍历器（Walker/Visitor）。
- [ ] 实现不可变更新机制（Path Copying）。
- [ ] 实现 `replace`, `remove`, `insert` 基础操作。
- [ ] 集成 ScopeManager，实现 `rename` 操作（含冲突检测）。
- [ ] 实现 `move` 操作的安全检查逻辑。
- [ ] 编写测试用例，覆盖各种边缘情况。

## 验收标准
- ✅ 所有编辑操作均返回新的 AST 根节点，原 AST 保持不变。
- ✅ 重命名操作能正确更新所有引用，且不破坏其他同名变量。
- ✅ 移动操作能识别作用域冲突并报错或警告。
- ✅ 编辑后的 AST 结构合法（符合 ESTree）。
- ✅ 性能可接受（避免全量深拷贝，使用结构共享）。

## 优先级
🔴 **MVP**（代码转换工具的基础）

## 相关 Issue
- Issue 04: ScopeManager（提供作用域分析支持）
- Issue 06: Codegen（将编辑后的 AST 转回代码）

