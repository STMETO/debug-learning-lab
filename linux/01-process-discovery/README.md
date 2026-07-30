# 实验 01：定位进程 —— 先确认你在观察谁

> 🎯 **目标**：用 ps、pgrep、pidof 找到进程，建立 PID 是后续所有现场命令入口的认知。

~~~bash
make
./process_demo > demo.log 2>&1 &
pid=$!
echo "$pid"
ps -fp "$pid"
pgrep -a process_demo
pidof process_demo
~~~

| 命令 | 最适合的问题 |
|------|--------------|
| ps -fp PID | 该 PID 的用户、状态、启动命令是什么？ |
| pgrep -a 名称 | 同名进程有哪些，完整命令行是什么？ |
| pidof 名称 | 某服务的 PID 列表是什么？ |
| ps -ef --forest | 谁启动了谁？进程树如何？ |

结束现场：

~~~bash
kill "$pid"
wait "$pid" 2>/dev/null || true
~~~

不要对名称模糊匹配后直接 kill。先用 ps -fp 确认 PID、用户和命令行，再进行后续操作。

下一步：[资源快照](../02-resource-snapshot/)。

