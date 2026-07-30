# 实验 08：动态库 —— 链接成功后，加载器还要做一次解析

> 🎯 **目标**：查看 DT_NEEDED、RUNPATH 和 ldd 输出，区分 -L、-l 与 rpath。
>
> ⏱️ 30 分钟
>
> 📖 **前置**：[实验 07](../07-static-libraries/)

## 先跑起来

~~~bash
make
make inspect
make run
~~~

readelf -d 显示 ELF 的 dynamic section。关注：

~~~text
NEEDED   运行时需要哪个共享库，例如 libgreeting.so
RUNPATH  运行时额外搜索的目录
~~~

-Lbuild/lib 只在**链接时**帮助 ld 找到库；-lgreeting 选择 libgreeting.so；-Wl,-rpath,... 把运行时搜索路径写入最终 ELF。

## ldd 与 readelf 的分工

~~~bash
ldd build/bin/app
readelf -d build/bin/app
~~~

ldd 展示当前环境下实际会解析到哪些库；readelf 只读文件里的声明。排障时先用 readelf 确认二进制要求什么，再用 ldd 观察当前机器给了什么。

> 对来源不可信的可执行文件，不建议直接运行 ldd；优先使用 readelf -d。

## RUNPATH 和 RPATH

现代链接器通常生成 RUNPATH。它与传统 RPATH 在间接依赖和 LD_LIBRARY_PATH 优先级上的规则不同；日常定位先记住：两者都是 ELF 内记录的运行时搜索提示，readelf -d 可以确认它们是否存在。

## 🧪 试试看

1. 去掉 rpath 后重建并运行，观察加载器错误。
2. 用 LD_LIBRARY_PATH=build/lib 临时修复无 rpath 的程序。
3. 用 nm -D build/lib/libgreeting.so 确认 greeting 是否导出。

## 下一步

[实验 09：LD_DEBUG](../09-loader-debug/) 让动态加载器把搜索过程完整说出来。

