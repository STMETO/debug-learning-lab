# 实验 12：综合实战 —— 连接失败到底卡在哪一层？

> 🎯 **目标**：用最短命令链区分 refused、timeout、DNS 失败和监听地址错误。

建议按下表执行，而不是先抓所有包：

| 现象 | 第一组命令 | 可能结论 |
|------|------------|----------|
| connection refused | ss -ltnp；tcpdump port | 主机可达但没有对应监听或被主动拒绝 |
| timeout | ip route get；tcpdump host/port | 无路由、防火墙丢包、对端无响应 |
| 名称失败 | getent hosts；resolv.conf；tcpdump port 53 | DNS/NSS 配置或解析服务问题 |
| 本机可访问、远端不行 | ss -ltnp；ip addr | 服务仅绑定 127.0.0.1 |

实战：启动实验 07 服务后，先用 ss 确认它只监听 127.0.0.1:18080；再用 nc 连接该地址并抓包。把服务改为绑定 0.0.0.0 前，先理解这会扩大暴露范围，而不是把它当作通用修复。

完成后，你已经可以把 strace 的 connect 错误、ss 的 socket 状态和 tcpdump 的报文证据串成完整网络排障链。

