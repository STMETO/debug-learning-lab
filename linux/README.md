# Linux 现场排障与调试必备命令

本模块面向 C/C++ 与 Linux 工程师，学习在没有 IDE、只有终端和现场进程时如何收集证据。每个实验提供可控的进程、文件、日志或端口，避免把命令背成零散清单。

## 学习路线

| # | 实验 | 目标 |
|---|------|------|
| [01](01-process-discovery/) | 定位进程 | ps、pgrep、pidof 与进程树 |
| [02](02-resource-snapshot/) | 资源快照 | uptime、free、top、ps |
| [03](03-procfs/) | /proc 现场 | status、cmdline、environ、fd、maps |
| [04](04-searching/) | 查找文件 | find、which、whereis、readlink |
| [05](05-text-pipelines/) | 提取日志证据 | grep、awk、sed、sort、uniq |
| [06](06-pipes-redirection/) | 组合命令 | 管道、重定向、tee、xargs、timeout |
| [07](07-lsof/) | 打开对象 | lsof 与文件描述符 |
| [08](08-ss/) | 网络现场 | ss 与监听/连接状态 |
| [09](09-environment-limits/) | 运行环境 | env、ulimit、timeout |
| [10](10-watch-time/) | 持续观察 | watch、time 与周期性状态 |
| [11](11-system-logs/) | 系统日志 | journalctl、dmesg 与应用日志 |
| [12](12-incident-response/) | 综合实战 | 服务启动失败的证据链 |

## 环境

~~~bash
sudo apt install procps psmisc lsof iproute2 gawk sed findutils coreutils util-linux
~~~

部分命令（ss 的进程归属、dmesg、journalctl 全量日志）可能需要 sudo，容器内若没有 systemd，journalctl 的示例会受限。

从 [实验 01](01-process-discovery/) 开始。

