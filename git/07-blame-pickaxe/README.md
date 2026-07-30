# 实验 07：blame 与 pickaxe —— 谁、何时、为什么

> 🎯 **目标**：定位一行配置的引入提交。

~~~bash
../create-demo-repo.sh sandbox && cd sandbox
git blame app.conf
git log -S'timeout_ms=200' -- app.conf
git log -G'^retries=' -p -- app.conf
~~~

blame 给出最后改动一行的提交；-S 搜索文本出现/消失；-G 用正则搜索 diff。结论前总要 git show 阅读上下文。

下一步：[bisect](../08-bisect/)。

