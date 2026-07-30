# 实验 11：C++ 符号 —— 名字为什么变成一串编码？

> 🎯 **目标**：理解 name mangling，使用 nm -C 和 c++filt 还原 C++ 函数签名。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 10](../10-strings/)

## 先跑起来

~~~bash
make
nm cpp_symbols | grep decorate
nm -C cpp_symbols | grep decorate
./cpp_symbols
~~~

普通 nm 可能显示：

~~~text
_ZN3lab8decorateERKNSt7__cxx1112basic_string...
~~~

nm -C 会 demangle：

~~~text
lab::decorate(std::__cxx11::basic_string<...> const&, int)
~~~

## 为什么 C++ 需要 mangling？

C 的函数名通常只需要表达名字；C++ 支持命名空间、类成员、函数重载、模板和参数类型。链接器仍要用唯一的符号名区分它们，因此编译器把完整签名编码进名字。这个编码规则与 ABI、编译器和版本有关。

~~~bash
echo '_ZN3lab8decorateERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEi' | c++filt
~~~

## C 与 C++ 混编的常见链接错误

若 C++ 调用 C 库的头文件没有 extern "C"，C++ 会按 mangled 名称查找，而库导出的却是 C 名称，于是得到 undefined reference。正确的 C 头文件通常写：

~~~cpp
#ifdef __cplusplus
extern "C" {
#endif
/* C declarations */
#ifdef __cplusplus
}
#endif
~~~

## 🧪 试试看

1. 给 decorate 增加重载版本，比较两个 mangled 名称。
2. 用 objdump -T cpp_symbols | c++filt 查看动态符号。
3. 在 GDB 中 break lab::decorate，观察调试器如何显示 C++ 名称。

## 下一步

[实验 12：综合诊断](../12-elf-diagnosis/) 用本模块全部工具完成一次完整排障。

