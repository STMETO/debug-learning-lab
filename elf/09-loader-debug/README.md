# 实验 09：LD_DEBUG —— 让动态加载器说出它找过哪些地方

> 🎯 **目标**：用 LD_DEBUG=libs 观察库搜索顺序，定位“明明有 .so 却找不到”的问题。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 08](../08-dynamic-libraries/)

## 先跑起来

~~~bash
make
./build/bin/with_rpath
LD_DEBUG=libs ./build/bin/no_rpath 2>&1 | less
~~~

no_rpath 预期失败。LD_DEBUG 输出会列出加载器针对 libplugin.so 尝试的每一个目录；它比“cannot open shared object file”多出最重要的信息：**加载器实际按什么顺序搜索**。

## 常用 LD_DEBUG 类别

| 值 | 作用 |
|----|------|
| libs | 搜索和加载共享库 |
| bindings | 符号绑定 |
| reloc | 重定位处理 |
| files | 打开的对象文件 |
| all | 所有类别，输出通常非常大 |

可把日志保存下来：

~~~bash
LD_DEBUG=libs LD_DEBUG_OUTPUT=loader ./build/bin/no_rpath
~~~

加载器会创建 loader.<pid> 文件。

## 正确的修复顺序

1. 用 readelf -d 确认程序真正 NEEDED 什么库。
2. 用 ldd 确认当前环境解析结果。
3. 用 LD_DEBUG=libs 看搜索路径。
4. 选择正确修复：安装到系统库目录并运行 ldconfig、在构建时写 RUNPATH，或为开发环境临时设置 LD_LIBRARY_PATH。

不要把 LD_LIBRARY_PATH 当作永久部署方案；它是进程全局变量，容易污染其他依赖解析。

## 🧪 试试看

1. 用 LD_LIBRARY_PATH=build/lib 运行 no_rpath。
2. 对 with_rpath 使用 LD_DEBUG=libs，找出 RUNPATH 何时被使用。
3. 将 libplugin.so 改名，观察输出中的每次尝试。

## 下一步

[实验 10：strings](../10-strings/) 转向没有源码或没有符号时仍可使用的二进制线索。

