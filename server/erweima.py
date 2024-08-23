
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
        version=10,  # 控制二维码的大小，1 是最小尺寸。你可以将其设置为 1 到 40 的整数，数值越大，生成的二维码尺寸越大，能容纳的信息也越多。
        error_correction=qrcode.constants.ERROR_CORRECT_Q,  # 控制二维码的容错率,ERROR_CORRECT_M：大约 15% 的数据码字可以被修复,ERROR_CORRECT_Q：大约 25% 的数据码字可以被修复。ERROR_CORRECT_H：大约 30% 的数据码字可以被修复。
        box_size=1,  # 控制每个小方块的像素大小。1 是最小的方块大小，适合在控制台输出。对于显示或保存为图片时，可以适当增大 box_size 值来获得更清晰的二维码。
        border=4,  # 控制二维码的边框宽度。0 表示没有边框，通常为了使二维码更易扫描，边框宽度至少设置为 4。
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
    print(f"username:{username}")
    print(f"")
    # 生成并在控制台展示二维码
    generate_qr_in_console(username)
    print(f"")
    password = input("请输入密码：")





