# 实验 11：cherry-pick 与补丁 —— 精确迁移修复

> 🎯 **目标**：把一个已验证修复迁移到另一条分支。

~~~bash
../create-demo-repo.sh sandbox && cd sandbox
git switch -c fix/timeout
sed -i 's/timeout_ms=200/timeout_ms=250/' app.conf
git commit -am 'fix: tune timeout'
commit=$(git rev-parse HEAD)
git switch main
git cherry-pick "$commit"
git format-patch -1 HEAD --stdout > fix.patch
~~~

cherry-pick 复制一个提交的变更，不复制分支关系；冲突解决后使用 git cherry-pick --continue 并重新测试。

下一步：[综合诊断](../12-git-diagnosis/)。

