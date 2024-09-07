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

