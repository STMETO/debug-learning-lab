# 实验 02：读懂 strace 输出 —— 每一个字符都有含义

> 🎯 **目标**：深入理解 strace 输出的完整格式——返回值、错误码、耗时、信号、未完成调用。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[实验 01](../01-hello-strace/)（你至少跑过一次 `strace ./hello`）

---

## 先看示例程序

[decode.c](decode.c) 故意产生了各种"口味"的系统调用：

- ✅ 成功的：`open`、`read`、`write`、`lseek`
- ❌ 失败的：`open` 不存在的文件、`open` 没有权限的路径
- ⏱️ 慢的：`sleep(1)`（对比其他微秒级调用）

```bash
make && ./decode
```

---

## 实验开始

### 第一步：带耗时运行

```bash
strace -T ./decode
```

`-T` 在每行末尾追加耗时。看输出中的几个代表性行：

### 第二步：解剖四类返回值

#### 类型 A：成功返回整数（fd / 字节数 / 位置）

```
openat(AT_FDCWD, "/tmp/strace_test.txt", O_RDONLY) = 3 <0.000099>
 │                                                        │   │
 │                                                        │   └─ 耗时 99 微秒
 │                                                        └─ 返回值：fd = 3
 └─ 系统调用 + 参数
```

```
read(3, "Hello, strace!\nLine 2\nLine 3\n", 63) = 29 <0.000086>
 │    │                                    │    │
 │    │                                    │    └─ 返回 29 = 实际读到的字节数
 │    │                                    └─ 请求读 63 字节
 │    └─ fd = 3
 └─ 系统调用
```

```
lseek(3, 5, SEEK_SET) = 5 <0.000087>
      │  │  │            │
      │  │  └─ SEEK_SET = 从开头算起   └─ 返回新位置 = 5（第 6 个字节处）
      │  └─ 偏移量 = 5
      └─ fd = 3
```

| 系统调用 | 返回值的含义 |
|---------|------------|
| `openat` → `= 3` | 文件描述符（非负数 = 成功） |
| `read` → `= 29` | 实际读到的字节数 |
| `write` → `= 34` | 实际写入的字节数 |
| `lseek` → `= 5` | 新的文件偏移位置 |

#### 类型 B：失败返回 -1（带 errno）

```
openat(AT_FDCWD, "/tmp/does_not_exist_xyz.txt", O_RDONLY) = -1 ENOENT (No such file or directory)
                                                                  │   │
                                                                  │   └─ 人类可读的解释
                                                                  └─ errno 宏名
```

公式：`= -1 ERRNO (解释文字)`

| errno | 含义 | 典型原因 |
|-------|------|---------|
| `ENOENT` | No such file or directory | 文件不存在 |
| `EACCES` | Permission denied | 没有读/写/执行权限 |
| `ECONNREFUSED` | Connection refused | 网络端口没监听 |
| `ENOMEM` | Cannot allocate memory | 内存不足 |
| `EINVAL` | Invalid argument | 传给系统调用的参数非法 |

> 💡 **调试技巧**：用 strace 排查程序故障时，**先搜索 `= -1`**。找到的第一个错误往往就是根因。

#### 类型 C：不返回（或者返回不重要）

```
exit_group(0) = ?
```

`= ?` 表示这个调用不会正常返回——`exit_group` 直接终止进程，所以没有返回值可打印。

#### 类型 D：耗时异常

```
clock_nanosleep(CLOCK_REALTIME, 0, {tv_sec=1, tv_nsec=0}, ...) = 0 <1.000310>
```

对比"正常"调用的耗时：

| 系统调用 | 耗时 | 数量级 |
|---------|------|--------|
| `read` | `<0.000086>` | 86 微秒 |
| `openat`（成功） | `<0.000099>` | 99 微秒 |
| `clock_nanosleep` | `<1.000310>` | **1,000,310 微秒** |

> `clock_nanosleep` 的 1 秒耗时比其他调用大 10000 倍——`-T` 让你一眼看出"谁在拖慢程序"。

### 第三步：strace 的 stderr 和程序的 stdout 混在一起

注意这一行的输出：

```
write(1, "...", 644) = 644 <0.000167>
  directly to stdout via write()     ← 这是程序的 stdout 输出
=== strace 输出格式实验 ===           ← 混在 strace 的 stderr 输出中
```

strace 默认把报告写到 **stderr（fd=2）**，你的程序正常输出到 **stdout（fd=1）**。两者在终端里混合显示。

**分离它们**：

```bash
# 把 strace 报告保存到文件
strace -o trace.log ./decode

# 看程序自己的输出（干净）
./decode 2>/dev/null

# 只看 strace 报告（丢弃程序输出）
strace ./decode >/dev/null
```

### 第四步：控制字符串长度

strace 默认截断长字符串到 32 字节（加上 `...`）。用 `-s` 控制：

```bash
# 完整显示所有字符串参数
strace -s 999 ./decode

# 只显示前 10 字节
strace -s 10 ./decode
```

对比 `read` 的输出：

```
# 默认：截断的字符串
read(3, "Hello, strace!\nLine 2\nLine 3\n", 63) = 29

# -s 10：更短
read(3, "Hello, str"..., 63) = 29
```

### 第五步：十六进制输出

用 `-x` 将非 ASCII 字符以十六进制显示：

```bash
strace -x ./decode
```

输出中字符串里的特殊字符会变成 `\xNN` 格式——排查二进制协议或编码问题时非常有用。

### 第六步：信号的表示

虽然我们这个程序没有收到信号，但 strace 中信号的标准格式是：

```
--- SIGINT {si_signo=SIGINT, si_code=SI_USER, ...} ---
--- SIGSEGV {si_signo=SIGSEGV, si_code=SEGV_MAPERR, ...} ---
```

`---` 开头的行表示进程收到了信号。`SIGINT` 是 Ctrl-C，`SIGSEGV` 是段错误。

---

## 📝 命令小结

| 选项 | 作用 |
|------|------|
| `-T` | 在每行末尾显示系统调用耗时 |
| `-o 文件` | 将 strace 报告写入文件（不混入终端输出） |
| `-s N` | 控制字符串参数的最大长度（默认 32） |
| `-x` | 非 ASCII 字符以十六进制显示 |
| `-xx` | 所有字符串以十六进制显示 |
| `-v` | 详细模式——结构体字段完整展开 |

---

## strace 一行输出的完整公式

```
系统调用名(参数1, 参数2, ...) = 返回值 <耗时>
```

| 部分 | 可能出现的形式 |
|------|-------------|
| 系统调用名 | `openat`, `read`, `write`, `close`, `exit_group`... |
| 参数 | 整数、字符串、标志位（用 `\|` 组合）、结构体 `{...}` |
| 返回值 | `= N`（成功），`= -1 ERRNO (说明)`（失败），`= ?`（不返回） |
| 耗时 | `<0.000180>` 秒，`-T` 时出现 |

---

## 🧪 试试看

1. 用 `strace -T ./decode` 找出耗时最长的 3 个系统调用——它们各花了多长时间？
2. 修改 `decode.c`，让它尝试 `open("/dev/null/foo", ...)`——这会产生什么 errno？（`ENOTDIR`？）
3. 用 `strace -o /tmp/trace.log ./decode`，然后 `cat /tmp/trace.log`——和直接运行有什么不同？
4. 用 `-s 0` 运行——字符串参数会显示什么？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| strace 输出和程序输出混在一起 | stdout 和 stderr 都在终端 | `-o 文件` 或 `2>/dev/null` |
| 字符串被截断显示 `...` | 默认截断长度 32 | `-s 999` 加长 |
| `= -1 ENOENT` 但文件明明存在 | 程序使用了相对路径，当前工作目录不对 | 看 `openat` 的第一个参数是否是 `AT_FDCWD` |
| 看不到 `nanosleep`，看到的是 `clock_nanosleep` | 现代 glibc 使用 `clock_nanosleep` 实现 `sleep()` | 正常——它们是等价的 |

---

## 下一步

你已经能完整读懂 strace 的每一行输出了。下一步进入阶段二：[实验 03：过滤系统调用](../03-filtering/)——用 `-e trace=` 在成百上千个调用中只看你关心的那几种。
