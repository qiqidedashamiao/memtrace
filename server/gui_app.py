
import json
import logging
import tkinter as tk
from tkinter import messagebox
from tkinter import ttk

from ssh import SSHConnection

class ConfigApp:
    m_config_data = {}
    m_ssh_memory = None
    def __init__(self, root):
        self.load_config()
        self.root = root
        self.root.title("tool")

        self.root.geometry("800x600")

        # 在窗口关闭事件时，调用on_closing函数
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)

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

        # 创建标签
        tk.Label(root, text="内存信息").grid(row=0, column=0, sticky="w")
        tk.Label(root, text="CPU信息").grid(row=1, column=0, sticky="w")
        tk.Label(root, text="磁盘使用情况").grid(row=2, column=0, sticky="w")

        # 创建复选框
        self.meminfo_var = tk.BooleanVar()
        self.cpuinfo_var = tk.BooleanVar()
        self.diskinfo_var = tk.BooleanVar()

        tk.Checkbutton(root, text="启用", variable=self.meminfo_var).grid(row=0, column=1)
        tk.Checkbutton(root, text="启用", variable=self.cpuinfo_var).grid(row=1, column=1)
        tk.Checkbutton(root, text="启用", variable=self.diskinfo_var).grid(row=2, column=1)

        # 创建配置按钮
        tk.Button(root, text="保存配置", command=self.save_config).grid(row=3, column=0, pady=10)
        tk.Button(root, text="退出", command=root.quit).grid(row=3, column=1, pady=10)

    def on_closing(self):
        # 关闭SSH连接
        if self.m_ssh_memory is not None:
            self.m_ssh_memory.close()
        # 关闭Tkinter窗口
        self.root.destroy()

    def on_memory_start(self):
        if self.m_ssh_memory is not None:
            messagebox.showinfo("内存变化", "功能已经开启，请先停止再启动")
        else:
            messagebox.showinfo("内存变化", "内存变化启动")
            self.m_ssh_memory = SSHConnection(self.m_config_data["device"])

    def on_memory_stop(self):
        messagebox.showinfo("内存变化", "内存变化停止")
        if self.m_ssh_memory is not None:
            self.m_ssh_memory.close()
            self.m_ssh_memory = None

    def load_config(self):
        """从配置文件加载内容"""
        try:
            with open("config.json", "r") as config_file:
                self.m_config_data = json.load(config_file)
        except FileNotFoundError:
            # 如果文件不存在，使用默认值
            self.m_config_data = {
                "device": {
                "host": "",
                "username": "",
                "password": "",
                "interval": 1
                },
                "server": {
                "ip": "",
                "username": "",
                "password": "",
                "cross": ""
                }
            }

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

        # 创建“设备配置”子界面
        device_frame = ttk.Frame(notebook)
        notebook.add(device_frame, text="设备配置")

        # 使用 lambda 表达式配置列宽度和行高
        configure_grid = lambda frame: (
            frame.grid_columnconfigure(0, minsize=500 * 2 / 5),  # 设置第一列的最小宽度
            frame.grid_columnconfigure(1, weight=1),             # 第二列可以扩展
            frame.grid_rowconfigure(0, minsize=500 * 1 / 3)      # 设置第一行高度
        )

        # 配置列宽度，以确保控件的左边距
        # device_frame.grid_columnconfigure(0, minsize=500 * 2 / 5)  # 将第一列的最小宽度设置为100像素
        # device_frame.grid_columnconfigure(1, weight=1)     # 第二列可以扩展
        
        # device_frame.grid_rowconfigure(0, minsize=500*1/3)   # 设置第一行高度为100
        configure_grid(device_frame)
        
        #在“设备配置”界面中添加控件
        devicetextlist = ["IP:", "用户名:", "密码:", "采样时间:"]
        devicename = ["host", "username", "password", "interval"]
        device_entry = {}  # Define the device_entry dictionary
        device_key = "device"
        for i in range(len(devicetextlist)):
            ttk.Label(device_frame, text=devicetextlist[i], font=font_large).grid(row=i, column=0, padx=10, pady=10, sticky="se")
            device_entry[i] = ttk.Entry(device_frame, font=font_large)
            device_entry[i].grid(row=i, column=1, padx=10, pady=10, sticky="sw")  # Use device_entry[i] instead of device_ip_entry
            if devicename[i] in self.m_config_data[device_key]:
                device_entry[i].insert(0, self.m_config_data[device_key].get(devicename[i]))  # Set default value
        
        # 创建保存配置按钮，将IP、用户名、密码、采样时间保存到配置文件
        ttk.Button(device_frame, text="保存", command=lambda: self.save_config_device(device_entry,config_window,devicename,device_key), style="TButton").grid(row=4, column=0, padx=10, pady=10, sticky="se")
        #ttk.Button(device_frame, text="刷新", command=self.reload).grid(row=3, column=1, pady=10)
         # 添加退出按钮
        ttk.Button(device_frame, text="退出", command=lambda: self.close_config_window(config_window), style="TButton").grid(row=4, column=1, padx=100, pady=10, sticky="sw")

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
                server_entry[i].insert(0, self.m_config_data[server_key].get(servername[i], ""))

        
        # 创建保存配置按钮，将IP、用户名、密码、采样时间保存到配置文件
        ttk.Button(server_frame, text="保存", command=lambda: self.save_config_device(server_entry,config_window,servername,server_key), style="TButton").grid(row=4, column=0, padx=10, pady=10, sticky="se")
        #ttk.Button(device_frame, text="刷新", command=self.reload).grid(row=3, column=1, pady=10)
         # 添加退出按钮
        ttk.Button(server_frame, text="退出", command=lambda: self.close_config_window(config_window), style="TButton").grid(row=4, column=1, padx=100, pady=10, sticky="sw")

        # 将Notebook添加到配置窗口
        notebook.pack(expand=True, fill="both")

        #messagebox.showinfo("配置选项", "配置选项被点击")
    
    def save_config_device(self, device_entry, config_window,name,key):
        """保存输入内容到配置文件"""
        # config_data = {
        #     "device_ip": device_entry[0].get(),
        #     "device_username": device_entry[1].get(),
        #     "device_password": device_entry[2].get(),
        #     "device_interval": device_entry[3].get()
        # }
        config_data = {}
        config_data[key] = {name[i]: device_entry[i].get() for i in range(len(device_entry))}
        logging.info(f"config_data: {config_data}")

        self.m_config_data.update(config_data)
        #logging.info(f"m_config_data: {self.m_config_data}")

        with open("config.json", "w") as config_file:
            json.dump(self.m_config_data, config_file, indent=4)
        
        # 展示配置结果，这里可以修改为弹窗提示
        messagebox.showinfo("配置保存",config_data)

        # 提升配置界面到最前面
        config_window.lift()
        config_window.focus_force()  # 确保窗口获得焦点

    def close_config_window(self, config_window):
        """关闭配置窗口并返回主界面"""
        config_window.destroy()


    def on_tool_option(self):
        messagebox.showinfo("工具选项", "工具选项被点击")

    def save_config(self):
        # 处理用户配置
        config = {
            "meminfo_enabled": self.meminfo_var.get(),
            "cpuinfo_enabled": self.cpuinfo_var.get(),
            "diskinfo_enabled": self.diskinfo_var.get(),
        }

        # 展示配置结果
        config_message = (
            f"内存信息启用: {config['meminfo_enabled']}\n"
            f"CPU信息启用: {config['cpuinfo_enabled']}\n"
            f"磁盘使用情况启用: {config['diskinfo_enabled']}"
        )
        messagebox.showinfo("配置保存", config_message)

        # 在这里可以添加保存配置到文件或应用配置到系统的逻辑
        # 示例：
        # with open("config.txt", "w") as f:
        #     f.write(str(config))
