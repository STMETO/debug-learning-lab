# 实验 12：综合诊断 —— 解析失败先检查边界与版本

> 🎯 **目标**：建立 Protobuf 解析失败的排查顺序。

1. 确认拿到的是一条完整消息，而不是 TCP 流中的半包/粘包；
2. 确认 framing（常见是 varint 或固定长度前缀）已剥离；
3. 用 protoc --decode 与正确 schema 验证 payload；
4. 检查 producer/consumer 的字段编号、wire type、oneof 和 optional 演进；
5. 记录 unknown fields、版本、消息大小和解析错误，不要只打印“decode failed”。

结合 network、strace 和 ELF 模块：网络工具确认字节是否到达，strace 确认读写结果，Protobuf 工具确认字节能否按 schema 解释。

