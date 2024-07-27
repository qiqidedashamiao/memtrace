find ~ -name memery*.log > parse/list.txt
parse/mem_log_parse 0 parse/list.txt 0 0 1 ./sonia > parse/parselog.txt

0   可执行文件
1   解析类型 默认0
2   解析文件列表文件名
3   解析日志，0，不显示，1显示
4  是否翻译地址到符号0 不翻译  1 翻译
5  是否是全路径
6  可执行文件地址

echo "1 1 0 0" > param
echo "0 0 0 0" > param

export LD_PRELOAD=/root/mount/share/memtrace/libpreload.so
export -n LD_PRELOAD


编译动态链接库命令：
make
编译test测试文件命令
make test
