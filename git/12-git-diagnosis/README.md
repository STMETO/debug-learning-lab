# 实验 12：综合实战 —— 配置何时变坏？

> 🎯 **目标**：用 pickaxe 和 bisect 给出可复现的回归证据。

~~~bash
../create-bisect-repo.sh incident && cd incident
./test.sh; echo $?
git log -S'41' -p -- value.txt
git bisect start HEAD HEAD~4
git bisect run ./test.sh
git show --stat
git bisect reset
~~~

结论必须包含最后 good、首次 bad、自动测试命令、bisect 提交和 diff；这才是工程证据，而非“猜最近改动”。

