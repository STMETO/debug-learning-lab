# 实验 02：读懂 ltrace 输出 —— 返回值、来源库、嵌套调用

> 🎯 **目标**：深入理解 ltrace 输出的全部格式——不同类型的返回值、库来源标记、`<unfinished ...>` 含义。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[实验 01](../01-hello-ltrace/)（你至少跑过一次 `ltrace ./hello`）

---

## 先看示例程序

[decode.c](decode.c) 精心设计来展示五种不同"口味"的 ltrace 输出：

| 类型 | 示例函数 | 返回值特征 |
|------|---------|-----------|
| 返回整数 | `strlen`、`strcmp`、`memcmp` | `= 10`、`= -1`、`= 0` |
| 返回指针 | `strdup`、`strchr`、`memcpy` | `= 0x...` 或 `= "字符串"` |
| 返回 NULL | `fopen(不存在的路径)` | `= 0`（显示为 `nil`） |
| 返回 void | `free` | `= <void>` |
| 数学函数 | `sqrt`、`pow` | `= 8.0` |

```bash
make && ./decode
```

---

## 实验开始

### 第一步：带过滤运行，只看函数调用

```bash
ltrace -e strlen+strcmp+strdup+strchr+fopen+memset+memcpy+memcmp+free ./decode
```

输出（去掉 libc 内部噪音后的核心部分）：

```
decode->strdup("hello ltrace" <unfinished ...>     ← ① 嵌套调用标记
...
<... strdup resumed> )                           = 0x609f...  ← ②
decode->strchr("hello ltrace", 'l')              = "llo ltrace"  ← ③
decode->free(0x609f...)                          = <void>       ← ④
decode->fopen("/tmp/不存在的目录/文件.txt", "r")  = 0            ← ⑤
decode->memset(0x7fff..., '\0', 32)              = 0x7fff...   ← ⑥
decode->memcpy(0x7fff..., "overwritten", 11)     = 0x7fff...
decode->memcmp(0x7fff..., 0x..., 10)             = 0            ← ⑦
```

---

### 第二步：解码返回值类型

#### 类型 A：返回整数 —— `= N`

```
decode->strlen("measure me") = 10        ← 字符串长度
libc.so.6->strcmp("abc", "abd") = -1    ← a<b 按字典序（实际是 -1）
decode->memcmp(dest, src, 10) = 0       ← 0 = 内容相等
```

> 整数返回值直接显示数字。`strcmp` 返回 -1/0/1（负数=小于，0=等于，正数=大于）。

#### 类型 B：返回指针（带字符串解引用）—— `= "string"`

```
decode->strchr("hello ltrace", 'l') = "llo ltrace"
```

ltrace 的**特殊能力**：当函数返回 `char *` 时，ltrace 自动解引用指针并显示字符串内容。`strchr(p, 'l')` 返回指针 → ltrace 显示 `"llo ltrace"`。

对比原始指针显示：

```
decode->memcpy(0x7fff..., "overwritten", 11) = 0x7fff...
```

`memcpy` 返回 `void *`——ltrace 不知道它指向字符串，所以只显示地址。

#### 类型 C：返回 NULL / nil —— `= 0`

```
decode->fopen("/tmp/不存在的目录/文件.txt", "r") = 0
```

`fopen` 失败返回 `NULL`，ltrace 显示为 `0`（有时显示 `nil`）。

> 💡 **调试技巧**：在 ltrace 输出中搜索 `= 0` 或 `= nil`——找到的可能是失败的调用。

#### 类型 D：返回 void —— `= <void>`

```
decode->free(0x609f...) = <void>
```

`free` 没有返回值（返回类型是 `void`），ltrace 显示 `<void>`。

#### 类型 E：不返回 —— `= <unfinished ...>`

```
decode->strdup("hello ltrace" <unfinished ...>
libc.so.6->strlen("")                   = ...  ← libc 内部在做事
libc.so.6->strcmp(...)                  = ...
<... strdup resumed> )                  = 0x609f...  ← strdup 完成了
```

`<unfinished ...>` 和 `<... resumed>` —— 和 strace 中的一样！表示 `strdup` 开始执行但还没完成，其他调用先完成了。这在追踪多线程程序时特别有用。

---

### 第三步：库来源标记 —— `decode->` vs `libc.so.6->`

每行开头的标记告诉你**函数来自哪个库**：

| 标记 | 含义 |
|------|------|
| `decode->` | 你的程序（`decode` 可执行文件）调用的 |
| `libc.so.6->` | glibc 内部调用的（libc 的实现细节） |
| `libm.so.6->` | 数学库（math.h 的函数来自这里） |

**同一个函数名可以出现在不同的来源**：

```
decode->strlen("measure me")     = 10    ← 你的代码调用了 strlen
libc.so.6->strlen("")           = ...   ← libc 内部也在用 strlen
```

**过滤技巧**：只想看你的代码的直接调用？`grep 'decode->'` 过滤掉 libc 内部噪音。

```bash
ltrace ./decode | grep 'decode->'
```

输出只保留你的代码产生的调用：

```
decode->strdup("hello ltrace")
decode->strchr("hello ltrace", 'l')
decode->free(0x...)
decode->fopen("/tmp/...", "r")
decode->memset(0x..., '\0', 32)
decode->memcpy(0x..., "overwritten", 11)
decode->memcmp(0x..., 0x..., 10)
```

清楚多了——7 行 vs 原来的 50+ 行。

---

### 第四步：`-S` —— 同时显示系统调用

`-S` 让 ltrace 在显示库函数调用的同时，也显示这些库函数内部触发的系统调用：

```bash
ltrace -S -e free ./decode 2>&1 | head -5
```

输出：

```
SYS_brk(NULL)                        = 0x...       ← free 内部触发的 syscall
decode->free(0x...)                  = <void>      ← 你的库调用
```

`SYS_` 前缀的行是系统调用（由库函数内部触发），不带前缀的行是库函数调用本身。

> 这让你**用一个命令**同时看到库函数和它触发的系统调用。

---

## 📝 命令小结

| 命令/选项 | 作用 |
|----------|------|
| `ltrace -e 函数1+函数2` | 只追踪指定函数 |
| `grep '程序名->'` | 只看你的代码的调用（过滤 libc 内部噪音） |
| `-S` | 同时显示系统调用 |
| `-o 文件` | 输出到文件 |
| `-s N` | 控制字符串参数的最大显示长度 |

### ltrace 一行输出的完整公式

```
[PID] 库名->函数名(参数列表) = 返回值
  ↑     ↑      ↑         ↑       ↑
  │     │      │         │       └─ 整数/指针/字符串/<void>/nil
  │     │      │         └─ 参数值（字符串会被显示，指针显示地址）
  │     │      └─ 函数名
  │     └─ 来自哪个库（libc, libm, 或你的可执行文件名）
  └─ 可选：进程 PID（多线程/多进程时）
```

---

## 🧪 试试看

1. 用 `ltrace ./decode 2>&1 | grep 'decode->'`，看一共有几个调用？和 `grep 'libc'` 对比
2. 用 `ltrace -S -e fopen+fclose ./decode` 对比——`fopen` 内部触发了几个系统调用？
3. `strchr` 返回 `"llo ltrace"` 而不是 `0x...`——ltrace 怎么知道这是字符串？提示：`char *` 类型的返回值
4. 把 `fopen` 的路径换成一个存在的文件，返回值从 `0` 变成什么？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| libc 内部调用太多看不清 | `printf` 内部有大量 memcpy/strcmp | `grep '你的程序名->'` 过滤 |
| `sqrt`/`pow` 在 ltrace 里看不到 | 可能被编译器内联了 | `-O0 -fno-builtin` 关闭优化 |
| 字符串参数显示不全 | 默认截断长度 | `-s 999` 加长 |
| `= 0` 分不清是成功还是失败 | `fopen` 失败用 `NULL (0)`，`fclose` 成功用 `0` | 需要知道函数的语义——查 `man` |

---

## 下一步

你现在能读懂 ltrace 的每一行输出了。下一步进入阶段二：[实验 03：过滤库调用](../03-filtering/)——用 `-e` 按函数名、按库文件精确过滤，在海量输出中聚焦。
