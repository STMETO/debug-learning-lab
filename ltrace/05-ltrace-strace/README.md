# 实验 05：ltrace vs strace —— 同一个程序，两个完全不同的视角

> 🎯 **目标**：把 ltrace（库函数）和 strace（系统调用）的追踪结果并排对比，理解 C 代码→库函数→系统调用的完整调用链。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：ltrace 01-04 + strace 01-05

---

## 先看示例程序

[compare.c](compare.c) 做了三件事：

1. 文件写入（`fopen` → `fprintf` → `fclose`）
2. 文件读取（`fopen` → `fgets` → `fclose`）
3. 内存操作（`malloc` → `strcpy` → `free`）

```bash
make && ./compare
```

---

## 实验开始

### 第一步：ltrace 视角 —— 库函数层

```bash
ltrace -e fopen+fprintf+fclose+fgets+malloc+free ./compare 2>&1 | grep 'compare->'
```

输出：

```
compare->fopen("/tmp/compare_demo.txt", "w") = 0x...
compare->fclose(0x...)                       = 0
compare->fopen("/tmp/compare_demo.txt", "r") = 0x...
compare->fgets("Line 1: ...", 128, ...)     = 0x...
compare->fgets("Line 2: ...", 128, ...)     = 0x...
compare->fgets("Line 2: ...", 128, ...)     = 0       ← EOF
compare->fclose(0x...)                       = 0
compare->malloc(1024)                        = 0x...
compare->malloc(4096)                        = 0x...
compare->free(0x...)                         = <void>
compare->free(0x...)                         = <void>
```

> 和 C 代码几乎一一对应。`fgets` 调了 3 次——前两次有数据，第三次返回 `0`（EOF）。

### 第二步：strace 视角 —— 系统调用层

```bash
strace -e trace=openat,write,read,close,brk ./compare 2>&1
```

输出：

```
openat(AT_FDCWD, "/tmp/compare_demo.txt", O_WRONLY|O_CREAT|O_TRUNC, 0666) = 3
write(3, "Line 1: data from compare demo\nL"..., 49) = 49
close(3)                                = 0
openat(AT_FDCWD, "/tmp/compare_demo.txt", O_RDONLY) = 3
read(3, "Line 1: data from compare demo\nL"..., 4096) = 49
read(3, "", 4096)                       = 0       ← EOF
close(3)                                = 0
brk(NULL)                               = 0x...  ← malloc 内部触发
brk(0x...)                              = 0x...  ← 扩展堆空间
```

### 第三步：并排对比 —— 三层调用映射

把同一个操作的 C 代码、ltrace、strace 三栏并排：

| C 代码 | ltrace（库调用） | strace（系统调用） |
|--------|-----------------|-------------------|
| `fopen("f.txt","w")` | `fopen("f.txt","w") = 0x...` | `openat(..., "f.txt", O_WRONLY\|O_CREAT\|O_TRUNC, 0666) = 3` |
| `fprintf(fp,"Line1\n")` | （被优化为 `fwrite`） | `write(3, "Line 1...", ...) = N` |
| `fclose(fp)` | `fclose(0x...) = 0` | `close(3) = 0` |
| `fgets(buf,128,fp)` | `fgets(..., 128, 0x...) = 0x...` | `read(3, ..., 4096) = N` |
| `fgets(buf,128,fp)`（EOF） | `fgets(...) = 0` | `read(3, "", 4096) = 0`（返回 0） |
| `malloc(1024)` | `malloc(1024) = 0x...` | `brk(0x...) = 0x...` |
| `free(p)` | `free(0x...) = <void>` | （通常没有系统调用） |

**关键发现**：

1. **`fopen` → `openat`**：`fopen` 在 `openat` 上加了一层缓冲（`FILE *` 结构体）。`openat` 返回的是整数 fd=3，`fopen` 返回的是文件指针 `0x...`。

2. **`fgets` → `read`**：`fgets` 请求读 "一行"（最大 128 字节），但 strace 显示实际 `read` 一次读了 **4096 字节**——这是 glibc 的缓冲策略：多读一些，减少系统调用次数。

3. **`malloc(1024)` → `brk`**：`malloc` 管理堆内存池。小分配不触发系统调用——只有当池子不够时才 `brk` 向内核扩展。

4. **`free` → 没有系统调用**：`free` 只是把内存归还给堆管理器（用户态），不涉及内核。

### 第四步：三层调用模型

```
┌─────────────────────────────────────────────┐
│ 第一层：你的 C 代码                            │
│   fopen("f.txt", "w")                       │
│   fprintf(fp, "hello")                      │
│       ↓                                     │
│ 第二层：库函数（ltrace 看到这里）               │
│   fopen("f.txt", "w") → FILE *              │
│   fwrite("hello", 1, 5, fp) → 5            │
│       ↓                                     │
│ 第三层：系统调用（strace 看到这里）              │
│   openat(..., "f.txt", O_WRONLY|...) → 3    │
│   write(3, "hello", 5) → 5                 │
│       ↓                                     │
│   Linux 内核                                 │
└─────────────────────────────────────────────┘
```

> 理解这个三层模型，你就能把任何程序的行为同时从库函数和系统调用两个维度来分析了。

### 什么时候用 ltrace，什么时候用 strace？

| 问题 | 用 ltrace | 用 strace |
|------|----------|----------|
| "`fopen` 传了什么路径？返回了什么？" | ✅ 直接看到 | ❌ 只能看到 `openat` |
| "一共读了多少字节？每次 `write` 写多少？" | ❌ `fwrite` 只看参数 | ✅ `write(3, ..., N)` |
| "`malloc` 分配了多少次？泄漏了吗？" | ✅ 直接看到 `malloc`/`free` 配对 | ❌ `brk` 粒度过粗 |
| "`read` 为什么返回 0？errno 是什么？" | ❌ ltrace 不显示 errno | ✅ `= -1 EACCES ...` |
| "程序为什么卡住了？" | ❌ | ✅ 看最后一个系统调用 |

---

## 🧪 试试看

1. 同时用 `ltrace` 和 `strace` 追踪 `ls /tmp`——两个工具看到的输出有什么不同？
2. 修改 `compare.c`，让 `fopen` 打开一个不存在的文件，分别用 ltrace 和 strace 追踪——哪个更容易诊断 `ENOENT`？
3. `fprintf` 在 ltrace 中被显示为 `fwrite`——这是为什么？（提示：编译器优化，和实验 01 的 `printf`→`puts` 类似）

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| ltrace 和 strace 看到的行数差别很大 | 一个 `fopen` 可能触发多个系统调用 | 正常的——这是缓冲和内核开销 |
| strace 中 `read` 读的字节数比 ltrace 中 `fgets` 请求的多 | glibc 缓冲——一次 `read` 大块读取，`fgets` 从中取行 | 正常的缓冲策略 |
| ltrace 看不到某些函数调用 | 函数被编译器内联或静态链接 | `-O0 -fno-builtin` |

---

## 下一步

你已经建立了"代码→库→系统调用"的三层视角。最后一个实验 [实验 06：ltrace + GDB 联动](../06-ltrace-gdb/)——把 ltrace 的库调用视角和 GDB 的变量/执行流视角结合起来，完成四工具调试闭环。
