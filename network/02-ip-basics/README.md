# 实验 02：ip —— 接口、路由和邻居表

> 🎯 **目标**：用 ip 替代过时的 ifconfig/route，读取网络配置的真实来源。

~~~bash
ip -br addr
ip route show
ip route get 127.0.0.1
ip neigh show
ip link show lo
~~~

ip route get 是非常实用的命令：它不要求你猜路由规则，直接告诉你内核会选择哪个接口、源地址和下一跳。loopback 的目标应走 lo；远端局域网目标通常需要邻居表中的 ARP/NDP 条目。

🧪 试试看：比较 ip route get 127.0.0.1 和 ip route get 默认网关地址。

下一步：[连通性工具](../03-connectivity-tools/)。

