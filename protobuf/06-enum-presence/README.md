# 实验 06：enum 与 optional —— 默认值不代表字段出现过

> 🎯 **目标**：理解 proto3 默认值与 optional presence。

~~~proto
enum State { STATE_UNSPECIFIED = 0; STATE_READY = 1; }
message Job { State state = 1; optional int32 priority = 2; }
~~~

枚举第一个值应为 0，作为未知/未指定。普通 proto3 标量未设置时读到默认值；若业务必须区分“未设置”和“设置为 0”，使用 optional 并检查 has_priority()。

下一步：[import 与 package](../07-imports-packages/)。

