#增加一个命令类，输入参数为连接的shell对象，定时执行cat /proc/meminfo命令，获取内存信息
from datetime import datetime
import logging
import os
import sched
import threading
import time
from apscheduler.schedulers.background import BackgroundScheduler
from apscheduler.triggers.interval import IntervalTrigger
from matplotlib import animation, pyplot as plt

class MemInfoCmd:
    def __init__(self,device_config,configApp):
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
        with open(f"data/meminfo/meminfo-{self.time_str}.txt", "a") as f:
            pass
        self.shell = None
        self.interval = 5   #默认5s
        if "device_interval" in device_config:
            self.interval = device_config["device"]["device_interval"]
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
        # self.is_running = True

        self.scheduler.start()
        self.scheduler.add_job(self.get_meminfo, IntervalTrigger(seconds=self.interval))


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
            current_time_str = current_time.strftime("%H:%M:%S")
            #加上毫秒格式
            #current_time_str += f".{current_time.microsecond // 1000}"

            # 将每一行添加时间戳，并生成新的内容
            output_with_timestamp = "\n".join([f"[{current_time_str}] {line}" for line in output.splitlines()])
            #将接收到的数据写入到目录/data/meminfo/meminfo-time.txt文件中
            with open(f"data/meminfo/meminfo-{self.time_str}.txt", "a") as f:
                f.write(output_with_timestamp)
            

            for line in output.splitlines():
                #self.logger.info(f"{line}")
                if "MemAvailable" in line:
                    self.configApp.start_chart(int(line.split()[1])/1024)
                    return int(line.split()[1])  # 返回MemAvailable的值

            #self.meminfo.append(output)

        # if self.is_running:
        #     self.timer = threading.Timer(self.interval, self.get_meminfo)
        #     self.timer.start()

        return None

    #关闭定时器
    def stop(self):
    #     self.is_running = False
    #     self.timer.cancel()
    #     #等待定时器结束
    #     self.timer.join()
        if self.scheduler.running:
            self.scheduler.shutdown(wait=False)  # 关闭调度器
        self.logger.info(f"stop")
        self.configApp.close_chart()



        
