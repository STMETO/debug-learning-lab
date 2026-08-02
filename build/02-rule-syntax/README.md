# 实验 02：规则语法 —— target、prerequisite、recipe

> 🎯 **目标**：准确读写一条规则，理解冒号、Tab、注释和续行。
>
> ⏱️ 约 20 分钟

## 一条规则的结构

```makefile
target: prerequisites
	recipe
```

当前规则：

```makefile
hello: hello.c
	gcc -Wall -Wextra hello.c -o hello
```

| 部分 | 当前值 | 含义 |
|------|--------|------|
| target | `hello` | 要生成或完成的目标 |
| prerequisites | `hello.c` | 构建前需要检查的文件 |
| recipe | `gcc ...` | 交给 shell 执行的命令 |

冒号左边是目标，右边是依赖。recipe 开头必须是 **Tab 字符**。换成空格会得到 `missing separator`。

## 注释与续行

`#` 开始注释。反斜杠把一条长命令分成多行：

```makefile
	gcc -Wall -Wextra hello.c \
	    -o hello
```

Shell 最终仍把它视为一条命令。

## 多条规则与默认目标

```makefile
hello: hello.c
	gcc hello.c -o hello

run: hello
	./hello
```

`make run` 选择 `run`；只执行 `make`，默认选择文件中第一条规则的目标。

## 关键认知

- Makefile 语法和 shell 语法是两层语言；
- 目标可以是文件，也可以代表动作；
- 依赖可以有零个、一个或多个；
- 一条规则可以有多行 recipe。

## 🧪 试试看

1. 把 Tab 替换为空格，观察错误后恢复。
2. 新增 `run: hello` 并执行 `make run`。
3. 把 `run` 放到第一条，观察默认目标变化。

## 下一步

[实验 03：增量构建](../03-incremental-build/) 解释为什么需要依赖。

