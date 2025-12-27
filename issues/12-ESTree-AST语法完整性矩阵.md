# ESTree AST 语法完整性矩阵

## 需求
明确定义支持的 ECMAScript 语法子集和分阶段实现路线，避免散乱实现。
本文档是语法范围的**规范性参考**，确保解析器逐步达成覆盖目标。

## 技术要点

### 分阶段实现路线

#### 📌 **阶段 1（MVP）：Essential Features**
关键词：基础的编辑需求必须支持

| 语法特性 | 说明 | ESTree 节点类型 | 优先级 |
|---------|------|----------------|-------|
| **变量声明** | var/let/const | VariableDeclaration | 🔴 |
| **标识符与基础字面量** | 数字、字符串、布尔、null、undefined | Literal/Identifier | 🔴 |
| **对象字面量** | { key: value } | ObjectExpression | 🔴 |
| **数组字面量** | [ ... ] | ArrayExpression | 🔴 |
| **成员访问** | obj.prop / obj[key] | MemberExpression | 🔴 |
| **函数声明** | function foo() {} | FunctionDeclaration | 🔴 |
| **函数表达式** | const f = function() {} | FunctionExpression | 🔴 |
| **调用表达式** | foo() | CallExpression | 🔴 |
| **二元运算** | a + b, a && b 等 | BinaryExpression | 🔴 |
| **一元运算** | -x, !x, typeof x 等 | UnaryExpression | 🔴 |
| **赋值** | a = b, a += b 等 | AssignmentExpression | 🔴 |
| **条件语句** | if/else | IfStatement | 🔴 |
| **循环语句** | while/do-while/for | WhileStatement, ForStatement | 🔴 |
| **switch 语句** | switch/case/default | SwitchStatement | 🔴 |
| **try/catch/finally** | 异常处理 | TryStatement | 🔴 |
| **return/break/continue** | 流程控制 | ReturnStatement, BreakStatement | 🔴 |
| **块语句** | { ... } | BlockStatement | 🔴 |
| **程序和模块** | Program 根节点 | Program | 🔴 |
| **import 声明** | import x from 'y' | ImportDeclaration | 🔴 |
| **export 声明** | export { x } / export default | ExportDeclaration | 🔴 |
| **单行/块注释** | // 和 /* */ | Comment (非标准) | 🔴 |

**预期覆盖**：85% 的常见 JS 代码

---

#### 🟡 **阶段 2（Phase 2）：Modern Features**
关键词：ES6+ 特性，扩展编辑支持范围

| 语法特性 | 说明 | ESTree 节点类型 | 优先级 |
|---------|------|----------------|-------|
| **箭头函数** | () => {} | ArrowFunctionExpression | 🟡 |
| **模板字面量** | \`hello ${x}\` | TemplateLiteral | 🟡 |
| **展开运算符** | ...arr / ...obj | SpreadElement | 🟡 |
| **对象简写** | { x } 或 { x() {} } | PropertyDefinition | 🟡 |
| **默认参数** | function f(x = 1) {} | AssignmentPattern | 🟡 |
| **解构赋值** | const { x, y } = obj | ObjectPattern/ArrayPattern | 🟡 |
| **for-of 循环** | for (const x of arr) {} | ForOfStatement | 🟡 |
| **for-in 循环** | for (const x in obj) {} | ForInStatement | 🟡 |
| **Classes（基础）** | class Foo {} | ClassDeclaration | 🟡 |
| **Class 方法与属性** | constructor / method | MethodDefinition | 🟡 |
| **Class 继承** | extends / super | 扩展 ClassDeclaration | 🟡 |
| **async/await** | async function / await | FunctionExpression + AwaitExpression | 🟡 |
| **Promise** | 解析但不执行 | CallExpression (Promise) | 🟡 |
| **剩余参数** | function f(...args) {} | RestElement | 🟡 |

**预期覆盖**：95% 的现代 JS 代码（不含 TypeScript）

---

#### 🟢 **阶段 3（Phase 3+）：Advanced & Experimental**
关键词：高级特性与在野特性，可选支持

| 语法特性 | 说明 | 状态 | 优先级 |
|---------|------|------|-------|
| **Generator 函数** | function* / yield | TC39 Finalized | 🟢 |
| **动态 import()** | import('module') | TC39 Finalized | 🟢 |
| **可选链（Optional Chaining）** | obj?.prop?.method?.() | TC39 Finalized | 🟢 |
| **逻辑赋值** | a ??= b / a &&= b | TC39 Finalized | 🟢 |
| **Nullish Coalescing** | a ?? b | TC39 Finalized | 🟢 |
| **BigInt 字面量** | 1n, 0x1n 等 | TC39 Finalized | 🟢 |
| **WeakMap/WeakSet** | 解析声明 | TC39 Finalized | 🟢 |
| **Proxy/Reflect** | 解析但无语义 | TC39 Finalized | 🟢 |
| **符号（Symbol）** | Symbol('x') | TC39 Finalized | 🟢 |

**预期覆盖**：99% 的标准 JS 代码

---

### ❌ **不支持清单（明确排除）**

| 语法特性 | 原因 |
|---------|------|
| **TypeScript 类型语法** | 超出范围；如需支持需单独专项 |
| **JSX** | 可作为未来扩展，但不在 MVP 范围 |
| **装饰器（@decorator）** | TC39 Stage 3，需要等待语法稳定 |
| **管道运算符（\|>）** | TC39 Stage 2，实验性特性 |
| **模式匹配** | TC39 尚未标准化 |

---

## 交付物

### 📊 完整的语法支持矩阵表（markdown）
- 分阶段列表
- ESTree 节点对应
- 优先级与状态标记

### 🧪 阶段性语法单测套件
```
tests/
  syntax/
    phase1/
      variables.test.js
      literals.test.js
      functions.test.js
      statements.test.js
      modules.test.js
    phase2/
      arrows.test.js
      destructuring.test.js
      classes.test.js
      async.test.js
    phase3/
      generators.test.js
      optionalChaining.test.js
```

- 每个单测文件：20-50 个用例
- 覆盖正常情况 + 边界情况

### 📝 语法实现优先级文档
- 建议实现顺序（依赖关系）
- 各阶段的验收标准

### 🔄 迁移与扩展指南
- 添加新语法的 checklist
- 从语法添加到单测的流程

## 验收标准

- ✅ 矩阵表完整列出已实现/规划/排除的语法特性
- ✅ 各阶段的语法功能通过相应的单测（覆盖率 ≥ 90%）
- ✅ ESTree 节点对应关系准确（可验证）
- ✅ 文档可用于指导新语法添加工作
- ✅ Round-trip 测试覆盖各阶段的语法（见 Issue 11）

## 优先级

🔴 **MVP**（必须在开发前明确，防止需求蔓延）

## 相关 Issue
- Issue 03: Parser 实现（按本矩阵分阶段）
- Issue 11: 集成测试（包含语法覆盖验证）
- Issue 09: 质量护栏（覆盖率、基准测试）

