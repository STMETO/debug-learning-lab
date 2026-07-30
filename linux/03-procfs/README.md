# 实验 03：/proc —— Linux 进程现场的文件系统视图

> 🎯 **目标**：从 /proc/PID 读取命令行、环境、文件描述符、内存映射和状态。

~~~bash
make
APP_MODE=debug ./proc_demo &
pid=$!
cat /proc/$pid/status | sed -n '1,12p'
tr '\0' ' ' < /proc/$pid/cmdline; echo
tr '\0' '\n' < /proc/$pid/environ | grep APP_MODE
ls -l /proc/$pid/fd
head -n 5 /proc/$pid/maps
~~~

| 文件 | 回答的问题 |
|------|------------|
| status | 状态、线程数、VmRSS、UID |
| cmdline | 它是以什么参数启动的？ |
| environ | 它继承了哪些环境变量？ |
| fd/ | 打开了什么文件、管道、socket？ |
| maps | 加载了哪些 ELF、堆栈和匿名映射？ |

环境变量和 cmdline 都以 NUL 分隔，所以要用 tr 显示。结束后执行 kill "$pid"。

下一步：[查找文件](../04-searching/)。

