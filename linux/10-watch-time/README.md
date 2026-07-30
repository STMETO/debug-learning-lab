# 实验 10：watch 与 time —— 观察变化，而不是只看一个瞬间

> 🎯 **目标**：用 watch 周期性刷新状态，用 time 得到命令的基本耗时。

~~~bash
make
./counter &
watch -n 0.5 cat status.txt
~~~

按 Ctrl-C 退出 watch。另开终端测量：

~~~bash
/usr/bin/time -v ./counter
time ./counter
~~~

shell 内置 time 适合快速看 real/user/sys；/usr/bin/time -v 额外提供最大 RSS、上下文切换等。一次慢不代表稳定慢，watch 的价值是让你看到计数、连接数、文件大小等是否持续变化。

下一步：[系统日志](../11-system-logs/)。

