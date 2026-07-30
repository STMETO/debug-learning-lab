# 实验 06：stash 与 tag —— 暂存现场，标记基线

> 🎯 **目标**：临时保存 WIP，并标记已验证版本。

~~~bash
../create-demo-repo.sh sandbox && cd sandbox
echo debug=true >> app.conf
git stash push -m 'wip: debug config'
git stash list
git stash pop
git tag -a v1.0.0 -m 'known good'
git show v1.0.0
~~~

stash 适合短期切换，不替代提交；tag 适合记录可复现基线。

下一步：[blame 与 pickaxe](../07-blame-pickaxe/)。

