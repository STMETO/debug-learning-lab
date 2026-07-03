# 实验 04：查看变量 —— 不再需要 printf

> 🎯 **目标**：学会用 `print` 查看各种类型的数据，以及 `display` 自动显示、`info locals` 一键列出所有局部变量。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[实验 01–03](../)——你需要会设断点、单步执行

## 先看示例程序

[data.c](data.c) 故意在一个函数里塞了各种数据类型：

| 类型 | 示例 |
|------|------|
| `int` / `double` / `char` | 基本类型 |
| 数组 `int[]` `char[]` | 连续内存 |
| 结构体 `struct Student` | 复合类型 |
| 指针 `int *` | 间接访问 |

```bash
make && ./data
```

---

## 实验开始

```bash
gdb ./data
```

我们停在所有变量初始化完成之后的行：

```bash
(gdb) break 35          # printf("=== 检查点 ===\n") 这行
(gdb) run
```

程序停在检查点，此时所有变量都已就绪。

### 第一步：print 的基本用法

```bash
(gdb) print local_int
$1 = 50

(gdb) print local_double
$2 = 7.8500000000000005

(gdb) print local_char
$3 = 90 'Z'
```

> GDB 同时显示 ASCII 码值（90）和字符表示（'Z'）。

`$1`、`$2`、`$3` 是 GDB 的**值历史（value history）**。你可以随时引用之前的结果：

```bash
(gdb) print $1 + $2
$4 = 57.85
```

### 第二步：格式化输出

`print` 默认按变量类型显示。你可以用 `/格式` 改变显示方式：

```bash
(gdb) print local_int
$5 = 50

(gdb) print/x local_int        # 十六进制
$6 = 0x32

(gdb) print/t 42               # 二进制
$7 = 101010

(gdb) print/c local_char       # 作为字符显示
$8 = 90 'Z'
```

| 格式符 | 含义 | 示例输出 |
|--------|------|---------|
| `/x` | 十六进制 | `0x32` |
| `/d` | 有符号十进制（默认） | `50` |
| `/u` | 无符号十进制 | `50` |
| `/t` | 二进制 | `101010` |
| `/c` | 字符 | `'Z'` |
| `/f` | 浮点 | `7.85` |
| `/a` | 地址 | `0x7fff...` |

### 第三步：打印字符串和数组

```bash
(gdb) print word
$9 = "hello"

(gdb) print primes
$10 = {2, 3, 5, 7, 11}
```

> 数组默认打印所有元素。如果数组很大，你可以限定范围：

```bash
(gdb) print primes[0]@3       # 从第 0 个元素开始，显示 3 个
$11 = {2, 3, 5}
```

`@` 语法：`数组名[起始索引]@个数`。

### 第四步：打印结构体

```bash
(gdb) print stu
$12 = {name = "Alice", age = 20, score = 92.5}
```

也可以单独看某个字段：

```bash
(gdb) print stu.name
$13 = "Alice"

(gdb) print stu.age
$14 = 20
```

### 第五步：打印指针

```bash
(gdb) print ptr_int
$15 = (int *) 0x7ffdf6294d44       # 指针本身的值（地址）

(gdb) print *ptr_int                # 解引用：指针指向的内容
$16 = 50

(gdb) print ptr_array
$17 = (int *) 0x7ffdf6294d60

(gdb) print *ptr_array              # 数组第一个元素
$18 = 2
```

### 第六步：display —— 每次停都自动显示

`print` 是你主动去"问"GDB。`display` 不同——你告诉 GDB"每次停下来的自动给我看这个"。

```bash
(gdb) display local_int
(gdb) display stu.name
(gdb) display/x local_int          # 也可以带格式
```

现在用 `next` 单步几次，你会看到每次停止时 GDB 自动输出这些值，不用再手动 `print`。

```bash
(gdb) info display                 # 查看所有自动显示项
(gdb) undisplay 1                  # 取消编号为 1 的自动显示
(gdb) undisplay                    # 取消全部
```

### 第七步：info locals 和 info args —— 一键查看

当你停在一个函数里时，不想逐个 `print`：

```bash
(gdb) info locals
```

输出：

```
local_int = 50
local_double = 7.8500000000000005
local_char = 90 'Z'
primes = {2, 3, 5, 7, 11}
word = "hello"
stu = {name = "Alice", age = 20, score = 92.5}
ptr_int = 0x7ffdf6294d44
ptr_array = 0x7ffdf6294d60
```

所有局部变量一次性列出来。

```bash
(gdb) info args
```

输出：

```
count = 5
ratio = 3.1400000000000001
label = 0x... "debug-me"
```

> 💡 **黄金组合**：停在某个函数里 → `info args` 看谁传了什么 → `info locals` 看函数内部算出了什么。这就是 printf 调试做不到的效率。

### 第八步：print 作为计算器

GDB 里的 `print` 可以执行任意 C 表达式：

```bash
(gdb) print local_int * 2 + 10
$19 = 110

(gdb) print primes[2] + primes[3]
$20 = 12                          # 5 + 7

(gdb) print sizeof(word)
$21 = 6                           # "hello" = 5 个字符 + 1 个 '\0'
```
> `sizeof(word)` = 6 但肉眼只看到 5 个字母——多出来的 1 字节是 C 字符串末尾的 `'\0'`（null 终止符）。这就是"hello"实际占用的内存大小。

---

## 📝 命令小结

| 命令 | 简写 | 作用 |
|------|------|------|
| `print 表达式` | `p x` | 求值并打印 |
| `print/x 表达式` | `p/x x` | 以十六进制打印 |
| `print 数组[0]@5` | — | 打印数组的前 5 个元素 |
| `display 表达式` | `disp x` | 每次停止时自动显示 |
| `info display` | `i disp` | 查看自动显示列表 |
| `undisplay 编号` | `und 1` | 取消自动显示 |
| `info locals` | `i lo` | 列出所有局部变量 |
| `info args` | `i ar` | 列出函数参数 |

## 🧪 试试看

1. 试试 `print/t` 把 `local_int` 以二进制打印出来，你能看懂吗？
2. 用 `display` 同时监控 `local_int` 和 `stu.score`，然后连续 `next`，观察值的变化
3. 试试 `print sizeof(stu)`——你能算出这个结构体占多少字节吗？
4. 在 `main` 里设断点，`info locals` 能看到什么？和停在 `examine_data` 里有什么不同？
5. 试着 `print nonexistent_var`，GDB 会怎么报错？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `print word` 显示乱码 | 停在变量**初始化之前**了 | 确保停在初始化语句之后 |
| `print *ptr_int` 报 `Cannot access memory` | 指针为 `NULL` 或未初始化 | 先 `print ptr_int` 看地址是否为 0 |
| `info locals` 看不到全局变量 | `info locals` 只看**局部**变量 | 全局变量直接用 `print` |
| `print strlen(x)` 导致 GDB 崩溃 | `strlen`/`memcpy`/`memset` 是 glibc 的 IFUNC 函数，GDB 无法调用 | 用 `print sizeof(x)` 或 `print x` 代替 |

---

## 下一步

现在你能在某个暂停点"看见"程序的所有内部状态了。但如果程序有几十层函数调用，你不可能一个个进入再 `info locals`。下一步学习 [实验 05：调用栈](../05-backtrace/)——在任意位置查看整个函数调用链，并在不同栈帧之间跳转。
