# Protobuf：从 .proto 到二进制诊断

本模块使用 C++ 和 proto3，学习如何定义消息、用 protoc 生成代码、序列化/反序列化，以及如何安全演进协议。

| # | 实验 | 核心内容 |
|---|------|----------|
| [01](01-hello-proto/) | 第一份 proto | syntax、package、field number |
| [02](02-generate-cpp/) | 生成 C++ | protoc、.pb.h、.pb.cc |
| [03](03-serialize/) | 序列化 | SerializeToString |
| [04](04-deserialize/) | 反序列化 | ParseFromString、错误处理 |
| [05](05-collections-oneof/) | 复杂字段 | repeated、map、oneof |
| [06](06-enum-presence/) | 枚举与存在性 | enum、optional |
| [07](07-imports-packages/) | 模块组织 | import、package |
| [08](08-build-integration/) | 构建集成 | Makefile/CMake 思路 |
| [09](09-json-text/) | 可读格式 | JSON、TextFormat |
| [10](10-schema-evolution/) | 协议演进 | reserved、兼容性 |
| [11](11-wire-inspection/) | 二进制检查 | protoc --decode、--decode_raw |
| [12](12-protobuf-diagnosis/) | 综合诊断 | 解析失败与版本问题 |

~~~bash
sudo apt install protobuf-compiler libprotobuf-dev g++ make
protoc --version
~~~

