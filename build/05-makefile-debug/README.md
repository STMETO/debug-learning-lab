# 实验 05：Debug、Release 与 ASan —— 构建配置就是诊断能力

> 🎯 **目标**：为同一份源码构建 Debug、Release、ASan 三个版本，并理解它们的调试差异。
>
> ⏱️ 约 25 分钟
>
> 📖 **前置**：[实验 04](../04-makefile-libraries/)

## 先跑起来

~~~bash
make debug release asan
make run-debug
make run-release
make run-asan
~~~

源码里有一个故意的越界：循环条件错误地使用了 i <= count。普通版本的表现不可靠，可能打印一个看似正常的值；ASan 版本应该给出精确的 heap-buffer-overflow 报告。

## 三种构建，三种目的

| 目标 | 关键选项 | 用途 |
|------|----------|------|
| debug | -g -O0 | GDB 单步、查看变量、日常开发 |
| release | -O2 -DNDEBUG | 接近发布行为和真实性能 |
| asan | -g -O1 -fsanitize=address | 自动捕获内存错误 |

> -g 和 -O0 不是绑定关系。-g 负责调试信息，-O0 只是让代码尽量贴近源码。线上问题常需要 -g -O2 的 RelWithDebInfo 风格构建。

## 用 GDB 看 Debug 版本

~~~bash
gdb -q ./build/debug/buggy
~~~

~~~text
(gdb) break average
(gdb) run
(gdb) print count
(gdb) next
~~~

然后用 Release 版本重复。你可能会看到局部变量显示 <optimized out>，单步也可能跳过多行。这不是 GDB 坏了，而是优化器已经重排、内联或删除了源码中的中间状态。

## Sanitizer 必须参与链接

本实验把 -fsanitize=address 放在最终 gcc 命令中，而不是只用于 -c：

~~~makefile
$(CC) ... -fsanitize=address -o $@ $<
~~~

否则会出现 undefined reference to __asan_*。gcc 此时不仅是编译器驱动，也负责选择并链接 ASan 运行时。

## 🧪 试试看

1. 把 <= 改成 <，重新运行 make run-asan，确认报告消失。
2. 用 file 和 ls -lh 比较三个二进制文件。
3. 运行 make CFLAGS='...' 为什么不会影响本 Makefile？思考 WARNINGS 和每个目标的选项如何组织更清晰。

## 下一步

最后一个 Makefile 实验把这些能力组织成真正可维护的小项目：[实验 06：Makefile 工程化](../06-makefile-engineering/)。

