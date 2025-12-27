# README 更新建议

在主 README.md 中添加以下章节：

---

## 🧪 测试与质量保障

QuickJSFlow 采用全面的质量保障体系，包括自动化测试、代码覆盖率、性能基准测试和模糊测试。

### 快速开始

```bash
# 运行所有测试
make test

# 生成覆盖率报告
make coverage-report

# 运行性能基准测试
make run-benchmark

# 模糊测试（快速）
make fuzz-test-ci
```

### 文档

- 📘 [测试快速开始](TESTING_QUICKSTART.md) - 5分钟上手指南
- 📚 [完整测试文档](docs/QUALITY_TESTING.md) - 详细使用说明

### CI/CD 状态

[![CI](https://github.com/YOUR_USERNAME/quickjsflow/actions/workflows/ci.yml/badge.svg)](https://github.com/YOUR_USERNAME/quickjsflow/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/YOUR_USERNAME/quickjsflow/branch/main/graph/badge.svg)](https://codecov.io/gh/YOUR_USERNAME/quickjsflow)

*替换 `YOUR_USERNAME` 为实际的 GitHub 用户名*

### 质量指标

- ✅ 测试覆盖率目标: ≥ 80%
- ✅ 核心模块覆盖率: ≥ 90%
- ✅ CI 响应时间: < 5 分钟
- ✅ 连续模糊测试: 24+ 小时无崩溃

---

## 📊 性能基准

QuickJSFlow 包含完整的性能基准测试套件：

```bash
make run-benchmark
```

示例输出：
```
Benchmark                        Avg (ms)      MB/s
--------------------------------------------------
Lexer - Small                        0.001   7472.09
Parser - Medium                      0.030    921.16
Full Pipeline - Large                0.193    103.54
```

---

## 🛠️ 开发

### 构建系统

```bash
make help  # 查看所有可用命令
```

主要命令：
- `make all` - 构建主程序
- `make test` - 运行测试
- `make coverage-report` - 生成覆盖率报告
- `make run-benchmark` - 性能基准测试
- `make clean` - 清理构建产物

### 依赖安装

**Ubuntu/Debian:**
```bash
sudo apt-get install build-essential lcov valgrind afl++
```

**macOS:**
```bash
brew install lcov afl++
```

---

记得在文档中更新 GitHub 用户名和仓库名称！
