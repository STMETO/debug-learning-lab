#!/bin/sh
set -eu
target=bisect-sandbox
[ "$#" -gt 0 ] && target=$1
case "$target" in ''|.|..|*/* ) echo "target must be a local directory name" >&2; exit 2;; esac
rm -rf "$target"; mkdir "$target"; cd "$target"
git init -q -b main
git config user.name "Lab Learner"; git config user.email "learner@example.test"
printf '#!/bin/sh\n[ "$(cat value.txt)" = 42 ]\n' > test.sh; chmod +x test.sh
echo 42 > value.txt; git add .; git commit -qm 'feat: expected value'
git commit --allow-empty -qm 'refactor: simplify'
echo 41 > value.txt; git commit -am 'perf: optimize calculation'
git commit --allow-empty -qm 'docs: explain calculation'
git commit --allow-empty -qm 'chore: update metadata'
