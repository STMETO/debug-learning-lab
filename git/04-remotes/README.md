# 实验 04：远程仓库 —— fetch 先观察

> 🎯 **目标**：用本地 bare 仓库模拟 origin。

~~~bash
git init --bare origin.git
../create-demo-repo.sh sandbox
git -C sandbox remote add origin ../origin.git
git -C sandbox push -u origin main
git clone origin.git colleague
git -C colleague log --oneline
~~~

fetch 只下载远程引用；pull = fetch + 合并/变基。排障时优先分两步执行并检查差异。

下一步：[安全撤销](../05-undo-safely/)。

