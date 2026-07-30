# 实验 02：资源快照 —— 高负载先看全局，再看进程

> 🎯 **目标**：用 uptime、free、ps、top 判断 CPU、内存和负载属于系统级还是单进程问题。

~~~bash
make
./resource_demo > demo.log 2>&1 &
pid=$!
uptime
free -h
ps -p "$pid" -o pid,stat,%cpu,%mem,rss,etime,cmd
top -p "$pid"
~~~

观察顺序：

1. uptime 的 load average 是否异常；
2. free -h 的 available，而不是只看 free；
3. ps 的 RSS、%CPU、STAT；
4. top 中是否有多个进程竞争 CPU。

本例故意占用一个 CPU。按 q 退出 top，随后 kill "$pid" 清理。

下一步：[查看 /proc](../03-procfs/)。

