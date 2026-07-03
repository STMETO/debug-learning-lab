# 实验 06：重复释放与非法释放 —— 释放不该释放的东西

> 🎯 **目标**：理解 double-free 和 invalid-free 的 ASan 报告，学会追溯重复释放的根因。
>
> ⏱️ 约 10 分钟
>
> 📖 **前置**：[实验 05](../05-use-after-free/)（你需要会看 use-after-free 的三段式报告）

## 这些 bug 长什么样？

```c
free(ptr);
free(ptr);           // ★ double-free：释放了两次

int x;
free(&x);            // ★ invalid-free：试图 free 栈上的变量

char *p = malloc(100);
free(p + 50);        // ★ invalid-free：释放的不是 malloc 返回的原指针
```

---

## 先看示例程序

[dfree.c](dfree.c) 有四种释放相关的 bug：

| 模式 | 操作 | 严重程度 |
|------|------|---------|
| 1 | 同一个变量 `free` 两次 | 🔴 经典 double-free |
| 2 | 两个指针指向同一块内存，各 `free` 一次 | 🔴 别名 double-free |
| 3 | `free(&stack_var)` | 🟠 非法释放栈变量 |
| 4 | `free(malloc_ptr + 50)` | 🟠 释放偏移指针 |

```bash
make
./dfree 1    # 经典 double-free
./dfree 2    # 别名 double-free
./dfree 3    # free 栈变量
./dfree 4    # free 偏移指针
```

---

## 实验开始

### 模式 1：经典 double-free —— `./dfree 1`

```
=================================================================
==...==ERROR: AddressSanitizer: attempting double-free on 0x504000000010 in thread T0:
    #0 ... in __interceptor_free
    #1 ... in classic_double_free dfree.c:21   ← ① 第二次 free
    #2 ... in main dfree.c:77
```

注意！这次 **没有** `WRITE` 或 `READ` 行——因为 `free` 本身不是内存读写，而是内存管理操作。

接着看报告的第二段和第三段：

```
0x504000000010 is located 0 bytes inside of 40-byte region [...]
freed by thread T0 here:                        ← ② 第一次 free（合法的）
    #0 ... in __interceptor_free
    #1 ... in classic_double_free dfree.c:18   ← free(ptr) 首次
    #2 ... in main dfree.c:77

previously allocated by thread T0 here:          ← ③ 分配点
    #0 ... in __interceptor_malloc
    #1 ... in classic_double_free dfree.c:12   ← malloc 在这里
    #2 ... in main dfree.c:77
```

> 🎯 **Double-free 也是三段式结构**，但和 use-after-free 顺序不同：
>
> ```
> ① 第二次 free（出错）：dfree.c:21
>    ↓
> ② 第一次 free（合法）：dfree.c:18
>    ↓
> ③ 分配点：            dfree.c:12
> ```
>
> 读法：**从 ② 找到第一次 free 的位置 → 从 ① 找到第二次 free 的位置 → 看看中间有什么逻辑导致重复释放**。

### 模式 2：别名 double-free —— `./dfree 2`

```c
char *a = strdup("data");
char *b = a;          // b 和 a 指向同一块内存
free(a);              // 第一次 free
free(b);              // ★ 第二次 free——b 指向的和 a 是同一块
```

报告还是 `attempting double-free`，但调用栈里会有两个不同的变量名（`a` 和 `b`）。这种 bug 在大型项目中极难肉眼发现——指针通过函数参数传递后，你不知道它是否已经被释放了。

### 模式 3：free 栈变量 —— `./dfree 3`

```
==...==ERROR: AddressSanitizer: attempting free on address which was not malloc()-ed: 0x7ffe...
    #0 ... in __interceptor_free
    #1 ... in invalid_free dfree.c:51
```

关键短语：`not malloc()-ed` —— ASan 告诉你这片内存**不是堆上分配的**。

> 编译器（GCC）其实也能在编译时警告这个：
> ```
> warning: 'free' called on unallocated object 'stack_var' [-Wfree-nonheap-object]
> ```
> 编译器的静态分析和 ASan 的运行时检测互补——编译器能抓简单情况，ASan 抓运行时才能确定的场景。

### 模式 4：free 偏移指针 —— `./dfree 4`

```c
char *original = malloc(100);
free(original + 50);   // ★ 释放的不是 malloc 的原始返回值
```

```
==...==ERROR: AddressSanitizer: attempting free on address ... which is not at the start of a heap region
```

关键短语：`not at the start` —— `free` 必须传入 `malloc`/`calloc`/`realloc` **原始返回的指针**，不能是偏移后的。

> 编译器也抓到了这个：
> ```
> warning: 'free' called on pointer 'original' with nonzero offset 50
> ```

---

## Double-free 为什么危险？

```
free(ptr) 之后：

你的视角：        堆管理器的视角：        攻击者的视角：
"ptr 还指向那"    "这块内存空闲了"      "把恶意数据写到这块内存"
                  "下次 malloc 可能分给
                   另一个人"            "你的 double-free 释放了
                                       攻击者的数据 → 可用作 exploit"
```

Double-free 是 CTF 和安全研究中的经典攻击入口（fastbin dup 等攻击手法）。ASan 让它在**第一次发生时**就暴露。

---

## 📝 四种释放类错误速查

| 错误类型 | ASan 关键词 | 特征 |
|---------|------------|------|
| Use-after-free | `heap-use-after-free` | 三段式：错误→释放→分配 |
| Double-free | `attempting double-free` | 三段式：第二次 free→第一次 free→分配 |
| Free 非堆内存 | `not malloc()-ed` | 试图 free 栈/全局/代码段地址 |
| Free 偏移指针 | `not at the start of a heap region` | 指针不是 malloc 原始返回值 |

---

## 🧪 试试看

1. 在模式 1 的两个 `free` 之间加一个 `ptr = NULL;`——第二个 `free(NULL)` 是安全的（C 标准保证），ASan 还报吗？
2. 模式 2 中如果在 `free(a)` 后加 `b = NULL;` 能解决问题吗？为什么不能？
3. 尝试释放一个全局变量——ASan 报什么？
4. 把四个模式的 SUMMARY 行并列对比——只看 SUMMARY，你能立即说出每个是哪种 bug 吗？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| Double-free 报告只说 `__interceptor_free` 没给出行号 | 符号被 strip 了 | 检查 Makefile 是否有 `-g` |
| 两个 `free` 之间距离很远（不同函数/不同文件） | 代码架构问题 | 考虑"谁分配谁释放"的规则，或用引用计数 |
| free(NULL) 是安全的但 ASan 不报 | C 标准明确允许 `free(NULL)` | 不报是正常的——这不是 bug |
| 两个 `.c` 文件都 free 了同一个全局指针 | 维护责任不清 | 统一生命周期管理，或用 `const char *` 避免所有权混淆 |

---

## 下一步

🟡 阶段二全部完成！这四种内存错误覆盖了 C/C++ 项目 90% 以上的内存 bug：

- [实验 03](../03-heap-overflow/)：**堆越界**——最频繁
- [实验 04](../04-stack-overflow/)：**栈越界**——最隐蔽
- [实验 05](../05-use-after-free/)：**释放后使用**——最危险
- [实验 06](../06-double-free/)：**重复释放**——最易忽视

下一步进入 🟠 阶段三：[实验 07：内存泄漏检测](../07-leak-check/)——bug 不是越界，而是"忘了还"。
