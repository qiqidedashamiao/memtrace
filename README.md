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

rm -rf /mnt/appdata/mount/list/* \r
rm -rf /share/memtrace/map/* \r
rm -rf /mnt/appdata/mount/list.txt \r

ls -lh /mnt/appdata/mount/list \r
ls -lh /share/memtrace/map \r

ls /share/234646/list/memery-*.log > list.txt



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

打包python工程命令
python setup.py build
详细说明：
使用 cx_Freeze，也可以打包多个 Python 文件。假设项目结构相同，以下是步骤：
1. 创建 setup.py
你需要在项目根目录中创建一个 setup.py 脚本，用于定义打包配置。
'''
from cx_Freeze import setup, Executable

# 指定主脚本和依赖
executables = [Executable("main.py")]

# 设置打包选项
setup(
    name="MyApp",
    version="0.1",
    description="My Python Application",
    executables=executables
)
'''

2. 运行打包命令
在项目根目录下运行以下命令：

python setup.py build
生成的可执行文件会出现在 build 目录中，包含所有依赖的模块和文件。
D:\program\install_source\python-312\python.exe main.py


python安装插件包：
pip install numpy -i https://pypi.tuna.tsinghua.edu.cn/simple/
1. 选择合适的国内镜像源
国内有多个知名的Python包镜像源可供选择，如清华大学、阿里云、中国科学技术大学等。这些镜像源定期同步国外的PyPI仓库，因此包含了大多数常用的Python包。以下是一些常用的国内镜像源地址：

清华大学：https://pypi.tuna.tsinghua.edu.cn/simple/
阿里云：http://mirrors.aliyun.com/pypi/simple/
中国科学技术大学：https://pypi.mirrors.ustc.edu.cn/simple/
2. 使用pip安装时指定镜像源
在安装Python插件时，可以通过在pip命令后加上-i参数来指定镜像源。

依赖的插件包：
D:\program\install_source\python-312\Scripts\pip3.exe install pyprogect -i https://pypi.tuna.tsinghua.edu.cn/simple/
D:\program\install_source\python-312\Scripts\pip3.exe install numpy -i https://pypi.tuna.tsinghua.edu.cn/simple/
D:\program\install_source\python-312\Scripts\pip3.exe install matplotlib -i https://pypi.tuna.tsinghua.edu.cn/simple/
D:\program\install_source\python-312\Scripts\pip3.exe install paramiko -i https://pypi.tuna.tsinghua.edu.cn/simple/
#D:\program\install_source\python-312\Scripts\pip3.exe install cv2 -i https://pypi.tuna.tsinghua.edu.cn/simple/
#pip3 install cv2 -i http://mirrors.aliyun.com/pypi/simple/
#pip3 install cv2 -i https://pypi.mirrors.ustc.edu.cn/simple/
D:\program\install_source\python-312\Scripts\pip3.exe install pyzbar -i https://pypi.tuna.tsinghua.edu.cn/simple/
D:\program\install_source\python-312\Scripts\pip3.exe install qrcode -i https://pypi.tuna.tsinghua.edu.cn/simple/
D:\program\install_source\python-312\Scripts\pip3.exe install apscheduler -i https://pypi.tuna.tsinghua.edu.cn/simple/
D:\program\install_source\python-312\Scripts\pip3.exe install pyautogui -i https://pypi.tuna.tsinghua.edu.cn/simple/

#D:\program\install_source\python-312\Scripts\pip3.exe install zbar -i https://pypi.tuna.tsinghua.edu.cn/simple/ --only-binary=:all

D:\program\install_source\python-312\Scripts\pip3.exe install openpyxl -i https://pypi.tuna.tsinghua.edu.cn/simple/


内存变化：
第一阶段：
0、根据ssh和telnet自动选择连接方式以及是否需要校验，telnet可之后再做，dshell去掉，根据结果自动决定
1、内存信息写入excel表格，时间+内存列
2、展示信息横坐标按时间显示
    可增加选择按钮，按照最小时间间隔，5min，30分钟，1小时显示，总共显示50个点（显示的点个数可放在配置文件里）
3、纵坐标剩余内存：
    上下限和刻度阶梯放在设备配置里，增加一个线分割
4、鼠标点击每个点时能显示具体信息
    点加粗，时间，剩余内存，其他。。。
第二阶段：
0、支持telnet连接
1、可支持脚本命令
根据脚本执行设备命令

内存详情监听：
