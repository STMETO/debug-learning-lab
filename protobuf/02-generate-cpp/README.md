# 实验 02：protoc —— 从 schema 生成 C++ 类型

> 🎯 **目标**：生成并阅读 person.pb.h、person.pb.cc。

~~~bash
make
ls person.pb.h person.pb.cc
grep -n 'class Person' person.pb.h
~~~

protoc 把 schema 生成强类型 API；生成文件是构建产物，不应手工修改。Makefile 把 .proto 声明为 .pb.h/.pb.cc 的依赖，因此 schema 改变会自动重新生成。

下一步：[序列化](../03-serialize/)。

