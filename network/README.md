# Linux 网络排障与 tcpdump

本模块延续 Linux 现场排障的证据链：先用 ip、ss 确认本机状态，再用 tcpdump 确认报文是否真的到达。所有示例默认使用 loopback（127.0.0.1），无需外网。

## 学习路线

| # | 实验 | 核心内容 |
|---|------|----------|
| [01](01-network-map/) | 网络现场地图 | 接口、路由、DNS、端口 |
| [02](02-ip-basics/) | ip 命令 | addr、route、neigh |
| [03](03-connectivity-tools/) | 连通性工具 | ping、traceroute、curl、nc |
| [04](04-ss-tcpdump/) | 两层观察 | ss 与 tcpdump 的边界 |
| [05](05-tcpdump-basics/) | 首次抓包 | 接口、-n、-vv、-c |
| [06](06-bpf-filters/) | BPF 过滤 | host、port、net、tcp/udp |
| [07](07-tcp-lifecycle/) | TCP 生命周期 | 握手、数据、挥手 |
| [08](08-dns/) | DNS | 查询、失败和抓包 |
| [09](09-http/) | HTTP | 请求、响应、服务端证据 |
| [10](10-udp/) | UDP | 无连接协议与收发观察 |
| [11](11-pcap-wireshark/) | pcap | 保存、读取、Wireshark |
| [12](12-network-diagnosis/) | 综合实战 | refused、timeout、DNS、监听地址 |

安装：

~~~bash
sudo apt install iproute2 iputils-ping traceroute tcpdump curl netcat-openbsd
~~~

抓包通常需要 sudo 或容器的 NET_RAW/NET_ADMIN 能力。从 [实验 01](01-network-map/) 开始。

