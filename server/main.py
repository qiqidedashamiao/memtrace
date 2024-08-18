import io
import logging
import sys
from collections import defaultdict

import tkinter as tk

import struct

from gui_app import ConfigApp

if __name__ == "__main__":
    #pass
    # 设置默认编码为utf-8
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
    # 配置日志记录
    logging.basicConfig(level=logging.DEBUG)

    # ssh = SSHConnection("192.168.0.14", "root", "Aa1234561")
    # user_input = input("请输入结束字符exit：")
    # # 关闭MemInfoCmd中的定时器
    # if user_input == "exit":
    #     ssh.close()

    root = tk.Tk()
    app = ConfigApp(root)
    root.mainloop()