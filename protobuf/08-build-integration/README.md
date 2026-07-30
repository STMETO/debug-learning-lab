# 实验 08：构建集成 —— .proto 是源码依赖

> 🎯 **目标**：把 protoc 作为可重复的构建步骤。

Makefile 的关键关系：

~~~makefile
person.pb.cc person.pb.h: person.proto
	protoc --cpp_out=. $<
app: main.cc person.pb.cc
	g++ -o $@ main.cc person.pb.cc -lprotobuf
~~~

CMake 中可用 find_package(Protobuf REQUIRED) 与 protobuf_generate_cpp，或显式 add_custom_command。无论工具是什么，都要确保生成文件依赖 .proto、生成目录纳入 include 路径、最终目标链接 protobuf runtime。

下一步：[JSON 与 TextFormat](../09-json-text/)。

