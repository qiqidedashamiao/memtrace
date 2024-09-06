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

        # self.logger.info("close socket")
        # if self.thread_client is not None:
        #     self.logger.info("start stop client")
            # sleep(1)   # 等待客户端线程结束
            # self.thread_client.join()
            # self.thread_client = None
        # self.logger.info("stop client")
        # if self.thread_listen is not None:
        #     self.logger.info("start stop listen")
        #     self.thread_listen.join()
        #     self.thread_listen = None
        # # time.sleep(5)
        # self.logger.info("stop listen")
        self.logger.info("stop end")


    def connect(self):
        self.logger.info("connect start")
        config = self.app.get_config_data()
        host = config["server"].get('host', socket.gethostbyname(socket.gethostname()))
        port = config["server"].get('port', 65511)
        if host == '' or port == 0:
            messagebox.showinfo("提示", "请先配置服务器地址和端口")
            return False
        try:
            self.server = socketserver.ThreadingTCPServer((host, port), MemStatHandler)
            #判断self.server是否连接成功
            self.logger.info(f"Server socket :{self.server.socket}")
            self.logger.info(f"Server listening on {host}:{port}")
            self.thread_listen = threading.Thread(target=self.listen)
            self.thread_listen.start()
            # self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # self.logger.info(f"Server socket :{self.sock}")
            # self.sock.bind((host, port))
            # self.sock.listen()
            # self.logger.info(f"Server listening on {host}:{port}")
            # self.thread_listen = threading.Thread(target=self.listen)
            # self.thread_listen.start()
            return True
            # while True:
            #     conn, addr = self.sock.accept()
            #     thread = threading.Thread(target=self.handle_client, args=(conn, addr))
            #     thread.start()
            # with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            #     s.bind((host, port))
            #     s.listen()
            #     print(f"Server listening on {host}:{port}")
            #     while True:
            #         conn, addr = s.accept()
            #         thread = threading.Thread(target=self.handle_client, args=(conn, addr))
            #         thread.start()
        except Exception as e:
            # messagebox.showinfo("提示", f"连接失败:{e}")
            self.logger.error(f"连接失败:{e}")
            return False
        
    def listen(self):
        self.logger.info("listen start")
        
        self.server.serve_forever(poll_interval=1)

        # try:
        #     while self.sock:
        #         self.logger.info("listen")
        #         conn, addr = self.sock.accept()
        #         self.logger.info(f"Connected by {addr}")
        #         self.thread_client = threading.Thread(target=self.handle_client, args=(conn, addr))
        #         self.thread_client.start()
        # except Exception as e:
        #     self.logger.error(f"监听失败:{e}")
        #     # messagebox.showinfo("提示", f"监听失败:{e}")
        self.logger.info("listen end")
    
    def handle_client(self, conn, addr):
        self.logger.info(f"Connected by {addr}")
        try:
            while True:
                mem_type = conn.recv(1).decode()
                if not mem_type:
                    break
                if mem_type == 0:
                    pass
                self.logger.info(f"Received from bytes")
        except Exception as e:
            self.logger.error(f"处理客户端请求失败:{e}")
            # messagebox.showinfo("提示", f"处理客户端请求失败:{e}")
        finally:
            conn.close()

class MemStatHandler(socketserver.BaseRequestHandler):
    def handle(self):
        self.data = self.request.recv(1024).strip()
        print(f"{self.client_address[0]} wrote:")
        print(self.data)
        self.request.sendall(self.data.upper())
