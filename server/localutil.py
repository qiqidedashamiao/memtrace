
import importlib.util
import subprocess
import sys
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
