# 实验 01：第一份 Makefile —— 先让 make 工作起来

> 🎯 **目标**：第一次使用 Makefile 编译程序，知道 `make` 会读取什么、执行什么。
>
> ⏱️ 约 15 分钟

## 你现在要解决什么问题？

手工编译是：

```bash
gcc hello.c -o hello
```

Makefile 的第一个作用，是把构建命令记录在项目中，让所有人使用相同入口。

## 先跑起来

```bash
make
./hello
```

完整 Makefile 只有两行：

```makefile
hello: hello.c
	gcc hello.c -o hello
```

先读成一句话：

> 为了得到 `hello`，需要 `hello.c`；当需要构建时，执行 GCC 命令。

## make 做了什么？

1. 在当前目录寻找 `GNUmakefile`、`makefile` 或 `Makefile`；
2. 读取第一条规则；
3. 判断是否需要执行规则；
4. 需要时让 shell 执行 GCC。

Make 不是编译器，真正编译代码的仍是 GCC。

再次执行 `make`，会提示目标已经是最新的。实验 03 会解释这个判断。

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `No targets specified` | 当前目录没有 Makefile | 用 `pwd`、`ls` 确认目录 |
| `No rule to make target` | 文件名或规则写错 | 对照实际文件名 |
| 运行的程序没有变化 | 可能运行了其他目录的旧文件 | 用 `pwd`、`file hello` 确认 |

## 🧪 试试看

1. 修改程序输出，再执行 `make && ./hello`。
2. 把目标名改成 `app`，Makefile 中哪些地方要一起改变？
3. 将 Makefile 重命名，直接执行 `make` 会发生什么？

## 下一步

[实验 02：规则语法](../02-rule-syntax/) 会逐字解释这两行。

