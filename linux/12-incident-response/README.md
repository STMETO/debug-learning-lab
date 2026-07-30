# 实验 12：综合实战 —— 服务为什么启动失败？

> 🎯 **目标**：用本模块工具区分缺失配置、端口占用和正常运行三种状态。

先构建：

~~~bash
make
~~~

案例一，缺少环境变量：

~~~bash
./incident_service
echo $?
~~~

案例二，配置路径错误：

~~~bash
APP_CONFIG=/missing.conf ./incident_service
~~~

案例三，正确启动：

~~~bash
APP_CONFIG="$PWD/demo.conf" ./incident_service > service.log 2>&1 &
pid=$!
ps -fp "$pid"
tr '\0' '\n' < /proc/$pid/environ | grep APP_CONFIG
lsof -p "$pid"
ss -ltnp | grep 18081
~~~

再次启动同一个服务，第二个实例会因 bind 报 Address already in use。不要猜测“端口被占用”：先用 ss -ltnp 找到 PID，再 ps -fp 确认它是否就是第一个实例。

最终清理：

~~~bash
kill "$pid"
wait "$pid" 2>/dev/null || true
~~~

完成本模块后，你已经具备运行 strace、tcpdump、perf 之前最常用的现场收集能力。

