# 实验 05：Valgrind + GDB 联动 —— 在错误现场深入分析

> 🎯 **目标**：在 Valgrind 检测到内存错误时让程序暂停，用 GDB 远程连接查看完整现场——变量、调用栈、内存内容。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：Valgrind 01-03 + GDB 基础

## 场景

```
Valgrind 报告: "Invalid write of size 4 at hello.c:13"
你: "arr[5] 越界了——但 arr 指向哪？i 在循环中是多少？谁调用的？"

Valgrind 的调用栈只显示函数和行号。
GDB 能让你看到变量值、指针内容、完整内存布局。
```

---

## 先看示例程序

[gdb_vg.c](gdb_vg.c) 有两个故意的 bug：

- 堆越界写入 `arr[3] = 999`
- 未初始化值用于条件判断

```bash
make && ./gdb_vg
```

---

## 实验开始

### 第一步：启动 Valgrind 的 GDB 服务器

```bash
# 终端 1：用 Valgrind 运行程序，启动 vgdb 服务器
valgrind --vgdb=yes --vgdb-error=0 ./gdb_vg
```

关键选项：

| 选项 | 含义 |
|------|------|
| `--vgdb=yes` | 启动 GDB 调试服务器（默认端口） |
| `--vgdb-error=0` | 在**第一个**错误时暂停，等待 GDB 连接（0 = 立即） |
| `--vgdb-error=1` | 在第 1 个错误后暂停（默认是 999,999,999 = 不暂停） |

程序会在出现第一个内存错误时暂停：

```
==...== (action at startup) vgdb me ...
==...== Getting control...
==...== ERROR: Valgrind got a debugger request, waiting...
```

### 第二步：用 GDB 远程连接

```bash
# 终端 2：连接 Valgrind 的 GDB 服务器
gdb ./gdb_vg
```

```bash
(gdb) target remote | vgdb
```

输出：

```
Remote debugging using | vgdb
relaying data between gdb and process ...
0x... in problematic_function () at gdb_vg.c:15
15      arr[3] = 999;
```

> GDB 停在了越界写入**之前**那一行！

### 第三步：在 GDB 中查看现场

现在你可以用所有 GDB 命令：

```bash
(gdb) print arr
$1 = (int *) 0x...        # arr 指向的堆地址

(gdb) print arr[0]
$2 = 10

(gdb) print arr[2]
$3 = 30

(gdb) print arr[3]
$4 = ???                  # 还没执行赋值，但这是越界位置

(gdb) print sizeof(int)
$5 = 4

(gdb) backtrace
#0  problematic_function () at gdb_vg.c:15
#1  main () at gdb_vg.c:40

(gdb) info locals
arr = 0x...
```

> 💡 你现在有了**Valgrind 的错误检测 + GDB 的交互调试**——看到的不只是"哪一行越界了"，而是"arr 有多大、越了几个元素、调用栈长什么样"。

继续执行：

```bash
(gdb) continue
```

Valgrind 检测到第二次错误（未初始化值），再次暂停。再次 `target remote | vgdb` 连接。

### 第四步：检查未初始化值

Valgrind 暂停在未初始化值使用处，GDB 可以看到：

```bash
(gdb) frame 0
(gdb) print value
$6 = 32767              # ★ 垃圾值！在栈上随机分配的
(gdb) print &value
$7 = (int *) 0x...      # 栈地址——不是在堆上
```

> Valgrind 告诉你"Conditional jump depends on uninitialised value"，GDB 让你亲眼看到那个垃圾值是多少。

### 完整双终端工作流

```
终端 1                             终端 2
───────                             ───────
valgrind --vgdb=yes                 gdb ./gdb_vg
  --vgdb-error=0 ./gdb_vg
                                    (gdb) target remote | vgdb
  → 检测到错误，暂停                 → GDB 停在出错行
                                    (gdb) print 变量
                                    (gdb) backtrace
                                    (gdb) info locals
                                    (gdb) continue
  → 继续运行到下一个错误
  → 没有更多错误，程序结束
```

### 和 ASan + GDB 对比

| | Valgrind + GDB | ASan + GDB |
|------|---------------|-----------|
| 如何停止 | `--vgdb-error=0` | `break __asan_report_error` |
| 暂停时机 | 出错**之前** | 出错**之后** |
| 能继续运行吗 | 能 `continue` | 不能（ASan 会终止程序） |
| 性能 | 20x 减速 + GDB 开销 | 2x 减速（但程序会死） |

> Valgrind 的 `--vgdb-error=0` 让你能在出错**前**查看——这是 ASan 做不到的（ASan 在报错后立即终止）。

---

## 📝 命令小结

| 操作 | 命令 |
|------|------|
| Valgrind 启用 GDB 服务器 | `valgrind --vgdb=yes --vgdb-error=0 ./program` |
| GDB 连接 Valgrind | `(gdb) target remote \| vgdb` |
| 继续执行到下一个错误 | `(gdb) continue` |
| 查看 Valgrind 状态 | `(gdb) monitor help`（显示 `vgdb` 可用命令） |

---

## 🧪 试试看

1. 在两个终端中完成完整的 Valgrind+GDB 联动流程——在 GDB 中 `print` 越界前的数组
2. 把 `--vgdb-error=0` 改为 `--vgdb-error=1`——第一个错误后程序还暂停吗？
3. 不用 GDB，只用 Valgrind 的 `--vgdb-error=0` + `vgdb` 命令行工具——`vgdb` 可以直接查询 Valgrind 状态

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `vgdb` 连接失败 | Valgrind 还没启动或已退出 | 确认终端 1 中 Valgrind 正在运行且"waiting" |
| GDB 连接后 `continue` 程序不退出了 | 程序可能在等 GDB 的下一个命令 | `(gdb) continue` 或 `(gdb) detach` |
| `--vgdb-error=0` 总在启动时暂停 | 默认行为——Valgrind 的第一个错误就是程序启动 | 改成 `--vgdb-error=1` 跳过启动 |

---

## 下一步

✅ Valgrind 模块全部完成！🎉

回顾 5 个实验：

```
01 Hello Valgrind     → 不重编译，直接跑
02 ⭐ 未初始化值       → Valgrind 独家超能力
03 四级泄漏           → definitely/indirectly/possibly/reachable
04 Massif & Callgrind → 堆曲线 + 调用图性能分析
05 Valgrind + GDB     → 在错误现场深入调试
```

你现在可以：

- 用 Valgrind 检测任何二进制程序的内存错误（不需要源码！）
- 用 `--track-origins=yes` 追踪未初始化值来源
- 用 `--leak-check=full` 获得四级泄漏分类
- 用 Massif 看堆内存使用曲线
- 用 Callgrind 找 CPU 热点
- 用 Valgrind+GDB 在错误现场做交互式调试

### 🛠️ 五工具全景

```
          │  内存安全  │  源码调试  │  内核交互  │  库调用   │  无重编译
──────────┼───────────┼───────────┼───────────┼──────────┼──────────
  工具     │   ASan    │   GDB     │  strace   │  ltrace  │ Valgrind
  实验数    │   10      │   12      │   8        │   6      │   5
  核心优势  │  最快     │  最灵活   │  最底层   │  中层    │  最详尽
  ⭐独有能力│ 编译时插桩 │  交互调试 │  系统调用  │  库调用   │  未初始化
```

返回 [模块首页](../README.md) 或 [项目主页](../../README.md)。
