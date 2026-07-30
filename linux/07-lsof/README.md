# 实验 07：lsof —— 一个进程到底打开了什么？

> 🎯 **目标**：用 lsof 将 PID、文件描述符、文件、socket 和库关联起来。

~~~bash
make
./lsof_demo &
pid=$!
lsof -p "$pid"
lsof -p "$pid" -a -d 0,1,2,3,4
lsof "$PWD/held.log"
kill "$pid"
~~~

lsof 的 TYPE 可能是 REG（普通文件）、DIR、CHR、FIFO 或 IPv4/IPv6 socket。排查“文件删了磁盘不释放”“谁占用了日志”“进程是否加载了预期 .so”时，lsof 是比 ls 更接近进程现场的工具。

注意：查看其他用户进程或 socket 归属可能需要 sudo。

下一步：[ss](../08-ss/)。

