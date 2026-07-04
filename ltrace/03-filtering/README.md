# 实验 03：过滤库调用 —— 54 行缩减到 5 行

> 🎯 **目标**：用 `-e` 按函数名或库名过滤，在成百上千的调用中只看你关心的。
>
> ⏱️ 约 10 分钟
>
> 📖 **前置**：[实验 02](../02-read-output/)（库来源标记 `程序名->` vs `libc.so.6->`）

## 和 strace 的过滤有什么不同？

strace 用 `-e trace=类别`（file/network/process），按系统调用的**功能类别**分。

ltrace 用 `-e 函数1+函数2`（strlen+malloc+free），按**具体函数名**分。因为库函数太多了，没有一个好的分类体系——你得按名字来。

---

## 先看示例程序

[mixed.c](mixed.c) 有四类操作，每类使用不同的库函数：

```
字符串: strlen, strdup, strchr
内存:   memset, memcpy, memcmp, malloc, free
数学:   sqrt, pow
字符:   isalpha, isdigit
```

```bash
make && ./mixed
```

---

## 实验开始

### 第一步：看全量——54 行太多了

```bash
ltrace -o /tmp/full.log ./mixed
wc -l /tmp/full.log
```

输出：`54 /tmp/full.log`

其中大部分是 `libc.so.6->` 的内部调用（`printf` 格式化引擎），只有少数是 `mixed->`。

### 第二步：按函数名过滤——只看字符串函数

```bash
ltrace -e strlen+strdup+strchr ./mixed | grep 'mixed->'
```

输出（只有 5 行）：

```
mixed->strlen("Hello, ltrace filtering demo!") = 29
mixed->strdup("Hello, ltrace filtering demo!") = 0x...
mixed->strchr("Hello, ltrace filtering demo!", 'l') = "llo, ltrace filtering demo!"
mixed->strlen("memory test data")              = 16
mixed->strlen("memory test data")              = 16
```

> 54 行 → 5 行。你现在有一个清晰的"字符串函数调用史"：先量长度 → 复制 → 找字符 → 又量了两次长度。

### 第三步：按函数名过滤——只看内存函数

```bash
ltrace -e memset+memcpy+memcmp+malloc+free ./mixed | grep 'mixed->'
```

输出：

```
mixed->memset(..., '\0', 64)   = ...    ← buf1 清零
mixed->memset(..., '\0', 64)   = ...    ← buf2 清零
mixed->memcpy(..., "memory test data", 17) = ... ← 拷贝
mixed->memcmp(..., 16)         = 0       ← 比较（相等）
mixed->malloc(64)              = ...     ← 循环 5 次
mixed->free(...)               = <void>
mixed->malloc(64)              = ...
mixed->free(...)               = <void>
...（重复 5 组 malloc/free）
```

> 立刻能看出：5 次 `malloc` + 5 次 `free` 配对了（没有泄漏）、`memset` 调了 2 次、`memcmp` 返回 0（内存相等）。

### 第四步：组合过滤 + 过滤库来源

ltrace 支持三种过滤层级：

| 层级 | 语法 | 示例 | 效果 |
|------|------|------|------|
| 按函数名 | `-e 函数1+函数2` | `-e strlen+malloc` | 只看指定函数 |
| 按库文件 | `-e @库路径` | `-e @libc.so.6` | 只看来自某个库的调用 |
| 按可执行文件 | `-e @程序名` | 等价于 `grep 'mixed->'` | 只看你的程序的调用 |

**按库过滤**（只看 libc 调用）：

```bash
ltrace -e @libc.so.6 ./mixed| head -10
```

**排除某个库**：

```bash
ltrace -e '!@libc.so.6' ./mixed
```

### 第五步：实战——四种过滤看到四个不同的"故事"

对同一个 `mixed` 程序：

| 过滤条件 | 看到的"故事" |
|---------|------------|
| `-e strlen+strdup+strchr` | 字符串被测量、复制、搜索——参数和返回值都看得很清楚 |
| `-e malloc+free` | 5 次分配 5 次释放——配平了，没有内存泄漏 |
| `-e sqrt+pow` | sqrt(144)=12, pow(2,8)=256——数学计算结果正确 |
| `-e isalpha+isdigit` | 循环中对每个字符调用 isalpha/isdigit——也许应该用查找表优化 |

> 换个过滤条件，就像换个视角 —— 同一个程序，四种完全不同的故事。

---

## 📝 命令小结

| 语法 | 作用 |
|------|------|
| `-e 函数1+函数2` | 只追踪指定的函数 |
| `-e @库路径` | 只追踪来自指定库的调用 |
| `-e '!@库路径'` | 排除指定库 |
| `grep '程序名->'` | 只看你的程序产生的调用（过滤 libc 内部噪音） |

---

## 🧪 试试看

1. 用 `-e printf+puts+putchar` 追踪 `./mixed`——能看到哪些输出函数被调用了？
2. 对比 `ltrace -e malloc+free ./mixed` 和 `strace -e trace=memory ./mixed`——两者看到的"内存分配"有什么不同？
3. 用 `-e @libm.so.6` 只看数学库的调用——`sqrt` 和 `pow` 出现了吗？
4. 写一个自己的程序，用 `-e` 追踪其中一个函数——确认它被调用的次数和你预期的一样

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `-e strlen,malloc` 不生效 | ltrace 用 `+` 连接，不是 `,` | `-e strlen+malloc` |
| `-e @libc.so` 不匹配 | 完整名称可能是 `libc.so.6` | `ldd ./program` 查看精确的库名 |
| 过滤后输出仍然是 0 行 | 函数被内联或静态链接了 | `-O0 -fno-builtin` 编译 |

---

## 下一步

过滤让你"只看你想看的"。下一步 [实验 04：时间与统计](../04-timing-stats/)——用 `-t`/`-T` 看每次调用的耗时，用 `-c` 生成调用频率报告。
