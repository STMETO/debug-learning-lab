# 实验 10：strings —— 没有源码时先找可读线索

> 🎯 **目标**：从二进制提取错误文本、域名、路径和配置关键字，并知道它的局限。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[实验 09](../09-loader-debug/)

## 先跑起来

~~~bash
make
strings config_demo | grep -E 'api\.example|FEATURE|connecting'
./config_demo
~~~

strings 默认提取至少 4 个连续可打印字符。它不理解 C、ELF 或网络协议，只是扫描字节中的可读片段；正因如此，它适合在没有源码、没有符号、甚至不知道程序用途时快速建立假设。

## 实战用法

~~~bash
strings -n 8 /path/to/program | less
strings -a config_demo | grep -iE 'error|config|http|path'
strings -t x config_demo | grep FEATURE
~~~

-n 调整最短长度；-a 扫描整个文件；-t x 显示字符串在文件中的十六进制偏移。

## 不要把它当作安全审计结论

strings 找到明文 token、路径或域名值得警惕，但找不到不代表不存在：内容可能被压缩、加密、拆分、运行时生成，或根本不在该 ELF 中。它是线索收集工具，后续仍需要 readelf、strace、网络抓包或源码验证。

## 🧪 试试看

1. 修改 endpoint 后重编译，确认旧字符串不再出现。
2. 对 /bin/ls 运行 strings，能推断哪些能力？哪些不能？
3. 用 readelf -p .rodata config_demo 对比 strings 的结果。

## 下一步

[实验 11：C++ 符号](../11-cpp-symbols/) 解释为什么 nm 有时打印出难读的函数名。

