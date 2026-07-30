# 实验 05：repeated、map、oneof

> 🎯 **目标**：表达列表、键值和互斥消息体。

~~~proto
message Event {
  repeated string tags = 1;
  map<string, string> labels = 2;
  oneof payload { string text = 3; bytes binary = 4; }
}
~~~

repeated 对应列表；map 是生成的 entry message；oneof 同时只能保留一个字段，设置 binary 会清除 text。oneof 适合互斥语义，不适合“可选字段的集合”。

下一步：[枚举与存在性](../06-enum-presence/)。

