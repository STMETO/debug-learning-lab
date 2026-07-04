# 实验 06：ltrace + GDB 联动 —— 从库函数到源码变量

> 🎯 **目标**：把 ltrace（库函数调用层）和 GDB（源码变量层）结合起来，在函数调用和内部状态之间无缝切换。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：ltrace 01-05 + GDB 01-06

## 双视角 —— 从外到内

```
ltrace 看到：                  GDB 看到：
                              
strlen("alice") = 5           name = "alice", len = 5
strstr("alice","admin") = nil  if 条件不成立
strlen("bob_admin") = 9       name = "bob_admin", len = 9
strstr("bob_admin","admin")    if 条件成立
  = "admin"                   score = 50 + 90 + 100 = 240

ltrace: "函数被调用了什么，返回了什么"
GDB:    "函数内部发生了什么，变量怎么变的"
```

---

## 先看示例程序

[dual.c](dual.c) 有两个场景：

- **场景 1**：`compute_score()`——用 `strlen` 和 `strstr` 计算分数。`strstr` 的返回值（是否找到 "admin"）决定了分数跳变。
- **场景 2**：`build_greeting()`——用 `strlen`+`malloc`+`strcpy`+`strcat` 构建字符串。

```bash
make && ./dual
```

---

## 实验开始

### 场景 1：ltrace 看调用，GDB 看内部

**先用 ltrace 看库函数调用**：

```bash
ltrace -e strlen+strstr ./dual 2>&1 | grep 'dual->'
```

输出：

```
dual->strlen("alice")                = 5
dual->strstr("alice", "admin")       = nil
dual->strlen("bob_admin")            = 9
dual->strstr("bob_admin", "admin")   = "admin"     ← 找到了！
dual->strlen("charlie")              = 7
dual->strstr("charlie", "admin")     = nil
dual->strlen("dave_admin_super")     = 16
dual->strstr("dave_admin_super", "admin") = "admin" ← 找到了！
dual->strlen("eve")                  = 3
dual->strstr("eve", "admin")         = nil
```

> ltrace 告诉你：`strstr` 在 `bob_admin` 和 `dave_admin_super` 中找到了 `"admin"`——但**为什么这会导致分数跳变**？ltrace 看不到 C 代码的逻辑。

**再用 GDB 看内部状态**：

```bash
gdb ./dual
```

```bash
(gdb) break compute_score
(gdb) run
```

第一次停住（`alice`，不含 admin）：

```bash
(gdb) print name
$1 = 0x... "alice"

(gdb) print len
$2 = 47        ← 还没执行 strlen！这个值是垃圾

(gdb) next      # 执行 strlen(name)
(gdb) print len
$3 = 5         ← strlen("alice") = 5

(gdb) next      # 执行 score = base + len * 10
(gdb) print score
$4 = 100       ← 50 + 5*10 = 100

(gdb) next      # 执行 strstr
(gdb) next      # 执行 if 判断
# if 条件不成立（strstr 返回 nil），跳过 score += 100
```

继续到 `bob_admin`（含 admin）：

```bash
(gdb) continue
(gdb) print name
$5 = 0x... "bob_admin"

(gdb) next 3    # 执行 strlen + 计算 + strstr + if
(gdb) print score
$6 = 240       ← 50 + 90 + 100 = 240（加了 100！）
```

> **GDB 揭示了为什么分数跳变**：`strstr` 返回非 NULL → `if` 条件成立 → `score += 100`。

### 并排解读：ltrace + GDB 协同

| 用户 | ltrace | GDB |
|------|--------|-----|
| alice | `strlen("alice") = 5` | `len = 5`, `score = 50+50 = 100` |
| alice | `strstr("alice","admin") = nil` | if 跳过, `score` 还是 100 |
| bob_admin | `strlen("bob_admin") = 9` | `len = 9`, `score = 50+90 = 140` |
| bob_admin | `strstr("bob_admin","admin") = "admin"` | if 执行, `score = 140+100 = 240` |

> ltrace 告诉你"**什么被调用了**"，GDB 告诉你"**为什么会导致这个结果**"。

### 场景 2：ltrace 看调用链，GDB 看缓冲区

```bash
ltrace -e strlen+malloc+strcpy+strcat+free ./dual | grep 'dual->'
```

输出：

```
dual->strlen("Alice")                       = 5
dual->malloc(25)                            = 0x...
dual->strcat("Hello, ", "Alice")            = "Hello, Alice"
dual->strlen("Hello, Alice")                = 12
dual->free(0x...)                           = <void>
```

GDB 验证中间状态：

```bash
(gdb) break build_greeting
(gdb) run
(gdb) print nlen
$1 = 5
(gdb) print buf
$2 = 0x... ""
(gdb) next    # 执行 strcpy
(gdb) print buf
$3 = 0x... "Hello, "
(gdb) next    # 执行 strcat
(gdb) print buf
$4 = 0x... "Hello, Alice!"
```

> ltrace 展示调用序列（strlen→malloc→strcat→strlen→free），GDB 展示缓冲区内容的逐步变化（"" → "Hello, " → "Hello, Alice!"）。

---

## 📝 四工具决策矩阵

完成四个模块后，你现在拥有了完整的调试工具箱：

| 问题类型 | 第一工具 | 辅助工具 |
|---------|---------|---------|
| 内存越界/泄漏/UAF | **ASan** | GDB（深入现场） |
| "程序在做什么？卡住了？" | **strace** | ltrace（库调用层） |
| "库函数调了什么？参数是什么？" | **ltrace** | strace（底层） |
| "变量值怎么变的？逻辑对不对？" | **GDB** | ltrace（函数调用上下文） |
| 性能瓶颈——哪个调用慢？ | **strace -T** 或 **ltrace -c** | GDB（内部循环逻辑） |
| 程序崩溃但不在现场 | **ASan** + core dump → **GDB** | strace（最后几条系统调用） |

---

## 🧪 试试看

1. 用 ltrace 追踪 `./dual`，找出哪个 `strstr` 返回了非 nil——然后在 GDB 里验证对应的 `score` 值
2. 在 `build_greeting` 中改 `malloc(nlen + 20)` 为 `malloc(nlen)`（太小了），ltrace 能看到这个 bug 吗？ASan 呢？
3. 用 strace + ltrace + GDB 三个工具追踪同一个程序——各能看到什么独特的视角？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| GDB 中 `print` 的值和 ltrace 参数不一致 | 编译器优化改变了参数传递顺序 | `-O0` 编译 |
| ltrace 看到某函数被调用但 GDB 断不住 | 函数可能被 PLT 跳转优化 | 在调用该函数的那一行设断点 |

---

## 下一步

🟠 ltrace 模块全部完成！🎉

回顾 6 个实验：

```
阶段一：Hello ltrace → 读懂输出格式
阶段二：过滤库调用 → 时间与统计
阶段三：ltrace vs strace → ltrace + GDB
```

你现在可以：

- 用 `ltrace ./program` 看任何程序的库函数调用
- 用 `-e` 按函数名/库名过滤
- 用 `-t -T -c` 分析时序和统计
- 用 ltrace + strace 建立三层调用视角
- 用 ltrace + GDB 从库调用深入到源码变量

### 🛠️ 四工具全景

```
          │  内存安全  │  源码调试  │  内核交互  │  库调用层
──────────┼───────────┼───────────┼───────────┼──────────
  工具     │   ASan    │   GDB     │  strace   │  ltrace
  检测方式  │  自动报警  │  手动断点  │  被动追踪  │  被动追踪
  核心输出  │  内存报告  │ 变量/栈   │  系统调用  │  库函数
  实验数    │   10      │   12      │   8        │   6
```

返回 [模块首页](../README.md) 或 [项目主页](../../README.md)。
