# 实验 12：静态库与动态库 —— 链接期和运行期

> 🎯 **目标**：构建 `.a`、`.so`，理解 `ar`、`-fPIC`、`-L/-l` 和 rpath。

```text
mathlib.o → libmathlab.a  → app_static
          → libmathlab.so → app_shared
```

- `ar rcs` 把对象打包为静态库；
- `-shared` 生成共享库；
- 共享库对象通常使用 `-fPIC`；
- `-Lbuild -lmathlab` 在链接时查找 `libmathlab.so`；
- rpath 告诉动态加载器运行时去哪里找库。

这里有两个不要混淆的搜索阶段：

- 链接阶段：链接器根据 `-Lbuild -lmathlab` 找到库；
- 运行阶段：动态加载器根据 RUNPATH、`LD_LIBRARY_PATH` 和系统目录找到 `.so`。

```bash
make run
ar t build/libmathlab.a
ldd build/app_shared
readelf -d build/app_shared | grep -E 'NEEDED|RPATH|RUNPATH'
```

静态库链接顺序通常要求“使用库的对象在前，库在后”。动态程序链接成功但启动失败，多半是运行时搜索路径问题，不是 `-L` 问题。

Makefile 中写 `$$ORIGIN` 是因为 Make 会先处理 `$`；传给 shell 和链接器后才变成 `$ORIGIN`。它表示可执行文件所在目录，使示例不依赖绝对路径。

## 🧪 试试看

1. 用 `nm` 比较两个库导出的符号。
2. 临时去掉 rpath，观察共享版程序的启动错误。
3. 调换静态链接命令中对象和库的顺序，观察是否出现未定义引用。

下一步：[Makefile 工程化](../13-make-engineering/)。
