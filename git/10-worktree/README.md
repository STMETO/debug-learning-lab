# 实验 10：worktree —— 并行复现旧版本

> 🎯 **目标**：在第二个目录检出历史版本。

~~~bash
../create-demo-repo.sh sandbox && cd sandbox
git worktree add ../release-copy HEAD~1
git worktree list
git -C ../release-copy log -1 --oneline
git worktree remove ../release-copy
~~~

worktree 适合主目录继续开发、第二目录复现旧版本或运行 bisect；一个分支不能同时被两个 worktree 检出。

下一步：[cherry-pick](../11-cherry-pick-patches/)。

