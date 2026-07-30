# 实验 01：网络现场地图 —— 先问数据包要往哪里走

> 🎯 **目标**：建立接口、地址、路由、DNS、端口四层现场地图。

~~~bash
ip -br addr
ip route
ip route get 1.1.1.1
cat /etc/resolv.conf
ss -ltn
~~~

| 信息 | 先回答的问题 |
|------|--------------|
| ip addr | 本机有哪些接口和地址？ |
| ip route | 默认网关和目标路径是什么？ |
| resolv.conf | DNS 请求会发给谁？ |
| ss -ltn | 本机到底监听了哪些 TCP 端口？ |

排障不要一开始抓包。若没有路由、目标地址错误或服务根本没监听，tcpdump 只会制造更多噪声。

下一步：[ip 命令](../02-ip-basics/)。

