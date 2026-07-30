# 实验 12：综合诊断 —— 从现象回到 ELF 根因

> 🎯 **目标**：针对两个独立现场，选择最短的工具链给出证据和修复方案。
>
> ⏱️ 约 35 分钟
>
> 📖 **前置**：完成实验 01–11

## 先准备现场

~~~bash
make
make run-fixed
~~~

本目录有两组线索：

| 文件 | 现象 |
|------|------|
| build/bin/missing_path | 能链接，但启动时找不到 libservice.so |
| build/bin/address_case | 只有一个函数地址，需要还原到源码 |

## 案例一：动态库为什么找不到？

先不要直接修改环境变量，按证据链执行：

~~~bash
file build/bin/missing_path
readelf -d build/bin/missing_path | grep -E 'NEEDED|RPATH|RUNPATH'
ldd build/bin/missing_path
LD_DEBUG=libs ./build/bin/missing_path 2>&1 | less
~~~

你应得出：

1. 程序声明 NEEDED libservice.so；
2. 它没有 RPATH/RUNPATH；
3. 当前默认路径没有 build/lib；
4. 加载器因此失败。

临时验证：

~~~bash
LD_LIBRARY_PATH=build/lib ./build/bin/missing_path
~~~

长期修复应像 fixed_path 那样，在构建/安装策略中提供正确的运行时路径，或把库安装到由 ldconfig 管理的位置。

## 案例二：一个地址来自哪里？

~~~bash
nm -n build/bin/address_case | grep suspicious_operation
addr2line -e build/bin/address_case -f -C 0x地址
objdump -S --disassemble=suspicious_operation build/bin/address_case
~~~

把 nm 输出的实际十六进制地址替换进最后一条 addr2line 命令。你应该能得到 address.c 中 suspicious_operation 的行号，并在 objdump 的源码混排中看到乘以 17 的机器指令。

## 最终检查表

~~~text
格式/架构是否正确？                 file
ELF 声明了什么依赖和路径？          readelf -d
某符号由谁需要、谁定义？            nm
运行时到底加载到什么？              ldd / LD_DEBUG=libs
地址如何还原为文件和行？            addr2line
机器码是否符合源码和优化预期？       objdump -S
~~~

## 挑战

1. 给 missing_path 增加 rpath，不修改 fixed_path。
2. 去掉 address_case 的 -g，记录 addr2line 输出变化。
3. 用 strip --strip-debug 复制一个 address_case_stripped，比较 file、readelf、GDB 的信息差异。
4. 解释为什么 addr2line 实验使用 -no-pie，而动态库实验没有。

## 完成模块

你现在可以把二进制当作可观察、可验证的工程产物，而不只是“编译后生成的黑盒”。下一步可以进入 GDB core dump、ASan 报告、strace 或网络排障模块，把 ELF 证据链用于真实故障。

