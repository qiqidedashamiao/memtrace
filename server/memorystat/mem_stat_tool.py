from tkinter import messagebox, ttk, Menu
import tkinter

from memorystat.mem_stat_process import MemStatProcess


class MemStatTool:
    def __init__(self, app):
        self.app = app
        self.__running = False
        self.logger = app.get_logger()
        pass

    def init_tool_gui(self, app):
        tool_menu = app.get_tool_menu()
        # 创建“内存统计”子菜单
        memory_change_menu = tkinter.Menu(tool_menu, tearoff=0)
        memory_change_menu.add_command(label="启动", command=lambda: self.__on_mem_stat_start(memory_change_menu))
        memory_change_menu.add_command(label="停止", command=lambda: self.__on_mem_stat_stop(memory_change_menu))

        # 将“内存变化”子菜单添加到“工具”菜单中
        tool_menu.add_cascade(label="内存统计", menu=memory_change_menu)

    def __on_mem_stat_start(self,memory_change_menu):
        if self.__running:
            # messagebox.showinfo("提示", "内存统计已经启动")
            self.logger.info(f"内存统计已经启动")
            return
        self.__running = True
        self.__process = MemStatProcess(self.app)
        self.logger.info(f"内存统计启动")
        if self.__process.start():
            # messagebox.showinfo("提示", "内存统计启动成功")
            self.logger.info(f"内存统计启动成功")
        else:
            messagebox.showinfo("提示", "内存统计启动失败")
            self.__running = False


    def __on_mem_stat_stop(self,memory_change_menu):
        if not self.__running:
            messagebox.showinfo("提示", "内存统计未启动")
            return
        self.__process.stop()
        self.__process = None
        self.__running = False
        pass







