#增加一个命令类，输入参数为连接的shell对象，定时执行cat /proc/meminfo命令，获取内存信息
from datetime import datetime
import logging
import os
import pdb
import queue
import re
import sched
import threading
import time
from apscheduler.schedulers.background import BackgroundScheduler
from apscheduler.triggers.interval import IntervalTrigger
from matplotlib import animation, pyplot as plt
import openpyxl

class MemInfoCmd:
    def __init__(self,device_config,configApp):
        self.is_running = True
        self.meminfo_queue = queue.Queue()
        self.configApp = configApp
        # 将当前时间格式化为字符串
        current_time = datetime.now()
        self.time_str = current_time.strftime("%Y-%m-%d %H-%M-%S")
        
        self.logger = logging.getLogger("master")
        self.logger.info(f"init MemInfoCmd")
        #如果目录data/meminfo不存在，则递归创建目录
        # 定义目录路径
        directory = "data/meminfo"
        os.makedirs(directory, exist_ok=True)
        self.filename = f"data/meminfo/{device_config["device"]["host"]}-meminfo-{self.time_str}.xlsx"
        # with open(self.filename, "a") as f:
        #     pass
        self.shell = None
        self.interval = 5   #默认5s
        if "interval" in device_config["device"]:
            self.interval = device_config["device"]["interval"]
        self.meminfo = []
        self.max_receive_length = 32768
        if "max_receive_length" in device_config:
            self.max_receive_length = device_config["max_receive_length"]

        # self.timer = None
        # self.is_running = False

        self.scheduler = BackgroundScheduler()


    def start(self, shell):
        self.logger.info(f"start")
        self.shell = shell

        # self.timer = threading.Timer(self.interval, self.get_meminfo)
        # self.timer.start()
        self.is_running = True

        self.scheduler.start()
        self.scheduler.add_job(self.get_meminfo, IntervalTrigger(seconds=self.interval))
        
        # self.process_thread = threading.Thread(target=self.process, args=())
        # self.process_thread.start()

    def process(self):
        # 10: interval*10, 100: interval*100 1000: interval*1000
        change_types = [1000,100,10]
        change_counts = 0
        self.logger.info(f"process")
        # pdb.set_trace()  # 设置断点
        while self.is_running:
            
            # pdb.set_trace()  # 设置断点
            self.logger.info(f"meminfo_queue:{self.meminfo_queue.qsize()}")
            if not self.meminfo_queue.empty():
                meminfo_tuple = self.meminfo_queue.get()
                meminfo_dict = {}
                meminfo_dict["Time"] = meminfo_tuple[0].strftime("%Y-%m-%d %H-%M-%S")

                meminfo_dict = self.parse_meminfo(meminfo_tuple[1],meminfo_dict)
                change_counts += 1
                change_type = 3
                for i in range(len(change_types)):
                    if change_counts % change_types[i] == 0:
                        change_type = i
                        break

                self.configApp.start_chart(3 - change_type,meminfo_dict["MemAvailable"]/1024)
                self.write_to_excel(self.filename, meminfo_dict)
                # with open(f"data/meminfo/meminfo-{self.time_str}.txt", "a") as f:
                #     f.write(meminfo_tuple[1])
            time.sleep(1)

    def parse_meminfo(self, meminfo_str, meminfo_dict):
        """
        解析 meminfo 字符串，返回一个字典，其中键为字段名称，值为对应的数值。
        """
        # meminfo_dict = {}
        lines = meminfo_str.strip().splitlines()
        for line in lines:
            match = re.match(r'(\w+):\s+(\d+)', line)
            if match:
                key = match.group(1)
                value = int(match.group(2))
                meminfo_dict[key] = value
        return meminfo_dict

    def write_to_excel(self, filename, meminfo_dict):
        """
        将 meminfo 字典写入 Excel 文件，第一列为时间戳，后面每列为 meminfo 的每一个字段。
        """
        # 如果文件不存在，则创建一个新的工作簿
        try:
            workbook = openpyxl.load_workbook(filename)
            sheet = workbook.active
        except FileNotFoundError:
            workbook = openpyxl.Workbook()
            sheet = workbook.active
            # 写入标题行
            sheet.append(list(meminfo_dict.keys()))

        sheet.append(list(meminfo_dict.values()))

        # 保存工作簿
        workbook.save(filename)


    def get_meminfo(self):
        self.logger.info(f"send cmd")
        self.shell.send("cat /proc/meminfo\n")
        time.sleep(1)
        self.logger.info(f"recv cmd")
        if self.shell.recv_ready():
            output = self.shell.recv(self.max_receive_length)
            #output = trim_binary_data(output)
            output = output.decode('utf-8')
            #self.logger.info(f"{output}")
            #print(output)
            # 获取当前时间并格式化为字符串
            current_time = datetime.now()
            meminfo_tuple = (current_time, output)
            # current_time_str = current_time.strftime("%H:%M:%S")
            # self.meminfo_queue.put(meminfo_tuple)
            #加上毫秒格式
            #current_time_str += f".{current_time.microsecond // 1000}"

            # 将每一行添加时间戳，并生成新的内容
            # output_with_timestamp = "\n".join([f"[{current_time_str}] {line}" for line in output.splitlines()])
            # #将接收到的数据写入到目录/data/meminfo/meminfo-time.txt文件中
            # with open(f"data/meminfo/meminfo-{self.time_str}.txt", "a") as f:
            #     f.write(output_with_timestamp)
            

            # for line in output.splitlines():
            #     #self.logger.info(f"{line}")
            #     if "MemAvailable" in line:
            #         self.configApp.start_chart(int(line.split()[1])/1024)
            #         return int(line.split()[1])  # 返回MemAvailable的值

            #self.meminfo.append(output)
            change_types = [1000,100,10]
            change_counts = 0
            meminfo_dict = {}
            meminfo_dict["Time"] = meminfo_tuple[0].strftime("%Y-%m-%d %H-%M-%S")

            meminfo_dict = self.parse_meminfo(meminfo_tuple[1],meminfo_dict)
            change_counts += 1
            change_type = 3
            for i in range(len(change_types)):
                if change_counts % change_types[i] == 0:
                    change_type = i
                    break

            self.configApp.start_chart(3 - change_type,meminfo_dict["MemAvailable"]/1024)
            self.write_to_excel(self.filename, meminfo_dict)

        # if self.is_running:
        #     self.timer = threading.Timer(self.interval, self.get_meminfo)
        #     self.timer.start()

        return None
    


    #关闭定时器
    def stop(self):
        self.is_running = False
    #     self.timer.cancel()
    #     #等待定时器结束
    #     self.timer.join()
        if self.scheduler.running:
            self.scheduler.shutdown(wait=False)  # 关闭调度器
        self.logger.info(f"stop")
        self.configApp.close_chart()
        # self.process_thread.join()




        
