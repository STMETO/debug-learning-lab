# 实验 04：反序列化 —— 解析失败必须是正常分支

> 🎯 **目标**：读取 person.bin，用 ParseFromString 还原消息。

先在实验 03 生成 person.bin，再复制到本目录：

~~~bash
cp ../03-serialize/person.bin .
make && ./parse_demo
printf 'broken' > person.bin
./parse_demo; echo $?
~~~

ParseFromString 失败可能意味着截断、错误协议、长度边界处理错误或数据损坏；不要在失败后继续读取字段。

下一步：[复杂字段](../05-collections-oneof/)。

