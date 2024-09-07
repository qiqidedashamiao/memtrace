import asyncio
from csv import reader, writer
from email.utils import parsedate
import pdb
import select
import socket
import struct
import threading


class MemStatCon:
    def __init__(self, app):
        self.server_socket = None
        self.logger = app.get_logger()
        self.thread_listen = None
        self.active_connections = []
        pass

    def start(self, app):
        self.logger.info("run start")
        # pdb.set_trace()
        # print("start1")
        config = app.get_config_data()
        host = config["server"].get('host', '')
        port = config["server"].get('port', 65511)
        try:
            self.loop = asyncio.new_event_loop()
            asyncio.set_event_loop(self.loop)
            self.coro =  asyncio.start_server(self.handle_client, host, port)
            self.server = self.loop.run_until_complete(self.coro)
            addr = self.server.sockets[0].getsockname()
            self.logger.info(f'addr:{addr}')
        except Exception as e:
            self.logger.error(f"Exception in create client: {e}")
            return False
        
        self.thread = threading.Thread(target=self.loop.run_forever)
        self.thread.start()
        self.logger.info("run end")
        return True
        
    def stop(self):
        self.logger.info("conn stop")
        self.server.close()
        self.loop.call_soon_threadsafe(self.loop.stop)
        # 关闭所有活动连接
        for reader, writer in self.active_connections:
            writer.close()
            self.loop.run_until_complete(writer.wait_closed())
        self.thread.join()
        self.loop.run_until_complete(self.server.wait_closed())
        # self.loop.stop()
        # self.loop.run_forever()
        self.loop.close()
        self.logger.info("conn stop end")
    
    async def handle_client(self, reader, writer):
        addr = writer.get_extra_info('peername')
        self.logger.info(f"Connection from {addr}")
        # 添加到活动连接列表
        self.active_connections.append((reader, writer))
        try:
            while True:
                data = await reader.read(1)

                if not data:
                    break
                # self.logger.info(f"Received {data} from {addr}")
                parse_date = {}
                parse_date['type'] = data
                # parsed_data = {
                # 'type': mem_log_info[0],
                # 'len': mem_log_info[1],
                # 'tid': mem_log_info[2],
                # 'currtime': mem_log_info[3],
                # 'size': mem_log_info[4],
                # 'ptr': mem_log_info[5],
                # 'ptrlr': mem_log_info[6],
                # 'ptrx': mem_log_info[7],
                # 'spinfo': mem_log_info[8:]
                # }
                # struct_format = '<I'
                # struct_size = struct.calcsize(struct_format)

                if data == b"16":
                    data = await reader.read(4)
                    parse_date['len'] = struct.unpack('!I', data)[0]
                    if parse_date['len'] == 0:
                        self.logger.info(f"Received error {parse_date['len']} from {addr}")
                        break
                    parse_date['maps'] = await reader.read(parse_date['len']).decode()
                    self.logger.info(f"parse_date:{parse_date}")
                # elif data == b"2":
                #     data = await reader.readuntil(b"\n")
                #     self.logger.info(f"Received {data} from {addr}")
                # else:
                #     self.logger.error(f"Unknown data type {data_type}")

                # data_send = f"received {len(data)} bytes".encode()
                # writer.write(data_send)
                # await writer.drain()
        except Exception as e:
            self.logger.error(f"Exception in handle_client: {e}")
        finally:
            # 从活动连接列表中移除
            self.active_connections.remove((reader, writer))
            writer.close()
            await writer.wait_closed()
            self.logger.info(f"Connection closed from {addr}")
