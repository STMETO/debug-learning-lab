#!/bin/sh
set -eu
target=sandbox
[ "$#" -gt 0 ] && target=$1
case "$target" in ''|.|..|*/* ) echo "target must be a local directory name" >&2; exit 2;; esac
rm -rf "$target"; mkdir "$target"; cd "$target"
git init -q -b main
git config user.name "Lab Learner"
git config user.email "learner@example.test"
printf 'timeout_ms=100\nretries=2\n' > app.conf
git add app.conf; git commit -qm 'feat: add initial configuration'
printf 'timeout_ms=200\nretries=2\n' > app.conf; git commit -am 'perf: raise timeout' -q
printf 'timeout_ms=200\nretries=3\n' > app.conf; git commit -am 'fix: retry failures' -q
