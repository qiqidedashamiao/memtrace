# import io
# from logging import warn
# from threading import local
# import sys
# import subprocess
# import time
# import importlib.util

# def installPipModule(module):
#     if importlib.util.find_spec(module) is not None:
#         return
#     print(f"\n{module}模块未安装，先安装{module}模块\n")
#     try:
#         cmd = f"pip install {module}"
#         print(cmd)
#         subprocess.call(cmd, shell=True)
#     except subprocess.CalledProcessError as e:
#         print(f"\n安装{module}模块时异常：\n", e.output)
#         sys.exit(1)
#     print(f"\n{module}模块安装成功\n")

# # 先安装fabric模块
# installPipModule("fabric")
# from fabric import Connection
# import paramiko

# def genRemoteConnectionFunc(host, pwd):
#     con = Connection(host=f"{host}", connect_kwargs={'password': pwd})
#     try:
#         con.open()
#     except Exception:
#         print(f"远程节点创建失败，hostIp = {host}")
#         sys.exit(1)

#     if con.is_connected:
#         print("远程节点连接创建成功")
#         return con
#     else:
#         print(f"远程节点创建失败，hostIp = {host}")
#         sys.exit(1)

# # 设置默认编码为utf-8
# sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
# remote = genRemoteConnectionFunc("192.168.0.14","Aa1234561")
# #不要显示run的输出
# #res=remote.run("addr2line -a -e /root/mount/share/memtrace/sonia -f -C 0x000000000041022f", warn=True, echo=False,hide=True).stdout
# res=remote.run("/root/mount/share/memtrace/sonia", warn=True, echo=False,hide=True).stdout
# print("00000")
# print(res)
# print("33333")
# # 输入数字0
# user_input = input("请输入数字0: ")

# res = remote.run(user_input, warn=True, echo=False, hide=True).stdout

# print("11111")
# print(res)
# print("2222")

# if remote is not None:
#     print("\n开始关闭远程连接......\n")
#     remote.close()
#     print("\n远程连接关闭成功\n")
# else:
#     print("\n远程连接已关闭，无需手动关闭\n")


import paramiko
import sys
import io
import time

# 设置默认编码为utf-8
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

def genRemoteConnectionFunc(host, username, password):
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(host, username=username, password=password)
    return client

try:
    remote = genRemoteConnectionFunc("192.168.0.14", "root", "Aa1234561")
    print("远程节点连接创建成功")
except Exception as e:
    print(f"远程节点创建失败，错误信息: {e}")
    sys.exit(1)

# 使用invoke_shell创建交互式shell会话
shell = remote.invoke_shell()

# 读取初始提示符
time.sleep(1)
if shell.recv_ready():
    output = shell.recv(1024).decode('utf-8')
    print(output)

# shell.send("/root/mount/share/memtrace/sonia" + '\n')

# # 读取初始提示符
# time.sleep(1)
# if shell.recv_ready():
#     output = shell.recv(1024).decode('utf-8')
#     print(output)

# # 输入命令
# user_input = input()
# shell.send(user_input + '\n')

# # 等待命令执行完成并读取输出
# time.sleep(2)
# if shell.recv_ready():
#     output = shell.recv(1024).decode('utf-8')
#     print(output)

shell.send("python /root/mount/share/memtrace/server/erweima.py" + '\n')
from PIL import Image
def display_qr_image(image_data):
    # 将二进制图像数据转换为 PIL 图像对象
    img = Image.open(io.BytesIO(image_data))
    
    # 显示图像
    img.show()

# 读取初始提示符
time.sleep(1)
if shell.recv_ready():
    output = shell.recv(1024)
    display_qr_image(output)


# 关闭远程连接
if remote:
    print("\n开始关闭远程连接......\n")
    remote.close()
    print("\n远程连接关闭成功\n")
else:
    print("\n远程连接已关闭，无需手动关闭\n")
