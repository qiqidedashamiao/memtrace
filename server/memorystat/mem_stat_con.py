import asyncio
from csv import reader, writer
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
        self.logger.info("conn start")
        asyncio.run(self.run(app))
        self.logger.info("conn end")
        return True

    async def run(self, app):
        self.logger.info("run start")
        # pdb.set_trace()
        # print("start1")
        config = app.get_config_data()
        host = config["server"].get('host', '')
        port = config["server"].get('port', 65511)
        self.server =  await asyncio.start_server(self.handle_client, host, port)
        addr = self.server.sockets[0].getsockname()
        self.logger.info(f'addr:{addr}')
        # try:
        #     self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        #     self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        #     self.server_socket.bind((host, port))
        #     self.server_socket.listen(1)
        #     self.server_socket.setblocking(False)  # 设置为非阻塞模式
        #     self.logger.info(f"Server listening on {host}:{port}")

        #     self.thread_listen = threading.Thread(target=self.handle_client)
        #     self.logger.info(f"start thread:{self.thread_listen}")
        #     self.thread_listen.start()
        #     # self.logger.info("start end")
        # except Exception as e:
        #     self.logger.error(f"Failed to start server on {host}:{port}: {e}")
        #     # print(f"Failed to start server on : {e}")
        #     return False
        # self.logger.info("start end")
        # print("start1 end")
        return True
        
    def stop(self):
        self.logger.info("conn stop")
        self.server.close()
        # await self.server.wait_closed()
        # if self.server_socket:
        #     self.logger.info("close start")
        #     self.server_socket.close()
        #     self.logger.info("close end")
        #     self.server_socket = None
        # self.logger.info("stop media")
        # if self.thread_listen:
        #     self.logger.info("join start")
        #     self.thread_listen.join()
        #     self.logger.info("join end")
        #     self.thread_listen = None
        self.logger.info("conn stop end")
    
    async def handle_client(self):
        # 用于存储输入和输出sockets
        self.logger.info("start handle_client")
        # print("start handle_client")
        # pdb.set_trace()
        try:
            # data = await reader.read(100)
            data = await asyncio.wait_for(reader.read(100),timeout=1)
            message = data.decode()
            addr = writer.get_extra_info('peername')
            self.logger.info(f"Received {message} from {addr}")
        except asyncio.TimeoutError:
            self.logger.info(f'oper time out')
        except Exception as e:
            self.logger.error(f"Failed to handle client: {e}")
        writer.close()
        self.logger.info("end handle_client")
