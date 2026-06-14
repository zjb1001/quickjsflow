# QuickJSFlow — 快速参考

## 项目地图

```
quickjsflow/
├── src/                          # 核心源码 (14 文件)
│   ├── lexer.c                   # 词法分析 — 8 种 Token 类型
│   ├── parser.c                  # 语法分析 — 47 AST 节点, 递归下降
│   ├── ast_print.c               # AST 构造/克隆/释放/JSON 打印
│   ├── scope.c                   # 作用域分析 — 两趟扫描, TDZ 检测
│   ├── edit.c                    # 不可变编辑 — 5 种原子操作
│   ├── codegen.c                 # 代码生成 — SourceMap + 注释保留
│   ├── cfg.c                     # 控制流图 — 9 种边类型, 3 种导出
│   ├── plugin.c                  # 插件系统 — Visitor 模式
│   ├── arena.c                   # Arena 分配器 — O(1) 分配/释放
│   ├── context.c                 # Context 抽象 — 线程安全, 统一生命周期
│   ├── api.c                     # 高层 C API — qjsf_parse/generate
│   ├── diff.c                    # Myers LCS diff 引擎
│   └── main.c                    # CLI 入口 — 6 个命令
├── include/quickjsflow/          # 公开头文件 (13 文件)
├── include/quickjsflow.h         # 统一公开头文件
├── test/                         # 测试 (14 二进制, 295 tests)
├── examples/                     # 示例 JS 文件
├── scripts/
│   └── pipeline-showcase.sh      # 全管道演示脚本
├── issues/                       # 15 个设计文档
├── .claude/skills/               # 8 个 Claude 技能
└── Makefile                      # C11 构建
```

## CLI 命令速查

| 命令 | 功能 | 示例 |
|------|------|------|
| `lex` | Token 流输出 | `build/quickjsflow lex file.js` |
| `parse` | JSON AST 输出 | `build/quickjsflow parse file.js` |
| `generate` | AST → JS 源码 | `build/quickjsflow generate file.js` |
| `check` | 全管道检查(含作用域) | `build/quickjsflow check file.js` |
| `cfg` | 控制流图 | `build/quickjsflow cfg file.js --format mermaid` |
| `run --plugin` | 运行插件转换 | `build/quickjsflow run file.js --plugin remove-console` |

## C 库 API

```c
#include "quickjsflow.h"

qjsf_context_t *ctx = qjsf_context_new();
AstNode *ast = qjsf_parse_string(ctx, "var x = 42;", 0);
CodegenResult out = qjsf_codegen(ctx, ast, NULL);
qjsf_context_free(ctx);  // 一键释放
```

## 管道架构

```
JS 源码 → Lexer → Parser → ScopeManager → Edit API → Codegen → 输出
              ↓                    ↓
           Token 流            作用域链
              ↓
         Plugin System ← AST 变换
              ↓
         CFG Builder ← 控制流分析
              ↓
         Diff Engine ← 增量编辑
```

## Issue 状态

| Issue | 模块 | 状态 |
|-------|------|------|
| 02 | Lexer | ✅ 完成 |
| 03 | Parser + AST | ✅ 完成 |
| 04 | ScopeManager | ✅ 完成 |
| 05 | Edit API | ✅ 完成 |
| 06 | Codegen | ✅ 完成 |
| 07 | CFG | ✅ 完成 |
| 08 | CLI/SDK/Plugin | ✅ 完成 |
| 09 | 质量/基准 | ✅ 完成 |
| 10 | Diff 引擎 | ✅ 完成 |
| 11 | 集成测试 | ✅ 完成 |
| 12 | 语法矩阵 | ✅ 完成 |
| 13 | 作用域细化 | ✅ 完成 |
| 14 | Library API | ✅ Alpha |

## 构建与测试

```bash
make clean && make    # 构建
make test             # 295 tests, 0 failures
make CC=clang         # Clang 构建
valgrind build/quickjsflow parse examples/sample.js  # 内存检查
```

## 技能参考

| 技能 | 用途 |
|------|------|
| `/qjsflow-teach` | 教学 — 解释管道各模块原理 |
| `/qjsflow-lab` | 动手实验 — 8 个渐进式练习 |
| `/qjsflow-debug` | 调试 — 四阶段根因分析 |
| `/qjsflow-dev` | 开发 — 七阶段功能开发流程 |
| `/qjsflow-review` | 评审 — 红蓝对抗代码审查 |
| `/qjsflow-style` | 风格 — C11 编码规范检查 |
| `/qjsflow-test` | 测试 — 测试策略与生成 |
| `/qjsflow-pr` | 补丁 — PR 审查与修复编排 |

---

**更新**：2026-06-14 | **版本**：v0.1.0 Alpha | **测试**：295 pass, 0 fail
