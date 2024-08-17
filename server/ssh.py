import io
from logging import warn
from threading import local
import sys
import subprocess
import time
import importlib.util
import paramiko
import sys
import io
import time
from PIL import Image

#安装module
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

# # 先安装fabric模块
# installPipModule("fabric")
# from fabric import Connection

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


# 将ASCII二维码转换为图像
def parse_ascii_to_image(ascii_data):
    # 按行拆分 ASCII 数据
    lines = ascii_data.splitlines()

    # 确定图像大小
    width, height = len(lines[0]), len(lines)

    print(width, height)

    # 创建黑白图片
    img = Image.new('1', (width, height*2), color=1)  # '1'模式是1位黑白图像
    pixels = img.load()

    # 填充图像像素 0 黑色 1 白色
    color1 = 1
    color2 = 0
    for y, line in enumerate(lines):
        for x, char in enumerate(line):
            index = y*2
            if char == '█':
                pixels[x,index] = color1 
                pixels[x, index+1] = color1
            elif char == '▀':
                pixels[x, index] = color1  # （显示上半部分）
                pixels[x, index+1] = color2  # （显示下半部分）
            elif char == '▄':
                pixels[x,index] = color2  # （显示上半部分）
                pixels[x, index+1] = color1  # （显示下半部分）
            elif char == ' ':  # 不间断空格
                pixels[x, index] = color2  
                pixels[x, index+1] = color2
    
    # 放大图像
    scale = 30
    new_width, new_height = width * scale, height * scale
    img = img.resize((new_width, new_height), Image.NEAREST)  # NEAREST用于放大时保持像素的锐利边缘

    return img

#二进制数据中截取掉第一个\r\n前的数据以及最后一个\r\n后的数据
def trim_binary_data(binary_data):
    # 查找第一个 \r\n 的位置
    first_crlf_pos = binary_data.find(b'\r\n')
    
    # 查找最后一个 \r\n 的位置
    last_crlf_pos = binary_data.rfind(b'\r\n')

    # 如果找到了 \r\n, 则进行截取
    if first_crlf_pos != -1 and last_crlf_pos != -1 and first_crlf_pos != last_crlf_pos:
        # 截取第一个 \r\n 后和最后一个 \r\n 前的数据
        trimmed_data = binary_data[first_crlf_pos + 2:last_crlf_pos]
        return trimmed_data
    else:
        # 如果没有找到，或数据无效，返回原数据
        return binary_data


shell.send('python3 /root/mount/share/memtrace/server/erweima.py' + '\n')


# 读取初始提示符
time.sleep(1)
if shell.recv_ready():
    output = shell.recv(8192)
    #返回的二进制数据中截取掉''python3 /root/mount/share/memtrace/server/erweima.py\r\n'
    #output = output[len('python3 /root/mount/share/memtrace/server/erweima.py\r\n'):]
    
    #返回的二进制数据中截取掉尾部的[root@vm14 ~]# 
    #output = output[:-len('\r\n[root@vm14 ~]# ')]

    #返回的二进制数据中截取掉第一个\r\n前的数据以及最后一个\r\n后的数据
    output = trim_binary_data(output)
    
    output = output.decode('utf-8')
    print(output)
    #输出output的大小
    print(len(output))
    
    # 将 ASCII 图案转为图片，可行
    img = parse_ascii_to_image(output)
    img.show()  # 展示图像


# 关闭远程连接
if remote:
    print("\n开始关闭远程连接......\n")
    remote.close()
    print("\n远程连接关闭成功\n")
else:
    print("\n远程连接已关闭，无需手动关闭\n")





