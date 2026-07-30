# 实验 09：环境与限制 —— 同一个二进制为何行为不同？

> 🎯 **目标**：确认环境变量、ulimit 和超时保护如何改变进程现场。

~~~bash
make
./env_demo
APP_MODE=debug ./env_demo
ulimit -n
ulimit -n 64
./env_demo
timeout 1 sh -c 'sleep 5' || echo "timeout"
~~~

环境变量是进程启动时的输入。排查时用：

~~~bash
env | sort
tr '\0' '\n' < /proc/PID/environ
ulimit -a
~~~

ulimit -n 过低会导致服务在高并发下报 Too many open files。不要只在当前 shell 临时调大后宣布修复；还要检查 systemd 的 LimitNOFILE、容器限制和启动脚本。

下一步：[持续观察](../10-watch-time/)。

