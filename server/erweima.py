
import sys
import subprocess
import importlib.util

# 先安装fabric模块
#installPipModule("qrcode")
#installPipModule("Image")

import qrcode
from PIL import Image
import io

import qrcode

def generate_qr_in_console(data):
    qr = qrcode.QRCode(
        version=1,  # 控制二维码的大小
        error_correction=qrcode.constants.ERROR_CORRECT_L,  # 控制二维码的容错率
        box_size=1,  # 控制输出的大小，这里设置为1是为了控制台输出
        border=0,  # 控制边框的宽度
    )
    
    qr.add_data(data)
    qr.make(fit=True)

    # 将二维码转化为控制台可显示的字符图案
    qr.print_ascii(invert=False)  # invert=True 反转黑白显示效果


# 假设 `received_image_data` 是从客户端接收到的二进制图像数据
# received_image_data = ...

# 展示接收到的二维码图像
#display_qr_image(received_image_data)


# 要展示的数据
data = "aaaaaa"

user_input = input("请输入shell:")
if user_input == "shell":
    username = input("请输入用户名：")
    # 生成并在控制台展示二维码
    generate_qr_in_console(username)
    password = input("请输入密码：")





