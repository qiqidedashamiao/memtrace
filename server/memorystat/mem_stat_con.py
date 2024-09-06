import pdb
import select
import socket
import threading


class MemStatCon:
    def __init__(self, app):
        self.server_socket = None
        self.logger = app.get_logger()
        self.thread_listen = None
        pass
    
    def start(self, app):
        self.logger.info("start")
        pdb.set_trace()
        # print("start1")
        config = app.get_config_data()
        host = config["server"].get('host', '')
        port = config["server"].get('port', 65511)
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind((host, port))
            self.server_socket.listen(1)
            self.server_socket.setblocking(False)  # 设置为非阻塞模式
            self.logger.info(f"Server listening on {host}:{port}")

            self.thread_listen = threading.Thread(target=self.handle_client)
            self.logger.info(f"start thread:{self.thread_listen}")
            self.thread_listen.start()
            # self.logger.info("start end")
        except Exception as e:
            self.logger.error(f"Failed to start server on {host}:{port}: {e}")
            # print(f"Failed to start server on : {e}")
            return False
        self.logger.info("start end")
        # print("start1 end")
        return True
        
    def stop(self):
        self.logger.info("stop")
        if self.server_socket:
            self.logger.info("close start")
            self.server_socket.close()
            self.logger.info("close end")
            self.server_socket = None
        self.logger.info("stop media")
        if self.thread_listen:
            self.logger.info("join start")
            self.thread_listen.join()
            self.logger.info("join end")
            self.thread_listen = None
        self.logger.info("stop end")
        # print("stop1")
        # if self.server_socket:
        #     self.server_socket.close()
        #     self.server_socket = None
        # print("stop1 media")
        # if self.thread_listen:
        #     print("join start")
        #     self.thread_listen.join()
        #     print("join end")
        #     self.thread_listen = None
        # print("stop1 end")
    
    def handle_client(self):
        # 用于存储输入和输出sockets
        self.logger.info("start handle_client")
        # print("start handle_client")
        # pdb.set_trace()
        inputs = [self.server_socket]
        outputs = []
        try:
            while self.server_socket and self.server_socket.fileno() != -1:
                self.logger.info("start select")
                # 使用select来监听sockets
                readable, writable, exceptional = select.select(inputs, outputs, inputs,1)
                self.logger.info("end select")
                for s in readable:
                    if s is self.server_socket:
                        # 处理新的连接
                        connection, client_address = s.accept()
                        self.logger.info(f"Connection from {client_address}")
                        connection.setblocking(False)
                        inputs.append(connection)
                    else:
                        # 处理客户端发送的数据
                        data = s.recv(1024)
                        if data:
                            # 有数据，可以进行处理
                            self.logger.info(f"Received {data} from {s.getpeername()}")
                            s.send(data)  # 发送回客户端
                        else:
                            # 没有数据，表示客户端关闭了连接
                            self.logger.info(f"Closing connection to {s.getpeername()}")
                            inputs.remove(s)
                            s.close()

                for s in exceptional:
                    self.logger.info(f"Handling exceptional condition for {s.getpeername()}")
                    # 移除并关闭有异常的socket
                    inputs.remove(s)
                    if s.fileno() != -1:
                        s.close()
                self.logger.info("end loop")
        except Exception as e:
            self.logger.error(f"Failed to handle client: {e}")
        finally:
            # 清理所有socket连接
            for s in inputs:
                if s and s.fileno() != -1:
                    s.close()
            # self.server_socket = None
        self.logger.info("end handle_client")
