# 实验 03：分支与合并 —— 把实验隔离

> 🎯 **目标**：创建修复分支并合并回 main。

~~~bash
../create-demo-repo.sh sandbox && cd sandbox
git switch -c fix/retry-limit
sed -i 's/retries=3/retries=5/' app.conf
git commit -am 'fix: increase retry limit'
git switch main
git merge --no-ff fix/retry-limit
git log --graph --oneline --all
~~~

分支名表达意图；调试实验和主线隔开，回滚范围才清晰。

下一步：[远程仓库](../04-remotes/)。

