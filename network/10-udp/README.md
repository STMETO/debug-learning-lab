# 实验 10：UDP —— 没有握手，也没有“连接已建立”

> 🎯 **目标**：用 nc 和 tcpdump 观察 UDP 发送，不把 TCP 的状态机套到 UDP 上。

终端 A：

~~~bash
nc -u -l 127.0.0.1 18083
~~~

终端 B：

~~~bash
sudo tcpdump -ni lo -c 5 'udp port 18083'
~~~

终端 C：

~~~bash
printf 'datagram\n' | nc -u -w1 127.0.0.1 18083
~~~

UDP 没有 SYN、ACK、ESTABLISHED 或可靠重传。tcpdump 看见发送包，不等于对端应用真的收到；需要应用协议的确认、服务端日志或计数器共同验证。

下一步：[pcap 与 Wireshark](../11-pcap-wireshark/)。

