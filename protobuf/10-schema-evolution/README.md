# 实验 10：schema 演进 —— 编号永远比名字重要

> 🎯 **目标**：安全添加、删除和废弃字段。

安全：新增使用过的新编号；保留旧字段编号；删除字段后 reserved 其编号和名称。

~~~proto
message Person {
  reserved 4, 6 to 9;
  reserved "legacy_id";
  string name = 1;
}
~~~

危险：改变已发布字段编号、复用删除字段编号、随意改变 wire type。新读者通常应忽略旧字段，旧读者通常应忽略新字段；这是渐进发布的基础。

下一步：[二进制检查](../11-wire-inspection/)。

