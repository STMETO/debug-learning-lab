# 实验 02：历史与差异 —— 改了什么？

> 🎯 **目标**：用 log、show、diff 审查修改。

~~~bash
../create-demo-repo.sh sandbox && cd sandbox
git log --oneline --decorate
git show HEAD
sed -i 's/timeout_ms=200/timeout_ms=300/' app.conf
git diff
git diff --check
~~~

git diff 看工作区与暂存区；git diff --staged 看暂存区与 HEAD。提交前先读 diff。

下一步：[分支](../03-branches-merge/)。

