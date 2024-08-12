from logging import warn
from threading import local
import sys
import subprocess
import time
import importlib.util

def installPipModule(module):
    if importlib.util.find_spec(module) is not None:
        return
    print(f"\n{module}模块未安装，先安装{module}模块\n")
    try:
        cmd = f"pip install {module}"
        print(cmd)
        subprocess.call(cmd, shell=True)
    except subprocess.CalledProcessError as e:
        print(f"\n安装{module}模块时异常：\n", e.output)
        sys.exit(1)
    print(f"\n{module}模块安装成功\n")

# 先安装fabric模块
installPipModule("fabric")
from fabric import Connection

def genRemoteConnectionFunc(host, pwd):
    con = Connection(host=f"{host}", connect_kwargs={'password': pwd})
    try:
        con.open()
    except Exception:
        print(f"远程节点创建失败，hostIp = {host}")
        sys.exit(1)

    if con.is_connected:
        print("远程节点连接创建成功")
        return con
    else:
        print(f"远程节点创建失败，hostIp = {host}")
        sys.exit(1)


remote = genRemoteConnectionFunc("192.168.0.14","Aa1234561")
#不要显示run的输出
res=remote.run("addr2line -a -e /root/mount/share/memtrace/sonia -f -C 0x000000000041022f", warn=True, echo=False,hide=True).stdout

print(res)