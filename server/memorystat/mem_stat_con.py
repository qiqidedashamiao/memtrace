import socket
import threading


class MemStatCon:
    def __init__(self):
        pass
    
    def start(self,config):
        host = config["server"].get('host', '')
        port = config["server"].get('port', 65511)
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.bind((host, port))
            s.listen()
            print(f"Server listening on {host}:{port}")
            while True:
                conn, addr = s.accept()
                thread = threading.Thread(target=handle_client, args=(conn, addr))
                thread.start()
    
    def handle_client(self,conn, addr):
        print(f"Connected by {addr}")
        try:
            while True:
                mem_type = conn.recv(1).decode()
                if not mem_type:
                    break
                if mem_type == MEMOP_MALLOC:
                    pass
                # 假设数据格式为 "thread_id,size"
                thread_id, size = data.split(',')
                size = int(size)
                memory_records[thread_id] = (memory_records[thread_id][0] + size, memory_records[thread_id][1] + 1)
                print(f"Received from {thread_id}: {size} bytes")
        finally:
            conn.close()