# 实验 07：TCP 生命周期 —— 把握手、数据和挥手放在一起看

> 🎯 **目标**：用 nc 生成一次短连接，在 tcpdump 中识别 SYN、ACK、数据和 FIN。

终端 A：

~~~bash
make && ./tcp_demo
~~~

终端 B：

~~~bash
sudo tcpdump -ni lo -c 20 'tcp port 18080'
~~~

终端 C：

~~~bash
printf 'hello\n' | nc 127.0.0.1 18080
~~~

抓包中按顺序找：SYN → SYN,ACK → ACK → 数据 → FIN/ACK。TCP 连接完成不等于应用成功；服务端 read、处理和 write 仍可能失败。

下一步：[DNS](../08-dns/)。

