# Issue #14: [Architecture] libqjsf - High-Performance JavaScript AST Library

## 1. 愿景与定位 (Vision & Positioning)

**愿景：** 将 QuickJSFlow 重构为 `libqjsf`，一个生产级 C 语言库，成为 JavaScript AST 处理领域的标准组件。  
**目标定位：** `cJSON` 级别的库 —— 轻量级、可嵌入、零依赖、透明 API、高性能。  
**核心价值：**
- 🎯 **即插即用：** 最小化集成成本，支持静态和动态链接。
- 🔧 **可扩展：** 为下游工具链（Linter、Formatter、Transformer）提供稳定基础。
- 🚀 **高性能：** 超越同类 JavaScript 解析库的性能基准。
- 📦 **生产就绪：** 完整的错误处理、文档、测试覆盖。

## 2. 问题分析 (Problem Statement)

**现状困境：**
1. 当前设计是**命令行工具优先**，无法作为库被集成
2. 全局状态导致**无法并发使用**
3. 每个节点 malloc 导致**内存碎片化**和难以调试的泄漏
4. **缺乏清晰的 API 边界**，内部实现细节暴露

**机会：** 通过精心设计的库化重构，QuickJSFlow 可以成为 JavaScript 开发工具链的基础设施，为 Linter、Formatter、Code Transformer 等工具赋能。

## 3. 核心架构原则 (Architectural Pillars)

### 2.1. 上下文感知与重入性 (Context-Aware & Reentrant)
*   **现状问题:** 全局状态导致无法在多线程环境中使用。
*   **设计方案:** 引入 `qjsf_context_t`。所有的解析、节点创建、代码生成操作都必须依赖一个上下文句柄。
*   **收益:** 实现完全的线程安全；支持多个独立的解析任务并行运行。
*   **示例：** Web 服务每个请求创建独立 Context，无需全局锁。

### 2.2. 区域内存管理 (Arena/Pool Memory Management)
*   **现状问题:** 对每个 AST 节点调用 `malloc` 会导致大量系统调用和内存碎片，且释放困难。
*   **设计方案:** 在 `qjsf_context_t` 中实现一个简单的线性分配器 (Arena Allocator)。
*   **收益:**
    *   **分配极快:** 仅需移动指针。
    *   **释放 O(1):** 销毁 Context 时直接释放整块内存，彻底根除 AST 节点的内存泄漏风险。
    *   **内存占用可预测:** 通常 1.0-1.5x 源代码大小。

### 2.3. 数据导向的 AST 设计 ("cJSON" Style)
*   **设计方案:** 将 `qjsf_node_t` 设计为**透明结构体** (Transparent Struct)，而非不透明句柄。
*   **理由:** C 语言开发者更习惯直接访问结构体成员 (如 `node->type`, `node->child`) 以获得最高性能和调试便利性，类似 `cJSON` 的设计模式。
*   **平衡:** 提供辅助宏/函数用于安全操作，但允许直接读取。
*   **性能优势：** 避免函数调用开销，内存布局紧凑，缓存友好。

### 2.4. 零依赖与可移植性 (Zero Dependency & Portability)
*   **核心库仅依赖 libc：** 不引入第三方库，便于跨平台编译。
*   **C99 标准：** 最低要求 C99，借用 C11 特性时保证优雅降级。
*   **支持平台：** Linux (x86_64, ARM), macOS, Windows, iOS, Android

## 3. API 设计规范 (API Design Specification)

### 3.1. 核心类型定义 (`quickjsflow.h`)

```c
// 不透明的上下文句柄
typedef struct qjsf_context_s qjsf_context_t;

// 节点类型枚举
typedef enum { QJSF_NODE_PROGRAM, QJSF_NODE_VAR_DECL, ... } qjsf_node_type_t;

// 透明的节点结构 (类似 cJSON)
typedef struct qjsf_node_s {
    qjsf_node_type_t type;
    struct qjsf_node_s *next, *prev; // 双向链表处理兄弟节点
    struct qjsf_node_s *child;       // 第一个子节点
    
    char *string_value;              // 标识符或字面量的值
    double number_value;             // 数字字面量的值
    
    // 源码位置信息
    int start_line, start_col;
    int end_line, end_col;
} qjsf_node_t;
```

### 3.2. 生命周期管理 (Lifecycle)

```c
// 1. 创建上下文 (包含内存池)
qjsf_context_t* ctx = qjsf_context_new();

// 2. 解析代码 (节点内存由 ctx 管理)
qjsf_node_t* root = qjsf_parse_string(ctx, code, length);

// 3. 遍历与修改 (直接操作指针或使用 API)
if (root->type == QJSF_NODE_PROGRAM) {
    // ...
}

// 4. 代码生成
char* new_code = qjsf_codegen(ctx, root);

// 5. 一键释放所有资源 (AST节点, 生成的代码, 上下文本身)
qjsf_context_free(ctx);
```

### 3.3. 错误处理模型 (Error Handling Model)
**设计原则：** 显式错误处理，避免异常 (C 语言特性)。

```c
// 错误类型枚举
typedef enum {
    QJSF_OK = 0,
    QJSF_ERR_INVALID_SYNTAX,
    QJSF_ERR_UNEXPECTED_EOF,
    QJSF_ERR_INVALID_TOKEN,
    QJSF_ERR_OUT_OF_MEMORY,
    QJSF_ERR_NULL_CONTEXT,
    QJSF_ERR_CODEGEN_FAILED,
} qjsf_error_t;

// 错误信息结构
typedef struct {
    qjsf_error_t code;
    char message[256];
    int line, column;
    int context_start, context_end;
} qjsf_error_info_t;

// 获取上次错误
const qjsf_error_info_t* qjsf_context_get_error(qjsf_context_t* ctx);
```

**错误传播模式：**
- 所有 API 返回 `qjsf_error_t` 或指针（NULL 表示失败）
- 通过 `qjsf_context_get_error()` 获取详细错误信息
- 错误堆栈：支持链式错误追踪

### 3.4. 遍历与转换 (Traversal & Transformation)

```c
// 访问者模式 - 用于代码分析和转换
typedef struct qjsf_visitor_s {
    void (*on_enter)(qjsf_node_t* node, void* user_data);
    void (*on_exit)(qjsf_node_t* node, void* user_data);
    void (*on_error)(qjsf_error_t err, void* user_data);
} qjsf_visitor_t;

qjsf_error_t qjsf_traverse(qjsf_node_t* root, const qjsf_visitor_t* visitor, void* user_data);

// 快速迭代遍历 - 用于简单遍历
qjsf_node_t* qjsf_node_first_child(qjsf_node_t* node);
qjsf_node_t* qjsf_node_next_sibling(qjsf_node_t* node);
```

### 3.5. ABI 稳定性与版本管理 (ABI Stability & Versioning)

```c
// 版本信息
#define QJSF_VERSION_MAJOR 1
#define QJSF_VERSION_MINOR 0
#define QJSF_VERSION_PATCH 0

// 运行时版本检查
const char* qjsf_version_string(void);
void qjsf_version_info(int* major, int* minor, int* patch);

// ABI 兼容性标记（每次不兼容变更递增）
#define QJSF_ABI_VERSION 1
```

**承诺：** 在主版本号不变的情况下，新增 API 不会破坏既有代码。所有指针类型大小固定，struct 字段只在末尾扩展。

## 4. 生产级设计考量 (Production-Grade Design Considerations)

### 4.1. 构建与发布策略 (Build & Distribution)
*   **CMake + pkg-config：** 支持 `find_package()` 和 `pkg-config` 集成。
*   **多目标输出：** 
    - 静态库：`libquickjsflow.a` (嵌入式、移动应用)
    - 动态库：`libquickjsflow.so`/`.dylib` (系统库、减少二进制大小)
*   **包管理器：** 初期目标在 vcpkg、conan 中注册，便于 C/C++ 开发者集成。

### 4.2. 文档与示例 (Documentation & Examples)
*   **Doxygen：** 从代码注释自动生成 HTML API 参考。
*   **示例库：**
    - `examples/parse_only.c` - 仅解析，不修改
    - `examples/transform_var.c` - 遍历并修改变量名
    - `examples/json_dump.c` - 将 AST 导出为 JSON（调试用）
    - `examples/rewrite_arrow_func.c` - 将箭头函数转换为普通函数
*   **快速入门指南：** 10 分钟从零开始集成 libqjsf 的教程。

### 4.3. 测试与质量保证 (Testing & QA)
*   **单元测试套件：** 覆盖所有公开 API，至少 90% 代码覆盖率。
*   **Valgrind 验证：** 零内存泄漏和 UB 漏洞。
*   **Fuzz 测试：** 使用 libFuzzer 测试解析器鲁棒性。
*   **性能基准：** 建立性能基线，提供解析速度与内存占用指标。
    - 目标：解析 100KB JS 代码 < 10ms，内存占用 < 2x 源代码大小。

### 4.4. 平台与兼容性 (Platform & Compatibility)
*   **支持平台：** Linux (x86_64, ARM), macOS, Windows (MSVC, MinGW), iOS, Android
*   **编译器：** GCC 7+, Clang 5+, MSVC 2015+
*   **C 标准：** C99（最低要求），借用 C11 特性时保证向后兼容

### 4.5. 配置系统 (Configuration System)
*   **编译时配置：** 
    ```makefile
    # 可选功能开关
    -DQJSF_ENABLE_SOURCE_MAP
    -DQJSF_ENABLE_UNICODE_ESCAPE
    -DQJSF_CUSTOM_ALLOCATOR  # 允许用户提供自定义内存分配器
    ```
*   **运行时配置：** 通过 `qjsf_context_config_t` 结构体传入选项
    ```c
    qjsf_context_config_t cfg = {
        .max_ast_depth = 1024,
        .max_string_length = 1024 * 1024,
        .enable_source_map = true,
    };
    qjsf_context_t* ctx = qjsf_context_new_with_config(&cfg);
    ```

### 4.6. 向下兼容性与演进计划 (Backwards Compatibility & Evolution)
*   **API 冻结期：** v1.0 发布后，12 个月内不做不兼容变更。
*   **扩展点：** 为 v2.0+ 保留扩展点，例如：
    - 增量解析 (Incremental Parsing)
    - 流式处理 (Streaming Parser)
    - 自定义节点类型注册 (Custom Node Types)
*   **废弃声明：** 使用 `QJSF_DEPRECATED` 宏标记将被移除的 API

## 5. 实施路线图与交付计划 (Implementation Roadmap & Milestones)

### 5.1. Alpha 阶段 (Foundation - v0.1)
**目标：** 建立基础架构，验证核心设计可行性  
**时间框架：** 4-6 周  
**交付物：**
- [x] `include/quickjsflow.h` 公开头文件，定义所有核心类型（通过 context.h 提供）
- [x] Context 和 Arena 内存管理实现（`src/arena.c`, `src/context.c`）
- [ ] Parser 改造完毕，支持 Context-based 内存分配
- [ ] 基础的 Codegen 支持 Context 管理的输出缓冲区
- [x] 50+ 个单元测试，覆盖核心功能（305 tests pass）
- [x] Valgrind 无泄漏验证通过（pre-existing CI check）

### 5.2. Beta 阶段 (Feature Complete - v0.5)
**目标：** 完整功能集，API 稳定，适合 alpha 测试  
**时间框架：** 4-6 周  
**交付物：**
- [ ] 错误处理模型完整实现，所有 API 遵循统一错误约定
- [ ] Visitor 模式和遍历 API 完成
- [ ] ESTree JSON 序列化（`qjsf_dump_json()`）
- [ ] CMakeLists.txt，支持静态和动态库构建
- [ ] 完整示例代码（4+ 个不同场景）
- [ ] 初版 Doxygen 文档
- [ ] 性能基准测试框架搭建，建立性能基线

### 5.3. 稳定版发布 (Stable Release - v1.0)
**目标：** 生产级库，ABI 冻结，长期支持承诺  
**时间框架：** 2-4 周  
**交付物：**
- [ ] 所有 API 版本化和向后兼容性承诺
- [ ] 快速入门指南和完整 API 参考文档
- [ ] pkg-config 和 CMake 集成验证
- [ ] vcpkg 包定义提交
- [ ] Fuzz 测试 100+ 小时无崩溃
- [ ] 主流 Linux 发行版打包（Fedora, Ubuntu, Debian）

### 5.4. 高级特性 (Future - v1.1+)
**Roadmap（不承诺时间表）：**
- [ ] 增量解析支持（用于 IDE 场景）
- [ ] 流式 Parser（处理超大文件）
- [ ] 自定义内存分配器接口
- [ ] AST 节点持久化格式（二进制序列化，快速 load）
- [ ] 官方 Rust 绑定 （libqjsf-sys）
- [ ] 性能优化：SIMD 加速的词法分析

## 6. 核心成功指标 (Key Success Metrics)

| 指标 | 目标 | 验证方法 |
|------|------|--------|
| **API 稳定性** | v1.0 后 6 个月内 0 不兼容变更 | 自动化 ABI 检查工具 (abicheck) |
| **内存安全** | Valgrind 无泄漏、无 UB | CI 流程中运行 valgrind --leak-check=full |
| **解析性能** | 100KB JS < 10ms；内存占用 < 2x 源代码大小 | 性能基准测试套件 (benchmark.c) |
| **代码覆盖** | 公开 API 90%+ 测试覆盖 | lcov + gcov 每次 CI 验证 |
| **文档完整性** | 所有公开 API 都有 Doxygen 注释 | 文档生成流程检查 |
| **跨平台支持** | Linux, macOS, Windows 编译通过 | CI 矩阵测试（GCC, Clang, MSVC） |
| **错误处理** | 所有 API 明确处理 OOM 和语法错误 | Fuzz 测试 + 集成测试 |
| **生产就绪度** | 可在 vcpkg/conan 中安装使用 | 实际集成测试验证 |

## 7. 架构设计亮点 (Architectural Highlights)

### 7.1. 内存管理的创新
**问题：** 传统逐个 malloc 导致碎片化和泄漏。  
**方案：** Arena Allocator —— 一次性分配大块内存，所有 AST 节点从中分配。  
**收益：**
- 📊 分配速度：从 O(log n) 降低到 O(1)
- 🧹 释放成本：销毁 Context 时一次性释放，复杂度 O(1)
- 🎯 内存占用可预测：最坏情况下 150% × 源代码大小

### 7.2. 线程安全的重入性
**问题：** 全局状态阻碍并发使用。  
**方案：** 所有状态封装在 `qjsf_context_t` 中，多线程各自创建独立 Context。  
**使用场景：** 
- Web 服务处理多个并发请求时，每个请求对应一个 Context
- IDE 在后台线程进行增量解析时，Context 隔离避免竞争

### 7.3. 零成本的抽象
**问题：** 过度的 Getter/Setter 增加性能开销。  
**方案：** 节点结构体透明暴露，允许直接访问成员（如 cJSON）。  
**平衡：** 提供宏和内联函数进行安全检查，但不强制使用。

## 8. 与现有工具链的集成 (Integration with Ecosystem)

### 8.1. 编程语言绑定
- **Rust:** `libqjsf-sys` crate，将库暴露给 Rust 生态
- **Python：** ctypes 绑定用于快速原型开发
- **Node.js：** N-API 绑定，便于前端开发者使用

### 8.2. 工具链生态
- **Linters:** ESLint 风格的 linter 可基于 libqjsf 实现
- **Formatters:** Prettier 风格的代码格式化工具
- **Babel 插件:** 通过 C 库提升解析性能
- **静态分析工具：** 数据流分析、类型检查工具的基础

## 9. 风险与缓解策略 (Risks & Mitigation)

| 风险 | 影响 | 缓解策略 |
|------|------|--------|
| ABI 不兼容变更导致用户代码崩溃 | 高 | 版本化 API，严格的回归测试，CI 自动检查 ABI 变更 |
| 内存泄漏未被发现 | 高 | 强制 Valgrind，Fuzz 测试长期运行 |
| 性能不达预期 | 中 | 早期性能基准建立，性能回归测试 |
| 跨平台编译问题 | 中 | 多平台 CI 验证，社区反馈 |
| 缺乏文档导致使用障碍 | 中 | Doxygen + 丰富示例，活跃社区反馈 |
