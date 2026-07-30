# 实验 04：查找文件 —— 先找到真实路径，再谈配置

> 🎯 **目标**：用 find、which、whereis、readlink 区分命令名、PATH、软链接和实际文件。

~~~bash
chmod +x workspace/bin/demo-tool
find workspace -type f -printf '%p\n'
find workspace -name '*.conf' -o -name 'demo-tool'
PATH="$PWD/workspace/bin:$PATH" which demo-tool
readlink -f "$(which demo-tool)"
whereis gcc
~~~

find 的 -o 优先级容易误解；复杂条件应加括号：

~~~bash
find workspace -type f \( -name '*.conf' -o -name '*.log' \)
~~~

排障时不要只相信 PATH 命中的第一个程序。用 readlink -f 确认真实路径，尤其在存在软链接、版本管理器和容器挂载时。

下一步：[文本管道](../05-text-pipelines/)。

