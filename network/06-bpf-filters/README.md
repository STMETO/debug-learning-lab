# 实验 06：BPF 过滤 —— 只抓与你的问题有关的包

> 🎯 **目标**：掌握 tcpdump 最常用的 host、port、net 和协议过滤表达式。

~~~bash
sudo tcpdump -ni lo 'tcp port 18080'
sudo tcpdump -ni lo 'host 127.0.0.1 and tcp'
sudo tcpdump -ni any 'udp port 53'
sudo tcpdump -ni lo 'tcp[tcpflags] & (tcp-syn|tcp-fin|tcp-rst) != 0'
~~~

过滤表达式在 shell 中建议用单引号，避免括号、! 等被 shell 解释。先从协议 + host + port 三件套开始；只有在包量很大时再使用 TCP flags 等复杂条件。

🧪 试试看：将 and 改为 or，比较输出数量和噪声。

下一步：[TCP 生命周期](../07-tcp-lifecycle/)。

