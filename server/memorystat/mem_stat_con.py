import asyncio
from csv import reader, writer
from email.utils import parsedate
from enum import Enum
import pdb
import queue
import select
import socket
import struct
import threading
import concurrent.futures

class ENUM_MEMOPTYPE(Enum):
    MEMOP_FREE = 0
    MEMOP_DELETE = 1
    MEMOP_DELETE_ARRAY = 2
    MEMOP_MALLOC = 3
    MEMOP_CALLOC = 4
    MEMOP_NEW = 5
    MEMOP_NEW_NOTHROW = 6
    MEMOP_NEW_ARRAY = 7
    MEMOP_REALLOC = 8
    MEMOP_MALLOC_BIG = 9
    MEMOP_CALLOC_BIG = 10
    MEMOP_NEW_BIG = 11
    MEMOP_NEW_NOTHROW_BIG = 12
    MEMOP_NEW_ARRAY_BIG = 13
    MEMOP_REALLOC_BIG = 14
    MEMOP_MAX = 15
    MEMOP_MAP = 16

class MemStatCon:
    def __init__(self, app):
        self.logger = app.get_logger()
        self.active_connections = []
        self.thread_consumer = None
        self.queue_data = queue.Queue()
        self.maps = MemStatMaps(app)
        pass

    def start(self, app):
        self.logger.info("run start")
        # pdb.set_trace()
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
        self.loop.close()
        self.server = None
        self.logger.info("conn loop stop")
        self.thread_consumer.join()
        self.thread_consumer = None
        self.logger.info(f"queue_data size: {self.queue_data.qsize()}")
        self.logger.info("conn stop end")
    
    async def handle_client(self, reader, writer):
        addr = writer.get_extra_info('peername')
        self.logger.info(f"Connection from {addr}")
        # 添加到活动连接列表
        self.active_connections.append((reader, writer))
        self.thread_consumer = threading.Thread(target=self.consumer, args=(reader, writer, addr))
        self.thread_consumer.start()
        try:
            while True:
                data = await reader.read(1)

                if not data:
                    break
                # self.logger.info(f"Received {data} from {addr}")
                data_type = struct.unpack('<b', data)[0]
                parse_date = {}
                parse_date['type'] = data_type
                
                self.logger.info(f"Received {data_type} from {addr}")

                if data_type == ENUM_MEMOPTYPE.MEMOP_MAP.value:
                    self.logger.info(f"type is MEMOP_MAP")
                    data = await reader.read(4)
                    self.logger.info(f"Received {data} from {addr}")
                    data_len = struct.unpack('<I', data)[0]
                    parse_date['len'] = data_len

                    if data_len == 0:
                        self.logger.info(f"Received error {data_len} from {addr}")
                        break
                    self.logger.info(f"Received {data_len} from {addr}")
                    data = await reader.read(data_len)
                    parse_date['maps'] = data.decode()
                    self.queue_data.put(parse_date)
                    # self.logger.info(f"parse_date:{parse_date}")
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
            self.queue_data.put(None)
            # 从活动连接列表中移除
            self.active_connections.remove((reader, writer))
            writer.close()
            await writer.wait_closed()
            self.logger.info(f"Connection closed from {addr}")

    def consumer(self, reader, writer, addr):
        self.logger.info(f"Consumer started for {addr}")
        while True:
            try:
                data = self.queue_data.get()
                # self.logger.info(f"Received data: {data}")
                if data is None:
                    self.logger.info(f"Received None from {addr}")
                    break
                if data['type'] == ENUM_MEMOPTYPE.MEMOP_MAP.value:
                    self.maps.parse_maps_data(data)
                # self.queue_data.task_done()
            except Exception as e:
                self.logger.error(f"Exception in consumer: {e}")
        self.logger.info(f"Consumer finished for {addr}")

#解析接收到进程的maps数据，并解析出每个模块的起始地址和大小
#00400000-00407000 r-xp 00000000 fd:00 42949                              /root/mount/share/memtrace/sonia
class MemStatMaps:
    def __init__(self, app):
        #key:end_addr, value:size
        self.maps = {}
        self.logger = app.get_logger()
        pass

    def parse_maps_data(self, data):
        self.maps.clear()
        maps = data['maps']
        maps_list = maps.split("\n")
        for map in maps_list:
            content = {}
            map_list = map.split(" ")
            if len(map_list) < 6:
                continue
            addr = map_list[0]
            addr_list = addr.split("-")
            key = int(addr_list[1], 16)
            content["start_addr"] = addr_list[0]
            content["end_addr"] = addr_list[1]
            content["name"] = map_list[-1].split("/")[-1]
            if len(content["name"]) == 0 or '[' in content["name"]:
                continue
            self.maps[key] = content
            self.logger.info(f"key:0x{key:08x} content:{content}")
        # self.logger.info(f"maps:{self.maps}")


