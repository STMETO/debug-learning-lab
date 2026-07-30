# 实验 03：自动头文件依赖 —— 不再漏编译

> 🎯 **目标**：用模式规则和 .d 文件，让 Makefile 自动追踪所有头文件依赖。
>
> ⏱️ 约 25 分钟
>
> 📖 **前置**：[实验 02](../02-makefile-basics/)

## 问题：只依赖 .c 不够

这个实验有两个源文件和两个头文件：

~~~text
src/main.c ──→ include/config.h, include/log.h
src/log.c  ───→ include/config.h, include/log.h
~~~

~~~bash
make
./build/dependency_demo
~~~

如果 Makefile 只写 main.o: main.c，那么你修改 config.h 后，make 不会知道 main.o 和 log.o 已经过期。程序可能继续使用旧定义，这种问题很隐蔽。

## 第一步：让编译器生成依赖

Makefile 的关键选项是：

~~~makefile
CFLAGS := -g -Wall -Wextra -MMD -MP
~~~

| 选项 | 作用 |
|------|------|
| -MMD | 编译每个 .c 时，同时写一个同名 .d 文件；忽略系统头文件 |
| -MP | 给每个头文件生成空规则，头文件删除或重命名时错误更友好 |

构建后查看：

~~~bash
cat build/main.d
cat build/log.d
~~~

你会看到类似：

~~~makefile
build/main.o: src/main.c include/config.h include/log.h
~~~

最后一行把这些规则读回 make：

~~~makefile
-include $(DEPS)
~~~

开头的 - 非常重要：首次构建时 .d 文件尚不存在，make 也不应报错。

## 第二步：模式规则和目录前置条件

~~~makefile
$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
~~~

% 是通配部分。它让一条规则同时适用于 main.c → main.o、log.c → log.o。

竖线后的 | $(BUILD) 是 order-only prerequisite：build 目录必须存在，但即使目录时间戳变化，也不应该让所有 .o 无意义地重编译。

## 第三步：验证依赖真的生效

~~~bash
make clean && make
sleep 1
touch include/config.h
make
~~~

这次 main.o 和 log.o 都会重新编译，再重新链接。只 touch include/log.h 也会触发两个对象，因为两个源文件都包含它。

再试试并行构建：

~~~bash
make clean
make -j4
~~~

目录前置条件会确保多个并行任务争抢 build 目录时仍然正确。

## 📝 核心认知

~~~text
手写头文件依赖：容易漏，维护成本随项目增长
-MMD + -include：让编译器告诉 make “我实际 include 了什么”
模式规则：把重复的编译命令压缩为一条通用规则
~~~

## 🧪 试试看

1. 给 log.c 新增一个头文件，并观察 .d 文件是否自动更新。
2. 删除 -include $(DEPS)，touch config.h 后还会重编译吗？
3. 执行 make -j8，观察命令是否可以并行。

## 下一步

接下来进入 [实验 04：静态库与动态库](../04-makefile-libraries/)。构建系统不只处理 .o，还要管理库和运行时加载。

