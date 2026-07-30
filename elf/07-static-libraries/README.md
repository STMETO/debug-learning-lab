# 实验 07：静态库 —— archive 不是“自动链接的文件夹”

> 🎯 **目标**：构建 .a，观察 archive 内部对象和符号索引，并理解链接顺序。
>
> ⏱️ 约 25 分钟
>
> 📖 **前置**：[实验 06](../06-addr2line/)

## 先跑起来

~~~bash
make
./build/app
make inspect
~~~

ar t 显示 libhelper.a 中的成员 helper.o；nm -s 显示 archive 索引和每个成员的符号。

## 链接器从左到右工作

正确链接命令：

~~~bash
gcc -o build/app build/main.o build/libhelper.a
~~~

链接器读到 main.o 时发现 checksum 未定义；随后读到 libhelper.a，便从 archive 中抽取提供 checksum 的 helper.o。

故意反过来：

~~~bash
make wrong-order
~~~

预期出现 undefined reference。链接器先读 archive 时还不知道有人需要 checksum，因此不会抽取 helper.o；读到 main.o 时已经错过 archive。

## 🧪 试试看

1. 用 ar x build/libhelper.a 解出 helper.o，再用 nm 查看。
2. 给 helper.c 加第二个不被调用的函数，比较 app 是否包含它的符号。
3. 如果两个静态库相互依赖，尝试使用 -Wl,--start-group 和 --end-group。

## 下一步

[实验 08：动态库](../08-dynamic-libraries/) 将把“链接时抽取”变为“运行时加载”。

