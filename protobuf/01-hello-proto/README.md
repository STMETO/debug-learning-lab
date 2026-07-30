# 实验 01：第一份 proto —— 字段编号是协议的一部分

> 🎯 **目标**：写出第一份 proto3 消息。

[person.proto](person.proto)：

~~~proto
syntax = "proto3";
package lab.people;
message Person {
  string name = 1;
  int32 age = 2;
  string email = 3;
}
~~~

字段名服务于代码可读性；字段编号服务于二进制编码与兼容性。编号一旦发布不能随意更改或复用。package 防止不同团队的消息名称冲突。

下一步：[生成 C++](../02-generate-cpp/)。

