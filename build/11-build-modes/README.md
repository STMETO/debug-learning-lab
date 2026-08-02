# 实验 11：Debug、Release 与 ASan —— 配置是构建输入

> 🎯 **目标**：为同一源码生成三套互不覆盖的产物。

```bash
make MODE=debug
make MODE=release
make MODE=asan
make MODE=asan run
```

`MODE ?= debug` 提供默认值，命令行的 `MODE=release` 可以覆盖它。`ifeq` 根据变量选择选项；无法识别的模式用 `$(error ...)` 在构建开始前终止。

| 模式 | 关键选项 | 目的 |
|------|----------|------|
| debug | `-g -O0` | GDB 单步和变量观察 |
| release | `-O2 -DNDEBUG` | 发布行为和性能 |
| asan | `-g -O1 -fsanitize=address` | 检测内存错误 |

不同模式使用 `build/debug`、`build/release`、`build/asan`，避免复用选项不同的旧对象。

Sanitizer 选项必须同时参与编译和最终链接；本例单步编译链接在同一 GCC 命令中。多文件项目应确保链接命令也带 `-fsanitize=address`。

`-g` 和 `-O0` 不是同一个功能：前者生成调试信息，后者关闭优化。线上符号化常使用 `-g -O2`。

`buggy.c` 故意越界访问。`make MODE=asan run` 出现 AddressSanitizer 报告并以非零状态结束，正是本实验的预期结果，不代表构建失败。

## 🧪 试试看

1. 比较三种模式的 GCC 命令和产物路径。
2. 执行 `make MODE=unknown`，观察参数校验发生在何时。
3. 用 `file`、`size` 或 GDB 比较 Debug 与 Release 产物。

下一步：[静态库与动态库](../12-libraries/)。
