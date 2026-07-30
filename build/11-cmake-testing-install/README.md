# 实验 11：CTest 与 install —— 构建不是到生成二进制就结束

> 🎯 **目标**：把单元测试注册给 CTest，并把库、头文件、程序安装到一个可控前缀。
>
> ⏱️ 约 30 分钟
>
> 📖 **前置**：[实验 10](../10-cmake-configurations/)

## 先跑起来

~~~bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$PWD/stage"
find stage -type f
./stage/bin/word_count
~~~

测试输出应包含：

~~~text
100% tests passed, 0 tests failed out of 1
~~~

## CTest 不等于测试框架

CTest 负责发现、调度和报告测试；本实验的测试本身只是一个使用 assert 的普通 C 程序：

~~~cmake
add_test(NAME textutil.unit COMMAND test_textutil)
~~~

大型项目可以把 GoogleTest、Catch2 等框架产生的测试也注册到 CTest。CI 只需执行 ctest，就能得到统一结果。

## install 是可验证的交付边界

~~~cmake
install(TARGETS textutil word_count ...)
install(FILES include/textutil.h DESTINATION ...)
~~~

使用 --prefix "$PWD/stage" 安装到临时目录，而不是写入 /usr/local。这样你可以检查交付物是否完整、路径是否合理，也不会需要 root 权限。

## BUILD_INTERFACE 与 INSTALL_INTERFACE

库在构建树中引用源码的 include/，安装后则应引用 prefix/include。两者不能混淆：

~~~cmake
$<BUILD_INTERFACE:.../include>
$<INSTALL_INTERFACE:include>
~~~

这是一种 generator expression：它在不同上下文生成不同的使用要求。若把源码的绝对路径安装进库的接口，别人使用安装后的库就会携带你机器上的无效路径。

## 🧪 试试看

1. 故意把 assert 的期望值改错，运行 ctest --output-on-failure。
2. 用 cmake --build build --target test 运行测试目标。
3. 删除 install(FILES ...)，重新安装，思考库消费者会缺少什么。

## 下一步

下一实验学习 [CMake Presets](../12-cmake-presets/)，把一长串配置参数变成版本可控的名字。

