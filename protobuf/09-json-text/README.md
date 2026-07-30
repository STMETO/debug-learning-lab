# 实验 09：JSON 与 TextFormat —— 可读格式用于边界与诊断

> 🎯 **目标**：理解 JSON 不是 Protobuf 二进制格式。

C++ 可使用 google::protobuf::util::MessageToJsonString 和 JsonStringToMessage；TextFormat 更适合日志、测试数据和人工检查。JSON 的字段名、整数精度、unknown field 策略都需要明确约定，不能假设与二进制完全等价。

下一步：[schema 演进](../10-schema-evolution/)。

