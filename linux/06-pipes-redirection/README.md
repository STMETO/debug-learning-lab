# 实验 06：管道与重定向 —— 不丢输出地组合诊断命令

> 🎯 **目标**：掌握 stdout/stderr、tee、xargs、timeout，避免日志只看了一半或命令无限等待。

~~~bash
chmod +x talker.sh
./talker.sh > stdout.log 2> stderr.log
./talker.sh 2>&1 | tee combined.log
printf '%s\n' a.log b.log | xargs -n1 echo "inspect"
timeout 2 sh -c 'sleep 10' || echo "timed out as expected"
~~~

| 写法 | 含义 |
|------|------|
| > file | 覆盖 stdout 到文件 |
| >> file | 追加 stdout |
| 2> file | stderr 到文件 |
| 2>&1 | 将 stderr 合并到当前 stdout 去向 |
| tee file | 既显示又保存 |

先用 xargs -n1 或 echo 验证参数展开，再执行有副作用的命令。timeout 适合保护可能卡住的诊断命令。

下一步：[lsof](../07-lsof/)。

