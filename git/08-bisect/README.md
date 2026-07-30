# 实验 08：bisect —— 二分定位回归

> 🎯 **目标**：让 Git 自动找出引入 bug 的提交。

~~~bash
../create-bisect-repo.sh bisect-sandbox
cd bisect-sandbox
git bisect start
git bisect bad HEAD
git bisect good HEAD~4
git bisect run ./test.sh
git bisect reset
~~~

test 返回 0=good、非 0=bad。真实项目应先准备稳定、自动、无副作用的复现命令；无法构建的提交可 git bisect skip。

下一步：[reflog](../09-reflog-recovery/)。

