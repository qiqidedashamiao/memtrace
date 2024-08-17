import io
from logging import warn
import logging
from threading import local
import sys
import subprocess
import time
import paramiko
import sys
import io
import time
from PIL import Image
import tkinter as tk

from cmd_meminfo import MemInfoCmd
from localutil import parse_ascii_to_image, trim_binary_data
from gui_app import ConfigApp

#定义一个ssh连接类，包含创建连接，登录，定时获取内存信息等功能
class SSHConnection:
    def __init__(self, host, username, password):
        self.host = host
        self.username = username
        self.password = password
        self.client = None
        self.shell = None
        self.cmdObj = MemInfoCmd()
        self.connect()
        self.start()

    def connect(self):
        try:
            self.client = paramiko.SSHClient()
            self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            self.client.connect(self.host, username=self.username, password=self.password)
            # 使用invoke_shell创建交互式shell会话
            self.shell = self.client.invoke_shell()
            self.shell = self.client.invoke_shell()
            self.login()
            return True
        except Exception as e:
            print(f"远程节点创建失败，错误信息: {e}")
            sys.exit(1)
            return False
        
    def login(self):
        # 读取初始提示符
        time.sleep(1)
        if self.shell.recv_ready():
            output = self.shell.recv(1024).decode('utf-8')
            print(output)
        
        self.shell.send('python3 /root/mount/share/memtrace/server/erweima.py' + '\n')

        # 输入shell命令
        user_input = "shell"
        self.shell.send(user_input + '\n')
        # 等待命令执行完成并读取输出
        time.sleep(2)
        if self.shell.recv_ready():
            output = self.shell.recv(1024).decode('utf-8')
            print(output)
        # 输入用户名
        #username = "234646"
        #self.shell.send(username + '\n')
        self.shell.send(self.username + '\n')
        # 读取二维码
        time.sleep(1)
        if self.shell.recv_ready():
            output = self.shell.recv(8192)
            output = trim_binary_data(output)
            output = output.decode('utf-8')
            print(output)
            #输出output的大小
            print(len(output))
            # 将 ASCII 图案转为图片，可行
            img = parse_ascii_to_image(output)
            img.show()
        # 输入密码
        print("请输入密码：")
        password = "pass"
        #self.shell.send(username + password + '\n')
        self.shell.send(self.username + self.password + '\n')

        # 等待命令执行完成并读取输出
        time.sleep(3)
        if self.shell.recv_ready():
            output = self.shell.recv(1024).decode('utf-8')
            print(output)

    def close(self):
        self.cmdObj.stop()
        if self.client:
            print("\n开始关闭远程连接......\n")
            self.client.close()
            print("\n远程连接关闭成功\n")
        else:
            print("\n远程连接已关闭，无需手动关闭\n")

    def start(self):
        self.cmdObj.start(self.shell)

if __name__ == '__main__':
    #pass
    # 设置默认编码为utf-8
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
    # 配置日志记录
    logging.basicConfig(level=logging.INFO)
    # ssh = SSHConnection("192.168.0.14", "root", "Aa1234561")
    # user_input = input("请输入结束字符exit：")
    # # 关闭MemInfoCmd中的定时器
    # if user_input == "exit":
    #     ssh.close()

    root = tk.Tk()
    app = ConfigApp(root)
    root.mainloop()






