
import json
import logging
import pdb
import threading
import tkinter as tk
from tkinter import BooleanVar, Checkbutton, OptionMenu, Radiobutton, StringVar, messagebox
from tkinter import ttk
from tkinter import scrolledtext
from tkinter import font

from ssh import SSHConnection

class ConfigApp:
    m_config_data = {}
    m_ssh_memory = None
    def __init__(self, root):
         # 初始化绘图变量
        self.chart_window = None  # 折线图窗口
        self.is_running = False   # 用于控制图表更新的状态

        self.root = root
        self.root.title("tool")

        self.root.geometry("1000x800")

        # 在窗口关闭事件时，调用on_closing函数
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        
        # 设置字体为等宽字体，例如“Courier New”、"Noto Sans"、"DejaVu Sans Mono"、"Arial Unicode MS"
        # Courier New可以显示二维码
        font_obj = font.Font(family="Courier New", size=12)
         # 创建日志显示区域
        self.log_display = scrolledtext.ScrolledText(root, width=85, height=40, state='disabled', wrap='none')
        self.log_display.grid(row=0, column=0, padx=20, pady=10)
        self.log_display.configure(font=font_obj, bg='#f0f0f0', fg='black')  # 设置背景颜色

        #在ScrolledText下面增加水平滚动条
        h_scroll = tk.Scrollbar(root, orient=tk.HORIZONTAL, command=self.log_display.xview)
        h_scroll.grid(row=1, column=0, sticky='ew')
        # 绑定水平滚动条
        self.log_display.configure(xscrollcommand=h_scroll.set)
        # # 设置日志记录
        self.setup_logging()

        menu_bar = tk.Menu(root)
        menu_bar.grid_rowconfigure(0, minsize=20)  # 使菜单栏填充整个窗口
        #没发现用途
        #root.attributes('-disabled', False)  # 透明度 0.0 (全透明) 到 1.0 (不透明)
        # 设置主题
        self.style = ttk.Style()
        self.style.theme_use('classic')  # 'clam', 'alt', 'default', 'classic' 等
        # 设置字体
        self.style.configure('TButton', font=('Arial', 12), foreground='#333333')

        # 设置背景颜色
        root.configure(bg="#f0f0f0")
        root.attributes('-alpha', 0.99)  # 透明度 0.0 (全透明) 到 1.0 (不透明)

        # 创建“配置”菜单
        config_menu = tk.Menu(menu_bar, tearoff=0)
        config_menu.add_command(label="配置", command=self.on_configure_option)
        #config_menu.add_command(label="编译服务器配置", command=self.on_configure_option)
        config_menu.add_separator()  # 添加分割线
        config_menu.add_command(label="退出", command=root.quit)

        # 创建“工具”菜单
        tool_menu = tk.Menu(menu_bar, tearoff=0)

        # 创建“内存变化”子菜单
        memory_change_menu = tk.Menu(tool_menu, tearoff=0)
        memory_change_menu.add_command(label="启动", command=self.on_memory_start)
        memory_change_menu.add_command(label="停止", command=self.on_memory_stop)

        # 将“内存变化”子菜单添加到“工具”菜单中
        tool_menu.add_cascade(label="内存变化", menu=memory_change_menu)
        #tool_menu.add_command(label="内存变化", command=self.on_tool_option)
        tool_menu.add_command(label="内存使用", command=self.on_tool_option)

        # 将“配置”和“工具”菜单添加到菜单栏
        menu_bar.add_cascade(label="选项", menu=config_menu, font=("Arial", 14))
        menu_bar.add_cascade(label="工具", menu=tool_menu, font=("Arial", 14))
        #menu_bar.entryconfig("选项", padding=20)

        # 将菜单栏设置为主窗口的菜单
        root.config(menu=menu_bar)
        
        self.load_config()

    def start_chart(self, change_type, mem_available):
        """开始动态更新图表并打开新窗口""" 
        if self.chart_window is None or not self.is_running:
            # 创建一个新的窗口
            self.chart_window = tk.Toplevel(self.root)
            self.chart_window.title("Dynamic Memory Chart")
            self.chart_window.protocol("WM_DELETE_WINDOW", self.close_chart)

            # 创建 Canvas 组件
            self.canvas = tk.Canvas(self.chart_window, width=800, height=400, bg='white')
            self.canvas.grid(row=0, column=0, columnspan=4)
            # self.canvas.pack()

            self.show_interval = StringVar(value="1")
            # 创建单选下拉框
            intervals = ["1", "10", "100", "1000"]
            self.interval_menu = OptionMenu(self.chart_window, self.show_interval, *intervals, command=self.update_chart)
            self.interval_menu.grid(row=1, column=0, columnspan=4, pady=10)
            self.index_map = {"1": 0, "10": 1, "100": 2, "1000": 3}
            # 初始化绘图数据
            self.data = [[],[],[]]
            self.max_points = 20
            self.update_interval = 5000  # 5秒更新一次
            self.y_min = 7000
            self.y_max = 7300
            self.y_step = 50
            self.point_tags = [[],[],[]]  # 存储数据点的标签
            self.change_type = 0
            # 设置图表为运行状态
            self.is_running = True
            # 在 Canvas 上放置下拉框
            self.canvas.create_window(100, 50, window=self.interval_menu)
        
        self.logger.info(f"mem_available: {mem_available}, is_running: {self.is_running}, change_type: {change_type}")
        
        # pdb.set_trace()  # 设置断点
        for i in range(change_type):
            if len(self.data[i]) >= self.max_points:
                self.data[i].pop(0)
            self.data[i].append(mem_available)
            self.logger.info(f"self.data: {self.data[i]}")
        # intervals = ["1", "10", "100", "1000"]
        self.draw_chart()

    def close_chart(self):
        """关闭图表窗口"""
        self.is_running = False  # 停止图表更新
        if self.chart_window:
            self.chart_window.destroy()
            self.chart_window = None

    def update_chart(self, *args):
        """更新图表数据"""
        # if not self.is_running:
        #     return  # 如果不再运行，停止更新

        # 模拟接收内存信息（实际应从客户端接收数据）
        #mem_available = random.randint(100000, 200000)  # 模拟的内存值
        # 记录数据
        # for i in range(change_type):
        #     if len(self.data[i]) >= self.max_points:
        #         self.data[i].pop(0)
        #     self.data[i].append(mem_available)
        # if len(self.data) >= self.max_points:
        #     self.data.pop(0)
        # self.data.append(mem_available)

        # 绘制图表
        self.draw_chart()

        # # 定时更新
        # if self.is_running:
        #     self.chart_window.after(5000, self.update_chart)  # 每秒更新一次

    def draw_chart(self):
        """绘制折线图"""
        if not self.is_running:
            return  # 如果不再运行，停止更新
        self.canvas.delete("all")  # 清除之前的图形
        
        self.point_tags = []  # 每次更新清空点标签列表

        # 再次绘制下拉框（防止被清除）
        self.canvas.create_window(100, 50, window=self.interval_menu)

        # 计算数据点的坐标
        width = self.canvas.winfo_width()
        height = self.canvas.winfo_height()
        padding = 70

        # 根据选择的间隔调整显示
        index = self.index_map[self.show_interval.get()]
        self.logger.info(f"index: {index}")

        # 绘制坐标轴
        self.canvas.create_line(padding, padding, padding, height - padding, fill='black')
        self.canvas.create_line(padding, height - padding, width - padding, height - padding, fill='black')

        self.logger.info(f"self.data: {self.data[index]}")
        if len(self.data[index]) < 1:
            return

        # 绘制纵坐标刻度和说明
        for i in range(self.y_min, self.y_max + 1, self.y_step):
            y = height - padding - ((i - self.y_min) / (self.y_max - self.y_min)) * (height - 2 * padding)
            self.canvas.create_line(padding - 5, y, padding + 5, y, fill='black')
            self.canvas.create_text(padding - 30, y, text=f"{i} MB", fill='black')

        self.index_map = {0: 1, 1: 10, "100": 2, "1000": 3}

        # 绘制横坐标说明（时间）
        for i in range(0, self.max_points, 1):  # 每10个点一个刻度
            x = padding + i * (width - 2 * padding) / (self.max_points - 1)
            self.canvas.create_line(x, height - padding - 5, x, height - padding + 5, fill='black')
            #self.canvas.create_text(x, height - padding + 20, text=f"{i*5//60}:{i*5%60:02d}", fill='black')
            self.canvas.create_text(x, height - padding + 20, text=f"{i*(index+1)} s", fill='black')
        

        # 计算折线的坐标点
        # x_scale = (width - 2 * padding) / (self.max_points - 1)
        # y_scale = (height - 2 * padding) / (max(self.data) - min(self.data))
        x_scale = (width - 2 * padding) / (self.max_points - 1)
        y_scale = (height - 2 * padding) / (self.y_max - self.y_min)

        points = []
        for i, value in enumerate(self.data[index]):
            x = padding + i * x_scale
            y = height - padding - (value - self.y_min) * y_scale
            points.extend((x, y))
            # # 在每个点上显示具体的内存值
            # self.canvas.create_text(x, y - 10, text=f"{value:.2f} MB", fill='red', font=("Arial", 8))
            # 绘制不可见的点（用于绑定事件）
            point_id = self.canvas.create_oval(x - 3, y - 3, x + 3, y + 3, outline="", fill="")
            self.point_tags.append(point_id)
            # 绑定鼠标点击事件
            self.canvas.tag_bind(point_id, "<Button-1>", lambda event, v=value, x=x, y=y: self.show_value(event, v, x, y))

        # 绘制折线
        self.canvas.create_line(points, fill='blue')
    def show_value(self, event, value, x, y):
        # 显示点击点的值
        self.canvas.create_text(x, y - 10, text=f"{value:.2f} MB", fill='red', font=("Arial", 8))

    def setup_logging(self):
        """设置日志记录器，将日志输出到Text控件中"""
        self.logger = logging.getLogger("master")
        self.logger.setLevel(logging.INFO)

        # 创建处理器，将日志输出到Text控件中
        self.log_handler = logging.StreamHandler(self)
        self.log_handler.setLevel(logging.INFO)

        formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(filename)s - %(lineno)d - %(message)s')
        self.log_handler.setFormatter(formatter)

        self.logger.addHandler(self.log_handler)

    def write(self, message):
        """将日志写入Text控件"""
        self.log_display.configure(state='normal')
        self.log_display.insert(tk.END, message + '\n')
        self.log_display.configure(state='disabled')
        self.log_display.yview(tk.END)  # 自动滚动到最后一行
        self.flush()  # 强制刷新窗口显示

    def flush(self):
        self.log_display.update_idletasks()  # 强制刷新窗口显示
        pass  # 这个方法是处理器接口的一部分，通常可以不需要实现


    def on_closing(self):
        # 关闭SSH连接
        if self.m_ssh_memory is not None:
            self.m_ssh_memory.close()
        # 关闭Tkinter窗口
        self.root.destroy()

    def on_memory_start(self):
        if self.m_ssh_memory is not None and  self.m_ssh_memory.state is True:
            messagebox.showinfo("内存变化", "功能已经开启，请先停止再启动")
        else:
            if self.m_ssh_memory is not None:
                self.m_ssh_memory.close()
                self.m_ssh_memory = None
            self.m_ssh_memory = SSHConnection(self.m_config_data,self)

    def on_memory_stop(self):
        messagebox.showinfo("内存变化", "内存变化停止")
        if self.m_ssh_memory is not None:
            self.logger.info("关闭内存变化")
            self.m_ssh_memory.close()
            self.m_ssh_memory = None

    

    def update_dict_recursively(self, original, updates):
        for key, value in updates.items():
            if isinstance(value, dict) and key in original:
                self.update_dict_recursively(original[key], value)
            else:
                original[key] = value

    def load_config(self):
        """从配置文件加载内容"""
        self.m_config_data = {
            "device": {
                "host": "",
                "username": "admin",
                "password": "7ujMko0admin123",
                "interval": 1,
                # "dshell": True,
                "telnet": False
            },
            "server": {
                "ip": "",
                "username": "",
                "password": "",
                "cross": ""
            },
            "user": {
                "username": "234646",
                "password": ""
            },
            "max_receive_length": 32768
        }
        # self.logger.info(f"config_data: {self.m_config_data}")

        try:
            with open("config.json", "r") as config_file:
                # self.m_config_data.update(json.load(config_file))
                self.update_dict_recursively(self.m_config_data, json.load(config_file))
                # self.logger.info(f"config_data: {self.m_config_data}")
        except FileNotFoundError:
            # 如果文件不存在，使用默认值
            pass

        with open("config.json", "w") as config_file:
            json.dump(self.m_config_data, config_file, indent=4)
        self.logger.info(f"config_data: {self.m_config_data}")

    def on_configure_option(self):
        
        # 设置字体大小
        font_large = ("Arial", 12)  # 使用Arial字体，字体大小为12
        font_medium = ("Arial", 10) # 使用Arial字体，字体大小为10
        # 创建一个新的配置窗口
        config_window = tk.Toplevel(self.root)
        config_window.title("配置界面")
        config_window.geometry("500x500")

        #style = ttk.Style()
        self.style.configure("TNotebook.Tab", font=("Arial", 12))  # 设置选项卡字体大小

        # 创建Notebook（选项卡容器）
        notebook = ttk.Notebook(config_window, style="TNotebook")
        
        # 使用 lambda 表达式配置列宽度和行高
        configure_grid = lambda frame: (
            frame.grid_columnconfigure(0, minsize=500 * 2 / 5),  # 设置第一列的最小宽度
            frame.grid_columnconfigure(1, weight=1),             # 第二列可以扩展
            frame.grid_rowconfigure(0, minsize=500 * 1 / 3)      # 设置第一行高度
        )

        # 创建“用户配置”子界面
        user_frame = ttk.Frame(notebook)
        notebook.add(user_frame, text="用户配置")

        # 配置列宽度，以确保控件的左边距
        # user_frame.grid_columnconfigure(0, minsize=500 * 2 / 5)  # 将第一列的最小宽度设置为100像素
        # user_frame.grid_columnconfigure(1, weight=1)     # 第二列可以扩展
        
        # user_frame.grid_rowconfigure(0, minsize=500*1/3)   # 设置第一行高度为100
        configure_grid(user_frame)
        
        #在“用户配置”界面中添加控件
        usertextlist = ["用户名:", "密码:"]
        username = ["username", "password"]
        user_entry = {}  # Define the device_entry dictionary
        user_key = "user"
        for i in range(len(usertextlist)):
            ttk.Label(user_frame, text=usertextlist[i], font=font_large).grid(row=i, column=0, padx=10, pady=10, sticky="se")
            user_entry[i] = ttk.Entry(user_frame, font=font_large)
            user_entry[i].grid(row=i, column=1, padx=10, pady=10, sticky="sw")  # Use device_entry[i] instead of device_ip_entry
            if username[i] in self.m_config_data[user_key]:
                user_entry[i].insert(0, self.m_config_data[user_key].get(username[i]))  # Set default value
        
        # 创建保存配置按钮，将IP、用户名、密码、采样时间保存到配置文件
        ttk.Button(user_frame, text="保存", command=lambda: self.save_config_device(user_entry,config_window,username,user_key), style="TButton").grid(row=4, column=0, padx=10, pady=10, sticky="se")
        #ttk.Button(user_frame, text="刷新", command=self.reload).grid(row=3, column=1, pady=10)
         # 添加退出按钮
        ttk.Button(user_frame, text="退出", command=lambda: self.close_config_window(config_window), style="TButton").grid(row=4, column=1, padx=100, pady=10, sticky="sw")

        # 创建“设备配置”子界面
        device_frame = ttk.Frame(notebook)
        notebook.add(device_frame, text="设备配置")

        # 配置列宽度，以确保控件的左边距
        # device_frame.grid_columnconfigure(0, minsize=500 * 2 / 5)  # 将第一列的最小宽度设置为100像素
        # device_frame.grid_columnconfigure(1, weight=1)     # 第二列可以扩展
        
        # device_frame.grid_rowconfigure(0, minsize=500*1/3)   # 设置第一行高度为100
        configure_grid(device_frame)
        device_frame.grid_columnconfigure(0, minsize=500 * 2 / 5),  # 设置第一列的最小宽度
        device_frame.grid_columnconfigure(1, weight=1),             # 第二列可以扩展
        device_frame.grid_rowconfigure(0, minsize=500 * 1 / 5)      # 设置第一行高度
        
        #在“设备配置”界面中添加控件
        #增加连接方式的单选按钮 ssh/telnet
        #增加是否使用dshell的使能开关按钮

        # 创建用于存储单选按钮选择的变量
        selected_connection = StringVar(value="SSH")  # 默认选择SSH

        
        # 创建Telnet单选按钮
        telnet_radio = Radiobutton(device_frame, text="Telnet连接", variable=selected_connection, value="Telnet")
        telnet_radio.grid(row=0, column=0, sticky="se", padx=10, pady=10)

        # 创建SSH单选按钮
        ssh_radio = Radiobutton(device_frame, text="SSH连接", variable=selected_connection, value="SSH")
        ssh_radio.grid(row=0, column=1, sticky="se", padx=60, pady=10)


        # # 创建一个用于启用/禁用验证的复选框
        # verification_enabled = BooleanVar(value=True)  # 默认使能
        # verification_checkbox = Checkbutton(device_frame, text="DShell", variable=verification_enabled)
        # verification_checkbox.grid(row=0, column=2, sticky="se", padx=10, pady=10)

        # 创建一个按钮用于确认选择
        #connect_button = tk.Button(device_frame, text="Connect", command=self.on_connect_mode)
        #connect_button.grid(row=0, column=0, pady=10)

        devicetextlist = ["IP:", "用户名:", "密码:", "采样时间:"]
        devicename = ["connect_mode", "host", "username", "password", "interval"]
        device_entry = [selected_connection]  # Define the device_entry dictionary
        device_key = "device"
        index = len(device_entry)
        row_index = 1
        for i in range(len(devicetextlist)):
            ttk.Label(device_frame, text=devicetextlist[i], font=font_large).grid(row=i+row_index, column=0, padx=10, pady=10, sticky="se")
            device_entry.append(ttk.Entry(device_frame, font=font_large))
            # device_entry[i+index] = ttk.Entry(device_frame, font=font_large)
            device_entry[i+index].grid(row=i+row_index, column=1, padx=10, pady=10, sticky="sw")  # Use device_entry[i] instead of device_ip_entry
            if devicename[i+index] in self.m_config_data[device_key]:
                device_entry[i+index].insert(0, self.m_config_data[device_key].get(devicename[i+index]))  # Set default value
        
        row_index = row_index + len(devicetextlist)
        device_dict_int = {"interval"}
        # 创建保存配置按钮，将IP、用户名、密码、采样时间保存到配置文件
        ttk.Button(device_frame, text="保存", command=lambda: self.save_config_device(device_entry,config_window,devicename,device_key,device_dict_int), style="TButton").grid(row=row_index, column=0, padx=10, pady=10, sticky="se")

        #ttk.Button(device_frame, text="刷新", command=self.reload).grid(row=3, column=1, pady=10)
         # 添加退出按钮
        ttk.Button(device_frame, text="退出", command=lambda: self.close_config_window(config_window), style="TButton").grid(row=row_index, column=1, padx=60, pady=10, sticky="sw")
        row_index+=1

        # 创建“编译服务器配置”子界面
        server_frame = ttk.Frame(notebook)
        notebook.add(server_frame, text="编译服务器配置")

        # 配置列宽度，以确保控件的左边距
        # server_frame.grid_columnconfigure(0, minsize=500 * 2 / 5)  # 将第一列的最小宽度设置为100像素
        # server_frame.grid_columnconfigure(1, weight=1)     # 第二列可以扩展
        
        # server_frame.grid_rowconfigure(0, minsize=500*1/3)   # 设置第一行高度为100
        configure_grid(server_frame)

        # 在“编译服务器配置”界面中添加控件
        servertextlist = ["IP:", "用户名:", "密码:", "交叉编译链:"]
        servername = ["ip", "username", "password", "cross"]
        server_entry = {}  # Define the server_entry dictionary
        server_key = "server"
        for i in range(len(servertextlist)):
            ttk.Label(server_frame, text=servertextlist[i], font=font_large).grid(row=i, column=0, padx=10, pady=10, sticky="se")
            server_entry[i] = ttk.Entry(server_frame, font=font_large)
            server_entry[i].grid(row=i, column=1, padx=10, pady=10, sticky="sw")
            if servername[i] in self.m_config_data[server_key]:
                server_entry[i].insert(0, self.m_config_data[server_key].get(servername[i]))

        
        # 创建保存配置按钮，将IP、用户名、密码、采样时间保存到配置文件
        ttk.Button(server_frame, text="保存", command=lambda: self.save_config_device(server_entry,config_window,servername,server_key), style="TButton").grid(row=4, column=0, padx=10, pady=10, sticky="se")
        #ttk.Button(device_frame, text="刷新", command=self.reload).grid(row=3, column=1, pady=10)
         # 添加退出按钮
        ttk.Button(server_frame, text="退出", command=lambda: self.close_config_window(config_window), style="TButton").grid(row=4, column=1, padx=100, pady=10, sticky="sw")

        # 将Notebook添加到配置窗口
        notebook.pack(expand=True, fill="both")

        #messagebox.showinfo("配置选项", "配置选项被点击")
    
    def save_config_device(self, device_entry, config_window,name,key,dict_int):
        """保存输入内容到配置文件"""
        # config_data = {
        #     "device_ip": device_entry[0].get(),
        #     "device_username": device_entry[1].get(),
        #     "device_password": device_entry[2].get(),
        #     "device_interval": device_entry[3].get()
        # }
        config_data = {}
        config_data[key] = {name[i]: int(device_entry[i].get()) if name[i] in dict_int else device_entry[i].get() for i in range(len(device_entry))}
        self.logger.info(f"config_data: {config_data}")

        self.m_config_data.update(config_data)
        #logging.info(f"m_config_data: {self.m_config_data}")

        with open("config.json", "w") as config_file:
            json.dump(self.m_config_data, config_file, indent=4)
        
        # 展示配置结果，这里可以修改为弹窗提示
        messagebox.showinfo("配置保存","保存成功")

        # 提升配置界面到最前面
        config_window.lift()
        config_window.focus_force()  # 确保窗口获得焦点

    def close_config_window(self, config_window):
        """关闭配置窗口并返回主界面"""
        config_window.destroy()


    def on_tool_option(self):
        messagebox.showinfo("工具选项", "工具选项被点击")

