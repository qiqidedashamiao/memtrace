import json
from tkinter import messagebox, ttk
import tkinter


class MemStatCfg:
    def __init__(self) -> None:
        self.__config = None

    def __del__(self):
        self.__config = None

    def set_config(config):
        self.config = config
    
    def get_config():
        return self.config
    
    @staticmethod
    def init_default_cfg():
        config = {
            "server": {
                "host": "",         # 主机地址
                "port": 65511,      # 主机端口
                "result_path":"",   # 内存统计结果路径
                "ip": "",           # 编译服务器ip
                "username": "",     # 编译服务器用户名
                "password": "",     # 编译服务器密码
                "num":45,           # 编译服务器镜像
                "cross": "",        # 设备交叉编译器
                "lib_path":"",      # 设备库路径
                "is_aslr":0,    # 是否开启aslr
                "is_backtrace":0    # 是否支持backtrace栈回溯
            }
        }
        return config
    
    @staticmethod
    def init_config_gui(app):
        config_memu = app.get_config_menu()
        config_memu.add_command(label="内存统计",command=lambda: MemStatCfg.on_configure_option(app))
        pass
    
    @staticmethod
    def on_configure_option(app):
        # 设置字体大小
        font_large = ("Arial", 12)  # 使用Arial字体，字体大小为12
        font_medium = ("Arial", 10) # 使用Arial字体，字体大小为10
        # 创建一个新的配置窗口
        config_window = tkinter.Toplevel(app.root)
        config_window.title("内存统计配置")
        config_window.geometry("500x500")

        style = ttk.Style()
        style.configure("TNotebook.Tab", font=("Arial", 12))  # 设置选项卡字体大小

        # 创建Notebook（选项卡容器）
        notebook = ttk.Notebook(config_window, style="TNotebook")
        
        # 使用 lambda 表达式配置列宽度和行高
        configure_grid = lambda frame: (
            frame.grid_columnconfigure(0, minsize=500 * 2 / 5),  # 设置第一列的最小宽度
            frame.grid_columnconfigure(1, weight=1),             # 第二列可以扩展
            frame.grid_rowconfigure(0, minsize=500 * 1 / 3)      # 设置第一行高度
        )

        # 创建“基础配置”子界面
        base_frame = ttk.Frame(notebook)
        notebook.add(base_frame, text="基础配置")

        # 配置列宽度，以确保控件的左边距
        # user_frame.grid_columnconfigure(0, minsize=500 * 2 / 5)  # 将第一列的最小宽度设置为100像素
        # user_frame.grid_columnconfigure(1, weight=1)     # 第二列可以扩展
        
        # user_frame.grid_rowconfigure(0, minsize=500*1/3)   # 设置第一行高度为100
        configure_grid(base_frame)
        
        #在“用户配置”界面中添加控件
        basetextlist = ["主机地址", "监听端口号:", "输出路径:"]
        basename = ["host", "port", "result_path"]
        base_entry = {}  # Define the device_entry dictionary
        base_key = "server"
        base_dict_int = {"port"}
        for i in range(len(basetextlist)):
            ttk.Label(base_frame, text=basetextlist[i], font=font_large).grid(row=i, column=0, padx=10, pady=10, sticky="se")
            base_entry[i] = ttk.Entry(base_frame, font=font_large)
            base_entry[i].grid(row=i, column=1, padx=10, pady=10, sticky="sw")  # Use device_entry[i] instead of device_ip_entry
            if basename[i] in app.m_config_data[base_key]:
                base_entry[i].insert(0, app.m_config_data[base_key].get(basename[i]))  # Set default value
        
        # 创建保存配置按钮，将IP、用户名、密码、采样时间保存到配置文件
        ttk.Button(base_frame, text="保存", command=lambda: MemStatCfg.save_config_device(app,base_entry,config_window,basename,base_key,base_dict_int), style="TButton").grid(row=4, column=0, padx=10, pady=10, sticky="se")
        #ttk.Button(base_frame, text="刷新", command=self.reload).grid(row=3, column=1, pady=10)
         # 添加退出按钮
        ttk.Button(base_frame, text="退出", command=lambda: MemStatCfg.close_config_window(config_window), style="TButton").grid(row=4, column=1, padx=100, pady=10, sticky="sw")

        # 创建“编译服务器配置”子界面
        server_frame = ttk.Frame(notebook)
        notebook.add(server_frame, text="解析配置")
        # server_frame = config_window

        # 配置列宽度，以确保控件的左边距
        # server_frame.grid_columnconfigure(0, minsize=500 * 2 / 5)  # 将第一列的最小宽度设置为100像素
        # server_frame.grid_columnconfigure(1, weight=1)     # 第二列可以扩展
        
        # server_frame.grid_rowconfigure(0, minsize=500*1/3)   # 设置第一行高度为100
       
        # configure_grid(server_frame)
        row_index = 0
        # 在“编译服务器配置”界面中添加控件
        servertextlist = ["IP:", "用户名:", "密码:", "镜像编号","交叉编译链:","库路径","支持ANSR","支持backtrace"]
        servername = ["ip", "username", "password","num", "cross","lib_path","is_aslr","is_backtrace"]
        server_entry = {}  # Define the server_entry dictionary
        server_key = "server"
        server_dict_int = {"num","is_aslr","is_backtrace"}
        for i in range(len(servertextlist)):
            ttk.Label(server_frame, text=servertextlist[i], font=font_large).grid(row=i, column=0, padx=10, pady=10, sticky="se")
            server_entry[i] = ttk.Entry(server_frame, font=font_large)
            server_entry[i].grid(row=i, column=1, padx=10, pady=10, sticky="sw")
            if servername[i] in app.m_config_data[server_key]:
                server_entry[i].insert(0, app.m_config_data[server_key].get(servername[i]))

        row_index = row_index + len(servertextlist)
        # 创建保存配置按钮，将IP、用户名、密码、采样时间保存到配置文件
        ttk.Button(server_frame, text="保存", command=lambda: MemStatCfg.save_config_device(app,server_entry,config_window,servername,server_key,server_dict_int), style="TButton").grid(row=row_index, column=0, padx=10, pady=10, sticky="se")
        #ttk.Button(device_frame, text="刷新", command=self.reload).grid(row=3, column=1, pady=10)
         # 添加退出按钮
        ttk.Button(server_frame, text="退出", command=lambda: MemStatCfg.close_config_window(config_window), style="TButton").grid(row=row_index, column=1, padx=100, pady=10, sticky="sw")

        # 将Notebook添加到配置窗口
        notebook.pack(expand=True, fill="both")

    @staticmethod
    def save_config_device(app,device_entry, config_window,name,key,dict_int):
        """保存输入内容到配置文件"""
        # config_data = {
        #     "device_ip": device_entry[0].get(),
        #     "device_username": device_entry[1].get(),
        #     "device_password": device_entry[2].get(),
        #     "device_interval": device_entry[3].get()
        # }
        config_data = {}
        config_data[key] = {name[i]: int(device_entry[i].get()) if name[i] in dict_int else device_entry[i].get() for i in range(len(device_entry))}
        app.logger.info(f"config_data: {config_data}")

        app.m_config_data.update(config_data)
        #logging.info(f"m_config_data: {self.m_config_data}")

        with open(app.m_config_filename, "w") as config_file:
            json.dump(app.m_config_data, config_file, indent=4)
        
        # 展示配置结果，这里可以修改为弹窗提示
        messagebox.showinfo("配置保存","保存成功")

        # 提升配置界面到最前面
        config_window.lift()
        config_window.focus_force()  # 确保窗口获得焦点

    @staticmethod
    def close_config_window(config_window):
        """关闭配置窗口并返回主界面"""
        config_window.destroy()