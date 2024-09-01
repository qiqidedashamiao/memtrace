import io
import logging
import pdb
import sys
from collections import defaultdict

import tkinter as tk

import struct

from gui_app import ConfigApp
from memorystat.mem_stat_init import mem_stat_init

if __name__ == "__main__":
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

    mem_stat_init(app,"server","内存统计")
    app.start()
    root.mainloop()

