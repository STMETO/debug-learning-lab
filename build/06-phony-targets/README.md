# 实验 06：`.PHONY` 与操作目标

> 🎯 **目标**：区分文件目标和动作目标，组织 `all/run/clean/help`。

`app` 是文件目标；`run`、`clean`、`help` 是动作：

```makefile
.PHONY: all run clean help
```

没有 `.PHONY` 时，如果存在名为 `clean` 的文件，make 可能跳过清理命令。

```makefile
.DEFAULT_GOAL := all
all: app
```

这显式指定默认目标。

```makefile
run: app
	./app
```

`make run` 会先确保 `app` 最新，不需要在 recipe 中再次调用 `make app`。

Recipe 默认先显示再执行；`@命令` 隐藏命令本身。命令返回非零时 make 默认停止，`-命令` 可忽略失败，但应谨慎使用。

## 🧪 试试看

1. `touch clean`，比较有无 `.PHONY` 的行为。
2. 给 `help` 增加变量覆盖示例。
3. 添加一个执行程序并检查返回码的 `check` 目标。

## 下一步

基础语法完成。下一实验进入 [多文件项目](../07-multifile/)。

