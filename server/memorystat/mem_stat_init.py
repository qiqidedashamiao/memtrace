import pdb
from memorystat.mem_stat_cfg import MemStatCfg


def mem_stat_init(app,config_key,module_name):
    # pdb.set_trace()
    app.init_default_cfg(MemStatCfg.init_default_cfg())
    #增加调试断点
    # pdb.set_trace(
    MemStatCfg.init_config_gui(app)
    pass