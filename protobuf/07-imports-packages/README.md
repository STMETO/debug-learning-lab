# 实验 07：import 与 package —— schema 也需要模块边界

> 🎯 **目标**：拆分公共类型和业务消息。

~~~proto
// common.proto
syntax = "proto3"; package lab.common;
message RequestId { string value = 1; }

// order.proto
import "common.proto";
message Order { lab.common.RequestId request_id = 1; }
~~~

生成时指定 import 根：protoc -I . --cpp_out=. order.proto common.proto。package 是 Protobuf 名称空间，不会自动等于目录结构。

下一步：[构建集成](../08-build-integration/)。

