# 实验 06：Makefile 工程化 —— 一个入口管理构建、测试和诊断

> 🎯 **目标**：组织一个带 src、include、tests、多个构建模式的非递归 Makefile 项目。
>
> ⏱️ 约 30 分钟
>
> 📖 **前置**：[实验 05](../05-makefile-debug/)

## 先跑起来

~~~bash
make
make run
make test
make MODE=release
make MODE=asan test
~~~

项目结构：

~~~text
include/parser.h       对外声明
src/parser.c           可复用逻辑
src/main.c             应用入口
tests/test_parser.c    独立测试程序
build/<mode>/...       所有生成物
~~~

## 为什么不把 .o 放在 src/？

把产物放进 build/debug、build/release 和 build/asan 有三个好处：

- 源码目录始终干净，git status 不会混入二进制；
- 不同配置能共存，不必在 Debug 和 Release 间反复 clean；
- 删除产物只需要 rm -rf build，不会误删源码。

## 配置是一个显式输入

~~~bash
make MODE=debug
make MODE=release
make MODE=asan test
~~~

Makefile 通过 MODE ?= debug 提供默认值，并用条件块选择选项。?= 的含义是“调用者没有提供时才赋值”，因此命令行能覆盖它。

本实验没有使用递归 Make（每个目录一个 Makefile，再从顶层逐层调用）。所有依赖都在一个 Makefile 图中，make 才能正确地看到目标之间的关系并安全并行。

## 用 make 帮助调试构建

~~~bash
make help
make -n MODE=asan test
make -j8 MODE=debug all
make -B MODE=debug
~~~

-n 是诊断 Makefile 的首选工具：先看命令是否符合预期，再真正执行。-B 则适合验证“是不是旧产物造成的问题”。

## 🧪 试试看

1. 给 parser.c 加一条 printf，分别运行 debug 和 release 构建，观察两套产物是否独立。
2. 修改 parser.h，make 会重建哪些 .o？为什么？
3. 尝试 make MODE=invalid，阅读 Makefile 的 $(error ...) 输出。

## 下一步

Makefile 阶段结束。接下来进入 [实验 07：CMake 基础](../07-cmake-basics/)，学习把构建图交给 CMake 生成。

