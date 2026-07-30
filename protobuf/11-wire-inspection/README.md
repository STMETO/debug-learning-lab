# 实验 11：二进制检查 —— 不要猜测 payload 是什么

> 🎯 **目标**：用 protoc 检查已知 schema 的二进制消息。

在实验 03 生成 person.bin 后：

~~~bash
protoc --decode=lab.people.Person person.proto < person.bin
protoc --decode_raw < person.bin
~~~

--decode 需要正确 schema；--decode_raw 只能给出未知字段编号/wire type 的线索，不能替代协议定义。生产抓包还要先处理长度前缀、压缩、TLS 和 framing。

下一步：[综合诊断](../12-protobuf-diagnosis/)。

