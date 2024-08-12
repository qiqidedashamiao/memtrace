ls ../list/memery-*.log > list.txt
find ~ -name memery*.log > parse/list.txt
parse/mem_log_parse 0 0 parse/list.txt > parse/parselog.txt

0   可执行文件
1   是否翻译地址到符号0 不翻译  1 翻译 ，默认不显示
2   解析日志，0，不显示，1显示， 默认不显示
3   解析文件列表文件名，不存在的话，会根据config.cfg配置文件中的PATH_LIST_DIR自动生成list.txt，可不填

可简化为parse/mem_log_parse > parse/parselog.txt


echo "1 1 0 0 2048" > param
echo "0 0 0 0 0" > param
1 是否开启内存追踪总开关
1 是否开启栈回溯，暂未使用
0 申请的内存大小限制
0 线程ID限制
2048 记录大内存申请


export LD_PRELOAD=/root/mount/share/memtrace/libpreload.so
#export LD_PRELOAD=/home/234646/tools/git/workspace/memtrace/libpreload.so
export -n LD_PRELOAD


编译动态链接库命令：
make
编译test测试文件命令
make test


rm -rf list/*  \r
rm -rf map/* \r
rm -rf parse/list.txt \r



字写栈帧方法
https://cloud.tencent.com/developer/article/1198218

地址空间布局随机化（ASLR）
随机化内存地址：
堆：堆的起始地址在每次程序运行时都不同。
栈：栈的起始地址和栈帧的布局会在每次程序运行时随机化。
动态链接库：动态链接库（如 .so 或 .dll 文件）的加载地址在每次程序运行时都不同。
可执行文件：整个可执行文件的加载地址也会被随机化（在支持 PIE（Position Independent Executable） 的系统中）。

Linux：大多数现代 Linux 发行版默认启用了 ASLR，可以通过 /proc/sys/kernel/randomize_va_space 文件来查看和设置 ASLR 状态。
值为 0 表示禁用 ASLR。
值为 1 表示启用基本随机化。
值为 2 表示启用完全随机化（包括堆和栈）。

查看可执行文件属性：使用 readelf 工具查看可执行文件的 ELF 头部信息，检查是否启用了 PIE 支持。
readelf -h your_executable
在输出结果中，查找以下字段：
Type：如果显示为 DYN，则表示该可执行文件是位置无关的。
Flags：查找 PIE 标志。

验证随机化效果：可以通过多次运行同一可执行文件，并使用 pmap 或 cat /proc/<pid>/maps 查看进程的内存映射，确认每次运行时的加载地址是否不同。

启用 PIE 编译：如果需要编译支持 PIE 的可执行文件，可以在编译时使用 -fPIE 选项，并在链接时使用 -pie 选项。

例如，运行可执行文件并查看其内存映射：
./sonia &
pmap $!