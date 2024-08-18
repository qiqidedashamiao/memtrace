import io
import logging
import socket
import sys
import threading
from collections import defaultdict

import tkinter as tk

import struct

from gui_app import ConfigApp

# 定义结构体的格式字符串
# < 表示小端序，b 表示 int8，h 表示 int16，I 表示 unsigned int，q 表示 64 位指针 (void*)
# 结构体总大小 = 1 + 1 + 2 + 4 + 4 + 8 + 8 + 8 + (8 * 5) = 76 bytes
#struct_format = '<bbhIiqqq5q'
#结构体总大小 = 1 + 1 + 2 + 4 + 4 + 8 + 8 + 8 = 36 bytes
struct_format = '<bbhIiqqq'
# 定义结构格式
#struct_format = 'I 4s f'  # 例如：一个无符号整数，一个4字节字符串，一个浮点数
struct_size = struct.calcsize(struct_format)
#存储struct_format格式的内存记录
memory_records = []
# 打包数据
packed_data = b''.join([struct.pack(struct_format, *data) for data in data_list])
# 将打包的数据存储到文件中
with open('memory_records.bin', 'wb') as f:
    f.write(packed_data)

# 从文件中读取数据
with open('memory_records.bin', 'rb') as f:
    read_data = f.read()
# 解包数据
unpacked_data_list = [struct.unpack(struct_format, read_data[i:i+struct_size]) for i in range(0, len(read_data), struct_size)]



# 存储内存申请记录的字典，键为线程ID，值为(总大小, 次数)
memory_records = defaultdict(lambda: (0, 0))

def parse_mem_log_info(data):
    # 使用 struct.unpack 解包数据
    mem_log_info = struct.unpack(struct_format, data)

    # 构建一个字典存储解包的数据
    parsed_data = {
        'type': mem_log_info[0],
        'dep': mem_log_info[1],
        'tid': mem_log_info[2],
        'currtime': mem_log_info[3],
        'size': mem_log_info[4],
        'ptr': mem_log_info[5],
        'ptrlr': mem_log_info[6],
        'ptrx': mem_log_info[7],
        'spinfo': mem_log_info[8:]
    }
    return parsed_data

# 示例数据（假设你已经接收到了 56 字节的数据）
data = b'\x01\x02\x34\x12\x78\x56\x34\x12\x9A\xBC\xDE\xF0\x08\x00\x00\x00' \
       b'\x10\x20\x30\x40\x50\x60\x70\x80\x10\x20\x30\x40\x50\x60\x70\x80' \
       b'\x10\x20\x30\x40'

parsed_data = parse_mem_log_info(data)
print(parsed_data)


def handle_client(conn, addr):
    print(f"Connected by {addr}")
    try:
        while True:
            data = conn.recv(1024).decode()
            if not data:
                break
            # 假设数据格式为 "thread_id,size"
            thread_id, size = data.split(',')
            size = int(size)
            memory_records[thread_id] = (memory_records[thread_id][0] + size, memory_records[thread_id][1] + 1)
            print(f"Received from {thread_id}: {size} bytes")
    finally:
        conn.close()

def start_server(host='127.0.0.1', port=12345):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((host, port))
        s.listen()
        print(f"Server listening on {host}:{port}")
        while True:
            conn, addr = s.accept()
            thread = threading.Thread(target=handle_client, args=(conn, addr))
            thread.start()