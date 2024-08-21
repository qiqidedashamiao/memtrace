import io
from logging import warn
import logging
from threading import local
import sys
import subprocess
import time
from tkinter import simpledialog
import numpy as np
import matplotlib.pyplot as plt
import paramiko
import sys
import io
import time
from PIL import Image
import cv2
import pyautogui
from pyzbar import pyzbar

import qrcode

from cmd_meminfo import MemInfoCmd
from localutil import parse_ascii_to_image, trim_binary_data

#定义一个ssh连接类，包含创建连接，登录，定时获取内存信息等功能
class SSHConnection:
    def __init__(self, device_config,configApp):
        self.logger = logging.getLogger("master")
        self.host = device_config["device"]["host"]
        self.username = device_config["device"]["username"]
        self.password = device_config["device"]["password"]
        self.max_receive_length = 32768
        if "max_receive_length" in device_config:
            self.max_receive_length = device_config["max_receive_length"]
        self.logger.info(f"host:{self.host}, username:{self.username}, password:{self.password}")
        self.client = None
        self.shell = None
        self.cmdObj = MemInfoCmd(device_config,configApp)
        # if self.connect():
        #     self.start()
        #     pass

    def connect(self):
        self.logger.info(f"开始创建远程连接......")
        try:
            self.client = paramiko.SSHClient()
            self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            self.client.connect(self.host, username=self.username, password=self.password)
            # 使用invoke_shell创建交互式shell会话
            self.shell = self.client.invoke_shell()
            self.shell = self.client.invoke_shell()
            #self.login()
            if self.login():
                self.start()
                return True
            else:
                self.close()
            return False
        except Exception as e:
            self.logger.error(f"远程节点创建失败，错误信息: {e}")
            #print(f"远程节点创建失败，错误信息: {e}")
            #sys.exit(1)
            return False
        
    def login(self):
        self.logger.info(f"开始登录......")
        try:
            # 读取初始提示符
            time.sleep(1)
            if self.shell.recv_ready():
                output = self.shell.recv(self.max_receive_length).decode('utf-8')
                self.logger.info(f"{output}")
                #print(output)
            
            self.shell.send('python3 /root/mount/share/memtrace/server/erweima.py' + '\n')

            # 输入shell命令
            user_input = "shell"
            self.shell.send(user_input + '\n')
            # 等待命令执行完成并读取输出
            time.sleep(1)
            if self.shell.recv_ready():
                output = self.shell.recv(self.max_receive_length).decode('utf-8')
                self.logger.info(f"{output}")
                #print(output)
            # 输入用户名
            #username = "234646"
            #self.shell.send(username + '\n')
            self.shell.send(self.username + '\n')
            img = None
            # 读取二维码
            time.sleep(1)
            if self.shell.recv_ready():
                output = self.shell.recv(self.max_receive_length)
                self.logger.info(f"{output}")
                output = trim_binary_data(output)
                output = output.decode('utf-8')
                self.logger.info(f"{output}")
                #输出output的大小
                self.logger.info(f"output size:{len(output)}")
                # 将 ASCII 图案转为图片，可行
                img = parse_ascii_to_image(output)
                #img.show()
                # 保存图片为png格式
                img.save('qrcode.png')

                ##读取图片解析
                # image = Image.open('qrcode.png')
                # # 转换为灰度图像
                # # img = img.convert('L')
                # # 使用 pyzbar 库解码二维码
                # decoded_objects = pyzbar.decode(image)

                # # 截屏当前屏幕
                # screenshot = pyautogui.screenshot()
                # # 将截图保存到内存中 (无需保存到磁盘)
                # screenshot = screenshot.convert("RGB")
                # # 解析截图中的二维码
                # decoded_objects = pyzbar.decode(screenshot)

                # # 遍历找到的所有二维码并打印结果
                # for obj in decoded_objects:
                #     self.logger.info(f"类型:{obj.type}")
                #     self.logger.info(f"数据:\r\n{obj.data.decode('utf-8')}")
                #     self.logger.info(f"位置:{obj.rect}")

                # if not decoded_objects:
                #     self.logger.info(f"未检测到二维码")
                
            # 使用matplotlib显示
            # self.logger.info(f"显示二维码")
            # plt.imshow(np.array(img), cmap='gray')
            # plt.axis('off')  # 关闭坐标轴显示
            # plt.show(block=False)  
            # 输入密码
            self.logger.info(f"输入验证码：")
            # 使用simpledialog来获取用户输入的验证码
            password = simpledialog.askstring("验证码", "请输入验证码：")
            plt.close()  
            self.logger.info(f"验证码：{password}")
            self.shell.send(password + '\n')

            # 等待命令执行完成并读取输出
            time.sleep(1)
            if self.shell.recv_ready():
                output = self.shell.recv(self.max_receive_length).decode('utf-8')
                self.logger.info(f"{output}")
                #print(output)
            self.logger.info(f"登录成功")
            return True
        except Exception as e:
            self.logger.error(f"登录失败，错误信息: {e}")
            return False

    def close(self):
        self.logger.info(f"开始关闭定时器......")
        self.cmdObj.stop()
        if self.client:
            self.logger.info(f"开始关闭远程连接......")
            #print("\n开始关闭远程连接......\n")
            self.client.close()
            self.logger.info(f"远程连接关闭成功")
            #print("\n远程连接关闭成功\n")
        else:
            self.logger.info(f"远程连接已关闭，无需手动关闭")
            #print("\n远程连接已关闭，无需手动关闭\n")

    def start(self):
        self.cmdObj.start(self.shell)







