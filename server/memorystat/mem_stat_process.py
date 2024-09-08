import asyncio
import pdb
import socket
import socketserver
import threading
from time import sleep
import time
from tkinter import messagebox

from memorystat.mem_stat_con import MemStatCon


class MemStatProcess:
    def __init__(self, app):
        self.app = app
        self.logger = app.get_logger()
        self.sock = None
        self.thread_listen = None
        self.thread_client = None
        self.server = None
        self.conn = MemStatCon(app)
        
        pass
    def __del__(self):
        pass
    def start(self):
        self.logger.info("start0")
        #增加调试接口
        # pdb.set_trace()
        ret = self.conn.start(self.app)
        self.logger.info("start0 end")
        return ret

    def stop(self):
        self.logger.info("stop")
        self.conn.stop()

        self.logger.info("stop end")

#新建一个队列线程类，处理推送过来的队列数据
class MemStatThread(threading.Thread):
    def __init__(self, queue, app):
        threading.Thread.__init__(self)
        # self.queue = queue
        self.app = app
        self.logger = app.get_logger()
        self.is_running = True
        self.queue = asyncio.Queue()
        asyncio.create_task(self.run())
        pass

    async def run(self):
        self.logger.info("run")
        while self.is_running:
            try:
                # 从队列中获取数据
                data = self.queue.get(timeout=1)
                self.logger.info(f"get data:{data}")
            except Exception as e:
                self.logger.error(f"Exception in get data: {e}")
        self.logger.info("run end")

    def stop(self):
        self.is_running = False
        pass

