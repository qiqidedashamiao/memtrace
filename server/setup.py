from cx_Freeze import setup, Executable

# 指定主脚本和依赖
executables = [Executable("main.py")]

# 设置打包选项
setup(
    name="MyApp",
    version="0.1",
    description="My Python Application",
    executables=executables
)
