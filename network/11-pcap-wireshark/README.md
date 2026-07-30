# 实验 11：pcap —— 抓取和分析分开进行

> 🎯 **目标**：保存 pcap、离线读取，并交给 Wireshark 做深入分析。

~~~bash
sudo tcpdump -ni lo -w demo.pcap 'tcp port 18080'
tcpdump -nn -r demo.pcap
tcpdump -A -r demo.pcap
~~~

抓包进程运行期间，用实验 07 的 nc 客户端生成流量，再 Ctrl-C 停止抓包。pcap 可复制到开发机后用 Wireshark 打开；捕获过滤器（tcpdump BPF）用于减少写入量，Wireshark 显示过滤器用于在已有 pcap 中筛选，两者语法不同。

下一步：[综合网络诊断](../12-network-diagnosis/)。

