# 实验 06：观察点 —— 谁动了我的变量？

> 🎯 **目标**：用 `watch` 监控变量，当它的值被修改时自动暂停程序。
>
> ⏱️ 约 10 分钟
>
> 📖 **前置**：[实验 02](../02-breakpoints/)、[实验 04](../04-inspecting/)

## 这是什么场景？

想象一个真实 bug：

> 程序里有一个全局变量 `counter`，运行结果总是比你预期的大一点。
> 你在几十个函数里都用了这个变量——但**不知道是哪个函数在什么时候多改了一次**。
> 你可以在每个函数里加 `printf`，改代码、重新编译、再跑……或者你可以用观察点。

**观察点（watchpoint）** 让你直接对 GDB 说：

> "帮我盯着 `counter`，不管是谁、在哪个函数里改了它，立刻停下来告诉我。"

---

## 先看示例程序

[mystery.c](mystery.c) 模拟了这种场景——`counter` 在 5 个阶段中被不断修改：

```
phase_one:   counter += 10      → 10
phase_two:   counter *= 3       → 30
phase_three: counter -= 4       → 26
phase_four:  counter = c²       → 676
phase_five:  helper_a/b         → 679
```

```bash
make && ./mystery
```

---

## 实验开始

```bash
gdb ./mystery
```

### 第一步：设置观察点

```bash
(gdb) watch counter
```

输出：

```
Hardware watchpoint 1: counter
```

> "Hardware" 说明 CPU 的硬件调试寄存器在帮你监控——几乎不影响程序运行速度。

### 第二步：运行并观察

```bash
(gdb) run
```

第一次触发——程序在 `phase_one()` 里修改了 `counter`：

```
Hardware watchpoint 1: counter

Old value = 0
New value = 10
phase_one () at mystery.c:11
11      }
```

看这行输出：
- **Old value = 0**：修改前 counter 的值
- **New value = 10**：修改后 counter 的值
- **phase_one()**：是谁改的
- **mystery.c:11**：改完之后停在哪个位置

```bash
(gdb) print counter
$1 = 10
```

> GDB 停在**修改语句的下一条指令**（函数末尾的 `}`）。虽然不能直接停在 `counter += 10` 这一行，但 `Old/New value` + 函数名已经足够定位了。

继续：

```bash
(gdb) continue
```

第二次触发：

```
Hardware watchpoint 1: counter

Old value = 10
New value = 30
phase_two () at mystery.c:15
15      }
```

第三次 `continue`：

```
Old value = 30
New value = 26
phase_three () at mystery.c:19
```

第四次 `continue`：

```
Old value = 26
New value = 676
phase_four () at mystery.c:23
```

> 你在不用任何断点的情况下，追踪到了 `counter` 的每一次变化。如果这是真实 bug，你已经找到所有嫌疑人。

### 第三步：查看观察点状态

```bash
(gdb) info watchpoints
```

输出：

```
Num     Type           Disp Enb Address            What
1       hw watchpoint  keep y                      counter
        breakpoint already hit 4 times
```

和普通断点一样，观察点也可以 `delete`、`disable`、`enable`：

```bash
(gdb) delete 1          # 删除观察点
(gdb) continue           # 程序跑完
```

---

## 断点 vs 观察点

| | 断点 (breakpoint) | 观察点 (watchpoint) |
|------|------|------|
| 触发条件 | 程序运行到**某一行** | **某个变量**的值被修改 |
| 你关注的是 | "程序走到了哪里" | "谁改了哪个变量" |
| 适用场景 | 检查某个函数的逻辑 | 追踪意外被修改的变量 |
| 命令 | `break 位置` | `watch 变量名` |

> 断点回答"何时"（when），观察点回答"谁"（who）。

### 读取观察点

如果不仅写操作要触发，**读取**也要触发：

```bash
(gdb) rwatch counter       # 读取时触发（read watchpoint）
(gdb) awatch counter       # 读取或写入都触发（access watchpoint）
```

### 观察结构体字段

```bash
(gdb) watch stu.score       # 只监控结构体的某个字段
(gdb) watch *ptr_int        # 监控指针指向的内容（不是指针本身）
```

### 局部变量的观察点有陷阱

```bash
(gdb) watch local_int       # ⚠ 小心！
```

> 如果 `local_int` 是函数的局部变量，当函数返回后，观察点会失效（变量所在的内存被回收了）。全局变量和堆变量更适合用 `watch`。

---

## 📝 命令小结

| 命令 | 简写 | 作用 |
|------|------|------|
| `watch 变量` | `wa counter` | 写入时触发（最常用） |
| `rwatch 变量` | `rw counter` | 读取时触发 |
| `awatch 变量` | `aw counter` | 读或写都触发 |
| `info watchpoints` | `i watch` | 查看所有观察点 |
| `delete 编号` | `d 1` | 删除观察点 |

## 🧪 试试看

1. 对 `counter` 设观察点，用 `run` 跑完全程，数一数 `counter` 一共被修改了几次？
2. 试试在 `phase_five` 的执行过程中，观察点触发了多少次？（提示：`helper_a` 和 `helper_b` 各改了一次）
3. 如果把 `watch counter` 改为 `awatch counter`，什么时候会停下来？在 `printf` 里会触发吗？
4. 删除所有断点和观察点后，在 `counter` 上设普通 `break` 能抓到修改吗？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `watch` 报 `No symbol "x" in current context` | 变量不在作用域内 | 先 `break` 进入包含变量的函数再设 `watch` |
| 观察点触发但看不到 Old value | 变量是第一次被写 | 第一次从"不存在"变为"存在的值"时 Old value 可能为空 |
| 程序变慢了 | 软件观察点（`Software watchpoint`） | 如果 CPU 硬件断点不够用，GDB 会用软件模拟——每执行一条指令检查一次，非常慢 |

---

## 下一步

🟡 阶段二已全部完成！你现在掌握了：

- [实验 04](../04-inspecting/)——**看数据**（`print`、`display`、`info locals`）
- [实验 05](../05-backtrace/)——**看路径**（`backtrace`、`frame`、`up/down`）
- [实验 06](../06-watchpoints/)——**盯变量**（`watch`、`rwatch`、`awatch`）

这些已经是日常调试中最常用的武器了。下一步进入 🟠 阶段三：[实验 07：条件断点](../07-conditional-break/)，解决一个新问题——"断点触发太频繁，我只想在 `i == 42` 的时候停下来"。
