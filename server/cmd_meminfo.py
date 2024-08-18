#增加一个命令类，输入参数为连接的shell对象，定时执行cat /proc/meminfo命令，获取内存信息
import logging
import sched
import threading
import time
from apscheduler.schedulers.background import BackgroundScheduler
from apscheduler.triggers.interval import IntervalTrigger

class MemInfoCmd:
    def __init__(self):
        logging.info(f"init MemInfoCmd")
        self.shell = None
        self.interval = 1
        self.meminfo = []

        # self.timer = None
        # self.is_running = False

        self.scheduler = BackgroundScheduler()

    def start(self, shell):
        logging.info(f"start")
        self.shell = shell

        # self.timer = threading.Timer(self.interval, self.get_meminfo)
        # self.timer.start()
        # self.is_running = True

        self.scheduler.start()
        self.scheduler.add_job(self.get_meminfo, IntervalTrigger(seconds=self.interval))

    def get_meminfo(self):
        logging.info(f"send cmd")
        self.shell.send("cat /proc/meminfo\n")
        time.sleep(1)
        if self.shell.recv_ready():
            output = self.shell.recv(8192)
            #output = trim_binary_data(output)
            output = output.decode('utf-8')
            logging.info(f"{output}")
            #print(output)
            self.meminfo.append(output)

        # if self.is_running:
        #     self.timer = threading.Timer(self.interval, self.get_meminfo)
        #     self.timer.start()

    #关闭定时器
    def stop(self):
    #     self.is_running = False
    #     self.timer.cancel()
    #     #等待定时器结束
    #     self.timer.join()
        if self.scheduler.running:
            self.scheduler.shutdown()  # 关闭调度器

        
