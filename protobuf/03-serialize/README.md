# 实验 03：序列化 —— 对象变成字节，不是文本

> 🎯 **目标**：填充 Person 并写出 person.bin。

~~~bash
make
./serialize_demo
ls -lh person.bin
xxd person.bin
~~~

SerializeToString 返回 bool；生产代码必须检查失败。Protobuf 二进制不可直接当 JSON 阅读，字段通过编号和 wire type 编码，体积小且适合网络/存储。

下一步：[反序列化](../04-deserialize/)。

