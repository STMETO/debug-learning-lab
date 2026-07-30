# 实验 05：文本管道 —— 把日志变成可验证的证据

> 🎯 **目标**：用 grep、awk、sed、sort、uniq 从日志中提取错误、聚合路径和筛选慢请求。

~~~bash
grep ' ERROR ' app.log
grep -E 'ERROR|WARN' app.log
awk '/ERROR/ {print $4, $6}' app.log
awk -F'path=| ' '{print $2}' app.log | sort | uniq -c
sed -n '2,4p' app.log
~~~

筛选超过 100ms 的请求：

~~~bash
awk 'match($0, /latency_ms=([0-9]+)/, a) && a[1] > 100 {print}' app.log
~~~

命令行的价值在于可组合、可记录。排障结论应保留命令、输入范围和输出，而不只是口头说“好像 /orders 很慢”。

下一步：[管道与重定向](../06-pipes-redirection/)。

