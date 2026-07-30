# 实验 11：系统日志 —— 时间线比孤立报错更重要

> 🎯 **目标**：从应用日志、journalctl 和 dmesg 中按时间建立故障证据链。

~~~bash
grep -E 'ERROR|WARN' service.log
journalctl -b -p warning..alert --no-pager
dmesg -T | tail -n 50
~~~

| 来源 | 最适合发现 |
|------|------------|
| 应用日志 | 业务错误、配置、请求 ID |
| journalctl | systemd 服务退出、权限、启动顺序 |
| dmesg | OOM killer、驱动、文件系统、内核网络信息 |

容器或 WSL 环境可能没有完整 systemd journal；这是环境限制，不代表命令错误。真实服务排障应记录时间范围、时区、主机名和 PID，再把应用日志与系统日志对齐。

下一步：[综合实战](../12-incident-response/)。

