# 实验 03：ping、traceroute、curl、nc —— 每个工具只证明一件事

> 🎯 **目标**：避免把“ping 通”误解为“服务可用”。

~~~bash
ping -c 1 127.0.0.1
traceroute -m 1 127.0.0.1
nc -vz 127.0.0.1 1
curl -v --max-time 2 http://127.0.0.1:1/
~~~

| 工具 | 能证明 | 不能证明 |
|------|--------|----------|
| ping | ICMP 可达 | TCP 服务、DNS、HTTP 正常 |
| traceroute | 路由路径线索 | 应用协议成功 |
| nc -vz | TCP connect 能否完成 | HTTP 内容是否正确 |
| curl -v | HTTP/TLS 请求过程 | 其他协议状态 |

端口 1 通常没有服务，因此 nc/curl 应失败。失败类型也有价值：connection refused 表示目标主机立即回了 RST；timeout 更像报文被丢弃或路径不通。

下一步：[ss 与 tcpdump](../04-ss-tcpdump/)。

