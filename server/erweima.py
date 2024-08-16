#!/usr/bin/env python
# coding=utf-8

import sys
import subprocess
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
installPipModule("qrcode")
installPipModule("Image")

import qrcode
from PIL import Image
import io

import qrcode

def generate_qr_in_console(data):
    qr = qrcode.QRCode(
        version=1,  # 控制二维码的大小
        error_correction=qrcode.constants.ERROR_CORRECT_L,  # 控制二维码的容错率
        box_size=1,  # 控制输出的大小，这里设置为1是为了控制台输出
        border=4,  # 控制边框的宽度
    )
    
    qr.add_data(data)
    qr.make(fit=True)

    # 将二维码转化为控制台可显示的字符图案
    qr.print_ascii(invert=True)  # invert=True 反转黑白显示效果


def display_qr_image(image_data):
    # 将二进制图像数据转换为 PIL 图像对象
    img = Image.open(io.BytesIO(image_data))
    
    # 显示图像
    img.show()

# 假设 `received_image_data` 是从客户端接收到的二进制图像数据
# received_image_data = ...

# 展示接收到的二维码图像
#display_qr_image(received_image_data)


# 要展示的数据
data = "https://www.example.com"

# 生成并在控制台展示二维码
generate_qr_in_console(data)

