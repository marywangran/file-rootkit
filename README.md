# file-rootkit
hide a file

```bash
[root@localhost hidefile]# ls	# 未加载模块，可以看到skinshoe
a  afile  b  c  hide_file.ko  skinshoe  test  wu
[root@localhost hidefile]# cat skinshoe	# 可以读出skinshoe的内容
1111
[root@localhost hidefile]# insmod ./hide_file.ko	# 加载模块，隐藏skinshoe
insmod: ERROR: could not insert module ./hide_file.ko: Operation not permitted
[root@localhost hidefile]# ls	# 确认文件被隐藏，确实看不到了
a  afile  b  c  hide_file.ko  test  wu
[root@localhost hidefile]# cat skinshoe	# 然而依然可以被打开并读取内容
1111
[root@localhost hidefile]# echo 2222 >>./skinshoe	# 追加点内容试试
[root@localhost hidefile]# ls	# 依然看不到skinshoe
a  afile  b  c  hide_file.ko  test  wu
[root@localhost hidefile]# insmod ./hide_file.ko hide=0	# 恢复skinshoe的显示
insmod: ERROR: could not insert module ./hide_file.ko: Operation not permitted
[root@localhost hidefile]# ls	# 确认一下，确实看到了
a  afile  b  c  hide_file.ko  skinshoe  test  wu
[root@localhost hidefile]# cat skinshoe	# 确认内容，隐藏期间的追加成功了！
1111
2222
[root@localhost hidefile]#
```
