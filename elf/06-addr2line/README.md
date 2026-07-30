# 实验 06：addr2line —— 把地址翻译回源码位置

> 🎯 **目标**：从 nm 获得函数地址，再用 addr2line 得到文件、行号和调用函数。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 05](../05-debug-symbols/)

## 先跑起来

~~~bash
make
nm -n address_demo | grep -E ' divide$| calculate$| main$'
~~~

选择 divide 的地址（示例为 0x401126），再执行：

~~~bash
addr2line -e address_demo -f -C 0x401126
~~~

输出类似：

~~~text
divide
.../crash.c:6
~~~

-e 指定 ELF；-f 显示函数名；-C 对 C++ 名称进行 demangle。

## 为什么 Makefile 用了 -no-pie？

PIE 程序运行时受 ASLR 影响，日志中的绝对地址通常要先减去加载基址。为了让第一次实验专注于“地址 → 行号”，本例编译为非 PIE，nm 显示的地址能直接传给 addr2line。

真实线上定位则是：

~~~text
运行时地址 - 模块加载基址 = ELF 内偏移
ELF 内偏移 → addr2line -e 对应二进制
~~~

GDB 的 info proc mappings、/proc/PID/maps 或崩溃日志可帮助找到加载基址。

## 批量符号化

~~~bash
printf '0x401126\n0x40113f\n' | addr2line -e address_demo -f -C
~~~

这正是很多 core dump、性能采样和 crash reporter 后端的基本工作。

## 🧪 试试看

1. 去掉 -no-pie，比较 file 和 nm 输出。
2. 删除 -g 后重新构建，addr2line 的行号如何变化？
3. 用 gdb 打开 address_demo，验证 break divide 的地址。

## 下一步

[实验 07：静态库](../07-static-libraries/) 回到链接阶段，分析符号怎样从 archive 中被抽取。

