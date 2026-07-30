# 实验 09：HTTP —— curl 看协议，tcpdump 看传输

> 🎯 **目标**：用 curl -v 和 tcpdump 对照一次 HTTP 请求响应。

终端 A：make && ./http_demo  
终端 B：sudo tcpdump -ni lo -A -c 20 'tcp port 18082'  
终端 C：curl -v http://127.0.0.1:18082/

curl -v 显示 URL、连接、请求头和响应头；tcpdump -A 显示可打印 payload。HTTP 明文可读仅适用于 HTTP；HTTPS 的 payload 已加密，抓包仍能看 IP、TCP、TLS 握手和时序，但不能直接看到 URL 路径和正文。

下一步：[UDP](../10-udp/)。

