import socket
import threading
from tkinter import messagebox


class MemStatProcess:
    def __init__(self, app):
        self.app = app
        self.logger = app.get_logger()
        self.sock = None
        pass
    def __del__(self):
        pass
    def start(self):
        self.logger.info("start")
        if self.sock:
            self.sock.close()
        return self.connect()
        pass

    def stop(self):
        if self.sock:
            self.sock.close()

    def connect(self):
        self.logger.info("connect start")
        config = self.app.get_config_data()
        host = config["server"].get('host', '')
        port = config["server"].get('port', 65511)
        if host == '' or port == 0:
            messagebox.showinfo("提示", "请先配置服务器地址和端口")
            return False
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.bind((host, port))
            self.sock.listen()
            self.logger.info(f"Server listening on {host}:{port}")
            self.thread_listen = threading.Thread(target=self.listen)
            self.thread_listen.start()
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
            messagebox.showinfo("提示", f"监听失败:{e}")
            self.logger.error(f"监听失败:{e}")
            return False
        
    def listen(self):
        self.logger.info("listen start")
        try:
            while True:
                conn, addr = self.sock.accept()
                thread = threading.Thread(target=self.handle_client, args=(conn, addr))
                thread.start()
        except Exception as e:
            self.logger.error(f"监听失败:{e}")
            messagebox.showinfo("提示", f"监听失败:{e}")
    
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
            messagebox.showinfo("提示", f"处理客户端请求失败:{e}")
        finally:
            conn.close()
