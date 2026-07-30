# 实验 09：reflog —— HEAD 去过哪里

> 🎯 **目标**：从误 reset 后找回提交。

~~~bash
../create-demo-repo.sh sandbox && cd sandbox
git reset --hard HEAD~1
git reflog
git reset --hard HEAD@{1}
git log --oneline
~~~

发现误操作时先停止进一步清理，再用 reflog 保存哈希；它不是远程备份，过久的悬空对象可能被回收。

下一步：[worktree](../10-worktree/)。

