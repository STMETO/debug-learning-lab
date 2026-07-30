# 实验 01：Hello Git —— 工作区、暂存区与提交

> 🎯 **目标**：理解 status、add、commit 三个状态转换。

~~~bash
mkdir sandbox && cd sandbox
git init -b main
git config user.name "Lab Learner"
git config user.email "learner@example.test"
echo hello > note.txt
git status
git add note.txt
git commit -m 'docs: add first note'
git log --oneline
~~~

工作区 → git add → 暂存区 → git commit → 历史。每次操作前先运行 git status。

下一步：[历史与差异](../02-history-diff/)。

