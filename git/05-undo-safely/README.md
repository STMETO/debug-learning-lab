# 实验 05：安全撤销 —— 先选影响范围

> 🎯 **目标**：区分 restore、reset、revert。

~~~bash
../create-demo-repo.sh sandbox && cd sandbox
echo temporary >> app.conf
git restore app.conf
git revert HEAD
git log --oneline -3
~~~

restore 丢弃未提交工作区修改；reset HEAD 文件只取消暂存；revert 新建反向提交，适合已共享历史。

下一步：[stash 与 tag](../06-stash-tags/)。

