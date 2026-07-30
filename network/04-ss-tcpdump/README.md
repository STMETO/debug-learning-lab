# 实验 04：ss 与 tcpdump —— 一个看内核状态，一个看报文

> 🎯 **目标**：知道何时用 ss，何时必须抓包。

~~~bash
ss -ltnp
ss -tan state established
sudo tcpdump -ni lo -c 5 tcp
~~~

ss 读取本机内核 socket 状态：监听、ESTABLISHED、TIME-WAIT、队列和进程归属。tcpdump 则在接口上观察实际经过的报文。

~~~text
ss 说“端口在监听”      不证明客户端的包真的到达
tcpdump 看见 SYN        不证明应用 accept 后正常处理
两者一起使用           才能缩短定位路径
~~~

下一步：[第一次抓包](../05-tcpdump-basics/)。

