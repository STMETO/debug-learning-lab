# 实验 08：DNS —— 名称失败和连接失败不是同一种故障

> 🎯 **目标**：确认 resolver 配置、DNS 查询与主机名解析的边界。

~~~bash
cat /etc/resolv.conf
getent hosts localhost
getent hosts example.com
sudo tcpdump -ni any -c 10 'udp port 53 or tcp port 53'
~~~

另一个终端执行 getent hosts example.com。若环境无外网，查询失败也可以观察；重点是先区分“名称无法解析”和“已得到 IP 但连接失败”。

getent 使用系统 NSS 路径，更接近真实应用；不要只用 nslookup/dig 的结果替代应用行为。

下一步：[HTTP](../09-http/)。

