# 实验 13：Makefile 工程化 —— 一个完整但可读的小项目

> 🎯 **目标**：把对象复用、自动依赖、测试和构建目录组合起来。

```bash
make
make run
make test
make -j4
make help
```

主程序和测试都复用 `build/calc.o`，避免同一实现重复编译。所有生成物进入 `build/`，源码目录保持干净。

## 常见反模式

- recipe 中递归调用 make 来弥补依赖漏写；
- 把所有源文件每次直接编译链接，失去增量构建；
- Debug/Release 共用同一对象目录；
- 用 `wildcard` 无条件收集所有源文件；
- `clean` 使用范围不明确的删除命令；
- 每个子目录递归 make，顶层看不到完整依赖图。

## 综合练习

1. 增加 `subtract` 函数和测试；
2. 增加 `MODE=release`；
3. 用 `make -n test` 解释每条计划命令；
4. 故意删掉测试对 `calc.o` 的依赖，观察链接错误。

## 下一步

Makefile 阶段完成。下一实验从头解释 [CMake 的工作模型](../14-cmake-model/)。

