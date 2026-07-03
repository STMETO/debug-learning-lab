# 实验 09：抑制文件 —— 聚焦新 bug，屏蔽已知问题

> 🎯 **目标**：学会用抑制文件（suppressions）临时屏蔽已知的 ASan 错误，在大型项目中渐进式接入 ASan。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：任意一个 ASan 实验

## 真实场景

你的项目有 100 万行代码，第一次接入 ASan：

```
$ ./my_huge_app
=================================================================
==1==ERROR: AddressSanitizer: heap-buffer-overflow ...
==1==ERROR: AddressSanitizer: heap-use-after-free ...
==1==ERROR: AddressSanitizer: stack-buffer-overflow ...
==1==ERROR: ...
...（几百个错误）
```

你不可能一次性修完所有这些 bug。你需要一个"临时白名单"——抑制那些已知但暂时不修的错误，聚焦新引入的 bug。

---

## 先看示例程序

[suppress.c](suppress.c) 有两个 bug：

- `known_overflow()`——一个已知的堆溢出，暂时不想修
- `new_uaf_bug()`——一个全新的 use-after-free，需要被 ASan 报出来

```bash
make
./suppress 3    # 两个 bug 都触发 → ASan 报第一个
```

---

## 实验开始

### 第一步：认识抑制文件

[known_bugs.supp](known_bugs.supp) 的内容：

```
# 抑制 known_overflow 函数中的所有 ASan 错误
known_overflow
```

每一行是一个抑制规则，格式为：

```
[错误类型:]匹配模式
```

| 格式 | 示例 | 匹配范围 |
|------|------|---------|
| `函数名` | `known_overflow` | 调用栈中出现这个函数名的所有错误 |
| `fun:函数名` | `fun:leaky_func` | 同上（显式写法） |
| `*:函数名` | `*:known_overflow` | 匹配所有错误类型的该函数 |
| `错误类型:函数名` | `heap-buffer-overflow:old_bug` | 只抑制特定类型 |
| `文件名` | `src/legacy/` | 匹配来自该文件的所有错误 |

### 第二步：不带抑制文件运行

```bash
./suppress 3
```

输出：

```
=== 抑制文件实验 ===
运行两个 bug...
  (不带抑制文件时两个都会报)

=================================================================
==...==ERROR: AddressSanitizer: heap-buffer-overflow ...
    #0 ... in known_overflow suppress.c:15
    ...
```

已知的 `known_overflow` 先触发了——还没跑到新的 `new_uaf_bug`。

### 第三步：带抑制文件运行

```bash
ASAN_OPTIONS=suppressions=known_bugs.supp ./suppress 3
```

输出：

```
=== 抑制文件实验 ===
运行两个 bug...

=================================================================
==...==ERROR: AddressSanitizer: heap-use-after-free ...
    #0 ... in new_uaf_bug suppress.c:25
    ...
```

**`known_overflow` 的错误被抑制了**，ASan 直接跳过它，继续运行到 `new_uaf_bug` 才停下来。

> 抑制不是修复——被抑制的 bug 依然存在，只是 ASan 不再为它报错。把它看作"已知缺陷"清单。

### 第四步：抑制文件的渐进策略

实际项目中的推荐工作流：

```
1. 第一次接入 ASan
    ↓
2. 运行 → 收集所有报错 → 分类
    ↓
3. 把"第三方库/历史遗留/暂时不修"的写入 .supp 文件
    ↓
4. CI 中设置: ASAN_OPTIONS=suppressions=known.supp
    ↓
5. 每次修好一批 bug，从 .supp 中删除对应的行
    ↓
6. 目标：.supp 文件最终变空
```

---

## 📝 抑制规则语法参考

```
# 按函数名抑制（匹配调用栈中的任意函数名）
old_buggy_function

# 按源文件路径抑制
src/third_party/json_parser.c

# 按模块/共享库抑制
# lib_legacy.so

# 按错误类型 + 函数名（精确抑制）
heap-buffer-overflow:flaky_allocation
heap-use-after-free:dangling_ref

# 通配所有类型（函数名匹配时抑制所有 ASan 错误）
*:known_issue_function
```

---

## 🧪 试试看

1. 修改 `known_bugs.supp`，把抑制范围精确到 `heap-buffer-overflow:known_overflow`——然后触发 `known_overflow` 里的其他类型错误（如果有的话）还会报吗？
2. 如果抑制规则写错了函数名（`known_overflowz`），ASan 会怎么提示？
3. 抑制文件能屏蔽 LSAN 泄漏报告吗？写一个泄漏函数然后试一下
4. 一行写多个抑制规则会怎样？一个文件里能有多少条规则？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `failed to parse suppressions` | 抑制文件格式不对 | 检查每行格式，注释用 `#` |
| 抑制不生效 | 函数名写错或调用栈中函数名被内联/mangled | 在 ASan 报告 `#0`~`#3` 中找函数名，不要用 C++ mangled name |
| 抑制了不该抑制的 bug | 规则太宽（比如写了 `*:main`） | 把规则精确到具体的错误函数 |

---

## 下一步

掌握了抑制文件，你可以在任何规模的项目中引入 ASan。最后一个实验把 ASan 和 GDB 结合起来：[实验 10：ASan + GDB 联动](../10-asan-gdb/)——在 ASan 检测到错误时用 GDB 抓住现场，在程序"死掉"之前深入检查内存状态。
