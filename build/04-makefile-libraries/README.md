# 实验 04：静态库与动态库 —— 编译通过，不代表运行时能找到库

> 🎯 **目标**：分别构建静态库和动态库，理解链接期、运行期与 rpath 的区别。
>
> ⏱️ 约 30 分钟
>
> 📖 **前置**：[实验 03](../03-makefile-dependencies/)

## 先跑起来

~~~bash
make
make run-static
make run-shared
~~~

两个程序输出相同，但里面装的东西不同：

~~~text
src/mathlib.c → libmathlab.a  → app_static
              → libmathlab.so → app_shared
~~~

## 静态库：在链接期复制代码

~~~bash
ar t build/lib/libmathlab.a
file build/bin/app_static
~~~

ar rcs 把 mathlib.o 打包进 .a。链接 app_static 时，链接器从 .a 中抽取所需机器码并写入最终可执行文件；运行时不再需要 libmathlab.a。

## 动态库：运行时再加载

~~~bash
ldd build/bin/app_shared
~~~

你应该能看到 libmathlab.so 指向本实验的 build/lib。Makefile 中这段是关键：

~~~makefile
-Wl,-rpath,'$$ORIGIN/../lib'
~~~

-Wl, 把参数交给链接器；$ORIGIN 表示“当前可执行文件所在目录”。app_shared 在 build/bin，因此 ../lib 正好是 build/lib。

试着暂时去掉 rpath，重新 make clean && make shared，再运行 app_shared。典型错误是：

~~~text
error while loading shared libraries: libmathlab.so: cannot open shared object file
~~~

这不是编译错误，也不是链接错误，而是**程序启动时**动态加载器找不到库。临时修复方式是：

~~~bash
LD_LIBRARY_PATH=build/lib ./build/bin/app_shared
~~~

生产项目更应使用安装路径、rpath 或系统的 ldconfig 配置，不要长期依赖全局 LD_LIBRARY_PATH。

## 为什么动态库对象加 -fPIC？

共享库可能被加载到进程的不同虚拟地址。-fPIC 生成位置无关代码，使同一份 .so 可以安全地映射到不同进程和位置。

## 🧪 试试看

1. 比较 ls -lh build/bin/app_static build/bin/app_shared 的大小。
2. 用 nm -D build/lib/libmathlab.so 查看导出的 square 和 cube。
3. 把 -lmathlab 放到 main.o 前面，观察某些链接器/库类型下为什么顺序会影响结果。

## 下一步

下一实验把构建选项变成明确的产品能力：[实验 05：Debug、Release 与 ASan](../05-makefile-debug/)。

