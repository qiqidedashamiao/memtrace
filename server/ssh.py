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
import pyautogui
from pyzbar import pyzbar
import threading
from tkinter import messagebox
import urllib
from urllib.parse import unquote
import ssl
from urllib.request import Request, urlopen

import qrcode

from cmd_meminfo import MemInfoCmd
from localutil import parse_ascii_to_image, trim_binary_data, parse_ascii_to_resv

USER_OR_PW_ERROR = "user or password error"
URL_OPEN_ERROR = "urlopen error"

#定义一个ssh连接类，包含创建连接，登录，定时获取内存信息等功能
class SSHConnection:
    def __init__(self, device_config,configApp):
        self.logger = logging.getLogger("master")
        self.host = device_config["device"]["host"]
        self.username = device_config["device"]["username"]
        self.password = device_config["device"]["password"]
        self.max_receive_length = 32768
        self.user = device_config["user"]["username"]
        self.pwd = device_config["user"]["password"]
        if "max_receive_length" in device_config:
            self.max_receive_length = device_config["max_receive_length"]
        self.logger.info(f"host:{self.host}, username:{self.username}, password:{self.password}")
        self.client = None
        self.shell = None
        self.cmdObj = MemInfoCmd(device_config,configApp)
        self.state = True
        self.ssh_thread = threading.Thread(target=self.connect, args=())
        self.ssh_thread.start()
            
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
                self.state = False
                self.close()
            messagebox.showinfo("内存变化", "内存监听启动失败")
            return False
        except Exception as e:
            self.logger.error(f"远程节点创建失败，错误信息: {e}")
            #print(f"远程节点创建失败，错误信息: {e}")
            #sys.exit(1)
            self.state = False
            messagebox.showinfo("内存变化", "内存监听启动失败")
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

            self.logger.info(f"发送shell:")
            # 输入shell命令
            user_input = "shell"
            self.shell.send(user_input + '\n')
            # 等待命令执行完成并读取输出
            time.sleep(1)
            if self.shell.recv_ready():
                output = self.shell.recv(self.max_receive_length).decode('utf-8')
                self.logger.info(f"{output}")
            # 输入用户名
            self.shell.send(self.user + '\n')
            img = None
            password = None
            # 读取二维码
            time.sleep(1)
            if self.shell.recv_ready():
                output = self.shell.recv(self.max_receive_length)

                output = trim_binary_data(output)
                #输出output的大小
                #self.logger.info(f"output size:{len(output)}")
                #self.logger.info(f"{output}")
                output = output.decode('utf-8')
                self.logger.info(f"{output}")
                #output = parse_ascii_to_resv(output)
                # self.logger.info(f"{output}")
                
                # # 将 ASCII 图案转为图片，可行
                img = parse_ascii_to_image(output)
                #img.show()
                # 保存图片为png格式
                img.save('qrcode.png')

                # 转换为灰度图像
                img = img.convert("RGB")
                img = img.convert('L')
                # # 使用 pyzbar 库解码二维码
                decoded_objects = pyzbar.decode(img)

                # # 截屏当前屏幕
                # screenshot = pyautogui.screenshot()
                # # 将截图保存到内存中 (无需保存到磁盘)
                # screenshot = screenshot.convert("RGB")
                # screenshot = screenshot.convert('L')
                # screenshot.save("screenshot.png")
                # # 解析截图中的二维码
                # decoded_objects = pyzbar.decode(screenshot)
                
                if not decoded_objects:
                    self.logger.info(f"未检测到二维码")
               # 遍历找到的所有二维码并打印结果
                for obj in decoded_objects:
                    self.logger.info(f"类型:{obj.type}")
                    self.logger.info(f"数据:\r\n{obj.data.decode('utf-8')}")
                    self.logger.info(f"位置:{obj.rect}")
                    data = obj.data.decode('utf-8')
                    self.logger.info(f"data:{data}")
                    ret,password = self.parse_verity(data)
                    if ret:
                        break
                
            # # 使用matplotlib显示
            # self.logger.info(f"显示二维码")
            # plt.imshow(np.array(img), cmap='gray')
            # plt.axis('off')  # 关闭坐标轴显示
            # plt.show(block=True)  

            # 输入密码
            self.logger.info(f"输入验证码：")
            if password is None:
                # 使用simpledialog来获取用户输入的验证码
                password = simpledialog.askstring("验证码", "请输入验证码：")
            #plt.close()  
            self.logger.info(f"验证码：{password}")
            self.shell.send(password + '\n')

            # 等待命令执行完成并读取输出
            time.sleep(1)
            if self.shell.recv_ready():
                output = self.shell.recv(self.max_receive_length).decode('utf-8')
                self.logger.info(f"{output}")
                if "Verify failed" in output:
                    self.logger.info(f"验证失败")
                    return False
            self.logger.info(f"登录成功")
            return True
        except Exception as e:
            self.logger.error(f"登录失败，错误信息: {e}")
            return False

    def parse_verity(self, obj):
        self.logger.info(f"开始解析二维码信息......")
        if self.__qr_str_info_is_valid(obj):
            auth_code = self.__get_auth_code(obj)
        if 0 == len(auth_code):
            self.logger.error(f"没有获取到有效的授权码,请重试...")
        elif USER_OR_PW_ERROR in auth_code:
            self.logger.error(f"用户名或密码错误, 当前账号: {self.__get_param_value_from_url_by_key(obj, "u")}")
        elif URL_OPEN_ERROR in auth_code:
            self.logger.error(f"打开链接失败,请重试...")
        else:
            self.logger.info(f"分析成功, 授权码:{auth_code}")
            return True,auth_code
        return False,None

    def __get_auth_code(self, qr_str_info):
        url = r"https://svsh.dahuatech.com/cgi-bin/psh/svsh.cgi"
        txt = self.__get_param_value_from_url_by_key(qr_str_info, "t")
        self.logger.info(f"二维码信息: \n{qr_str_info}")
        self.logger.info(f"二维码信息: \n{txt}")
        # glo.LOGGER.debug("text: \n%s" % qr_str_info)
        # glo.LOGGER.debug("text: \n%s" % txt)   
        data = {
                "user" : self.__get_param_value_from_url_by_key(qr_str_info, "u"),
                "pwd"  : self.pwd,
                "ver"  : self.__get_param_value_from_url_by_key(qr_str_info, "v"),
                "txt"  : unquote(txt)
               }
        
        # 登录dShell的用户和当前PC用户不一致, 或者当前PC保存的域密码为空
        # 则跳出用户密码输入对话框要求用户输入域密码
        # host_user = getpass.getuser().strip()
        # if (data["user"] != self.account) or (data["user"] != host_user) or (data["pwd"].strip() == ""):
        #     data["pwd"] = Options.CallAndWait(Options.poup_user_password_entry_dialog, data["user"])
        # 开始请求授权码
        auth_code = ""
        try:
            headers = {"User-Agent" : "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/66.0.3359.139 Safari/537.36"}
            # glo.LOGGER.debug("Request url data: %s" % data)
            #self.logger.info(f"Request url data: {data}")
            value = urllib.parse.urlencode(data)
            # glo.LOGGER.debug("Request url value: %s" % value)
            self.logger.info(f"Request url value: {value}")
            context = ssl._create_unverified_context()
            req = Request(url, data=value.encode('utf-8'), headers=headers)
            response = urlopen(req, context=context)
            # glo.LOGGER.debug("response msg: %s" % response.msg)
            self.logger.info(f"Request url msg: {response.msg}")
            auth_code = response.read().decode('utf-8').replace("\n", "")
            # glo.LOGGER.debug("auth code info: %s" % auth_code)
            self.logger.info(f"Request url info: {auth_code}")
            response.close()
        except Exception as e:
            auth_code = URL_OPEN_ERROR
            self.logger.error(f"请求验证码URL异常，错误信息: {e}")
        return auth_code

    def __qr_str_info_is_valid(self, qr_str):
        """ 判断给定的字符串是否完整的有效信息 """
        self.logger.info(f"开始判断二维码信息是否有效......")
        status = True
        txt = self.__get_param_value_from_url_by_key(qr_str, "t")
        usr = self.__get_param_value_from_url_by_key(qr_str, "u")
        ver = self.__get_param_value_from_url_by_key(qr_str, "v")
        if txt.strip() == ""  or usr.strip() == ""  or ver.strip() == "":
            status = False
            self.logger.info(f"qr_str_info abnormal with ver: {ver}")
            self.logger.info(f"qr_str_info abnormal with usr: {usr}")
            self.logger.info(f"qr_str_info abnormal with txt: {txt}")
        return status

    def __get_param_value_from_url_by_key(self, url, key):
        self.logger.info(f"开始获取二维码信息中的参数......")
        params = url.split("?")[-1]
        for key_value in params.split("&"):
            r = key_value.strip().split("=")
            if key == r[0].strip():
                return r[1].strip()
        return ""

    def close(self):
        self.logger.info(f"开始关闭定时器......")
        #self.ssh_thread.join()
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

    def parse_verity(self, obj):
        if obj.data.decode('utf-8') == self.user:
            self.logger.info(f"验证通过")
            return True,obj.data.decode('utf-8')
        else:
            self.logger.info(f"验证失败")
            return False,None







