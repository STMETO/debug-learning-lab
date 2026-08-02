# 实验 10：构建目录与并行 —— `|` 为什么重要？

> 🎯 **目标**：把产物放入 `build/`，理解 order-only prerequisite 和 `make -j`。

```makefile
$(BUILD)/%.o: src/%.c | $(BUILD)
```

竖线右边是 order-only prerequisite：

- `build/` 必须先存在；
- 但目录时间戳变化不应导致所有对象重编译。

普通依赖写在竖线左边，变化会让目标过期。

```bash
make -n
make -j4
make --debug=b
```

- `-n` 查看计划；
- `-j4` 最多并行执行四个互不依赖任务；
- `--debug=b` 显示基本判断过程。

并行构建会暴露漏写依赖。不要靠“某条命令碰巧先执行”维持正确性。

## 🧪 试试看

1. 把 `| $(BUILD)` 改成普通依赖，观察目录时间变化的影响。
2. `make clean && make -j8`。
3. 故意删除对象规则的目录依赖，观察首次并行构建。

下一步：[构建模式](../11-build-modes/)。

