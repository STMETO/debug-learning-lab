# 实验 02：Makefile 基础 —— 让 make 替你判断要做什么

> 🎯 **目标**：掌握 target、依赖、recipe、变量和自动变量；看懂 make 的增量构建判断。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 01](../01-from-source/)

## 先跑起来

~~~bash
make
make run
make print-vars
~~~

第一次 make 会显示 gcc 命令；立刻再执行一次：

~~~bash
make
~~~

你应该看到：

~~~text
make: Nothing to be done for 'all'.
~~~

这不是 make 偷懒。它比较了目标文件和依赖文件的修改时间，确认 hello_make 比 main.c 新，所以无需再次构建。

## 读懂一条规则

[Makefile](Makefile) 的核心是：

~~~makefile
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $@ $<
~~~

把它读成一句话：

> 要得到 TARGET，先确保 SOURCE 已经准备好；然后执行下一行命令。

| 写法 | 本实验中的值 | 含义 |
|------|--------------|------|
| $(TARGET) | hello_make | 变量引用 |
| $(SOURCE) | main.c | 变量引用 |
| $@ | hello_make | 当前规则的目标 |
| $< | main.c | 第一个依赖 |
| $^ | 所有依赖 | 多文件链接时常用 |

> recipe 前面必须是一个 Tab，不能用空格。make 报 missing separator 时，先检查这里。

## 观察增量构建

~~~bash
touch main.c
make
make
make clean
make
~~~

第一条 touch 让 main.c 变“新”，所以 make 重建目标；clean 删掉目标后，make 也必须重新构建。

## 为什么 clean 和 run 要写 .PHONY？

clean、run、print-vars 是“动作”，不是要生成名为 clean 的文件：

~~~makefile
.PHONY: all run clean print-vars
~~~

如果目录里恰好存在一个名为 clean 的文件，而你没有声明 .PHONY，make 会误以为 clean 已经是最新的，从而不执行删除命令。

## 📝 命令小结

| 命令 | 作用 |
|------|------|
| make | 构建第一个目标（这里是 all） |
| make run | 先构建，再运行 |
| make -n | 只打印将要执行的命令，不真正执行 |
| make -B | 忽略时间戳，强制重建 |
| make -j4 | 最多同时执行 4 个彼此独立的任务 |

## 🧪 试试看

1. 把 TARGET 改成 app，make -n 显示的命令如何变化？
2. 创建一个空文件 touch clean，再执行 make clean；去掉 .PHONY 后再试一次。
3. 用 make CFLAGS='-g -O2 -Wall -Wextra' 覆盖默认编译选项。

## 下一步

单文件项目还不够真实。下一实验学习 [自动头文件依赖](../03-makefile-dependencies/)，避免“改了 .h 却没有重新编译”的经典问题。

