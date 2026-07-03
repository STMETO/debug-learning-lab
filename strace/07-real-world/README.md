# 实验 07：实战调试 —— 用 strace 诊断"没有报错的 bug"

> 🎯 **目标**：用 strace 排查三种常见的"静默故障"——文件找不到、权限不够、日志丢失——这些 bug 不会让程序崩溃，但会导致错误行为。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[实验 02](../02-read-output/)（读懂 `= -1 ERRNO`）、[实验 03](../03-filtering/)

## 静默故障 —— 最折磨人的 bug

```
程序没有崩溃。
程序没有报错。
但程序的行为不对。

你在代码里找了 3 小时，printf 加了一堆，还是不知道为什么。
```

strace 在排查这类 bug 时有天然优势——它能看到**所有**系统调用的返回值。

---

## 先看示例程序

[broken.c](broken.c) 是一个"看起来正常、实际上坏得很隐晦"的程序：

- 场景 1：尝试多个路径找配置文件——都找不到，但程序只说"未找到"
- 场景 2：写日志到 `/var/log/myapp/`——失败，但错误被静默吞掉
- 场景 3：从 stdin 读输入、打开文件——可能因各种原因失败

```bash
make && echo "42" | ./broken
```

---

## 实验开始

### 排查场景 1：为什么没读到配置？

运行 strace，只看文件操作：

```bash
strace -e trace=openat ./broken
```

输出中搜索配置文件相关的调用：

```
openat(AT_FDCWD, "/etc/myapp/config.ini", O_RDONLY) = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "./config.ini", O_RDONLY)       = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/tmp/config.ini", O_RDONLY)    = -1 ENOENT (No such file or directory)
```

**三行 `= -1 ENOENT`**：

- `/etc/myapp/config.ini`→ 不存在
- `./config.ini` → 不存在
- `/tmp/config.ini` → 不存在

> 如果没有 strace，你只能看代码中 `printf("未找到配置文件")` 这行输出——但你不知道**它尝试了哪些路径**，也不知道**失败原因是 ENOENT 还是 EACCES**。

修复：

```bash
# 创建一个配置文件试试
echo "debug_mode=true" > /tmp/config.ini
echo "42" | strace -e trace=openat ./broken 2>&1 | grep config
```

现在 `/tmp/config.ini` 返回 `= 3`（成功）。

### 排查场景 2：日志写到哪里去了？

```bash
strace -e trace=openat,write,close ./broken 2>&1 | grep 'app.log\|var/log'
```

输出（关键两行）：

```
openat(AT_FDCWD, "/var/log/myapp/app.log", O_WRONLY|O_CREAT|O_APPEND, 0644) = -1 ENOENT (No such file or directory)
```

> **`= -1 ENOENT`**——`/var/log/myapp/` 这个目录不存在！`open` 带 `O_CREAT` 时可以创建**文件**，但不能创建**目录**。如果父目录不存在，`open` 返回 `ENOENT`。

程序的 bug：`write_log()` 中 `if (fd >= 0)` 的保护让失败**静默**——错误被吞掉，没有日志输出，也没有报错。

**strace 揭露了真相**：日志写入尝试失败了，但程序继续运行，仿佛什么都没发生。

修复：

```bash
mkdir -p /var/log/myapp
echo "42" | ./broken   # 这次日志写入会成功
```

### 排查场景 3：fopen 失败 —— 但为什么？

```bash
echo "42" | strace -e trace=openat ./broken 2>&1 | grep data_
```

```
openat(AT_FDCWD, "/tmp/data_42.txt", O_RDONLY) = -1 ENOENT (No such file or directory)
```

> strac 告诉你两个关键信息：
> 1. 程序尝试打开 `/tmp/data_42.txt`——所以输入值 `42` 被正确解析了 ✅
> 2. 文件不存在——`ENOENT` ❌

如果用 `echo "999" | ./broken` 再跑一次，strace 会显示它尝试打开 `/tmp/data_999.txt`——同样 `ENOENT`。

---

## 📝 实战排查黄金法则

```
1. 先用 strace 跑一次 → 看程序做了什么
2. 搜索 "= -1" → 找到所有失败的系统调用
3. 读 errno → 理解失败原因
4. 回到源代码 → 看失败路径是否有正确处理
5. 修复 → 重新 strace 验证
```

### 五种最常见的 errno 及其修复

| errno | 含义 | 典型修复 |
|-------|------|---------|
| `ENOENT` | 文件/目录不存在 | 创建文件或父目录 |
| `EACCES` | 权限不够 | `chmod` 或换一个有权限的路径 |
| `ECONNREFUSED` | 网络连接被拒 | 确认服务已启动，端口正确 |
| `ENOTDIR` | 路径中的某个部分是文件而非目录 | 检查路径拼接 |
| `ENOMEM` | 内存不足 | 检查是否有内存泄漏 |

---

## 🧪 试试看

1. 用 `strace` 追踪 `./broken`，找出**所有** `= -1` 的行——一共有几个？分别是什么 errno？
2. 在场景 1 中创建 `/tmp/config.ini`，重新跑 strace——`openat` 的返回值从 `= -1` 变成了什么？
3. 写一个程序尝试 `open("/root/test.txt", O_RDONLY)`——strace 会显示什么 errno？
4. 用 `strace -e trace=network curl http://localhost:99999`（一个不存在的服务）——strace 显示的 errno 是什么？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `ENOENT` 但文件确实存在 | 程序使用了相对路径，当前工作目录不对 | 看 `openat` 的路径是相对还是绝对 |
| strace 输出中 `= -1` 很多但程序正常运行 | 某些失败是"试探性的"（如 `access("/etc/ld.so.preload")`） | 正常的——关注你的代码产生的 `= -1` |
| 程序中已经 `perror` 了但终端看不到 | strace 的 stderr 被重定向了 | `-o 文件` 保存 strace 日志 |---

## 下一步

strace 告诉你程序在做什么（外部视角），但它不能看变量值、不能暂停执行。GDB 能做到这些。最后一个实验 [实验 08：strace + GDB 联动](../08-strace-gdb/)——把两个工具结合起来，同时拥有内外部双视角。
