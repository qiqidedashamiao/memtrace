#include <cstdio>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>

#include "maps.h"

Maps::Maps(const char * mapsPath, const char * execPath, const char* preCmd, bool aslr):m_execPath(execPath), m_preCmd(preCmd), m_bASLR(aslr)
{
    // 读取maps文件
    FILE *file = fopen(mapsPath, "r");
    if (file == NULL)
    {
        printf("open %s failed\n", mapsPath);
        return;
    }
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file))
    {
        std::string line(buffer);
        // 找到libmaps.so所在的内存区域
        if (line.find("/") != std::string::npos)
        {
            MemoryRegion region;
            char permission[32];
            char offset[32];
            char device[32];
            char inode[32];
            char pathname[32];
            // 解析maps文件的一行 map格式00400000-0040a000 r-xp 00000000 fd:00 42082                              /root/mount/share/memtrace/sonia
            sscanf(buffer, "%p-%p %s %s %s %s %s", &region.start, &region.end, permission, offset, device, inode, pathname);
            int index = line.find_last_of("/");
            int len = line.length() - index - 1 - 1;
            //printf("index %d len %d\n", index, len);
            region.execName = line.substr(index + 1, len);

            //去掉换行符
            //region.execName = region.execName.substr(0, region.execName.size() - 1);
            m_mapMemoryRegions[region.end] = region;
            // 打印内存区域信息
            //printf("%p-%p %s\n", region.start, region.end, region.execName.c_str());
        }
    }
    fclose(file);
}

bool Maps::findMemoryRegion(void *address, MemoryRegion &region) {
        auto it = m_mapMemoryRegions.lower_bound(address);
        

        if (it != m_mapMemoryRegions.end() && it->second.start <= address && it->second.end >= address) {
            region = it->second;
            if (!m_bASLR)
            {
                region.start = 0;
            }
            // 判断文件是否存在
            string name = m_execPath + region.execName;
            if (access(name.c_str(), F_OK) == 0)
            {
                return true;
            }

            return false;
        }
        return false;
    }

void Maps::addr2symbol(void * addr)
{
    MemoryRegion region;
    char symbolBuf[40960] = {0};
    if (findMemoryRegion(addr, region))
    {
        FILE * stream;
        char cmd[10240];

        unsigned long long offset = (unsigned long long)addr - (unsigned long long)region.start;
        //sprintf(cmd, "csky-abiv2-ux-linuxv3615-addr2line -a -e %s -f -C %p", soniaPath, addr);
        //sprintf(cmd, "aarch64-himix210-linux-sd3403v100-v1-addr2line -a -e %s -f -C %p", soniaPath, addr);
        sprintf(cmd, "%saddr2line -a -e %s%s -f -C %p", m_preCmd.c_str(), m_execPath.c_str(), region.execName.c_str(), offset);
        //printf("cmd %s\n", cmd);
        stream = popen(cmd, "r");
        fread(symbolBuf, sizeof(char), sizeof(symbolBuf), stream);
        pclose(stream);
        printf("%s", symbolBuf);
    }
    else
    {
        printf("%p\n", addr);
    }
}

void Maps::stat(void *address, int lrType)
{
    auto it = m_mapMemoryRegions.lower_bound(address);

    if (it != m_mapMemoryRegions.end() && it->second.start <= address && it->second.end >= address)
    {
        MemoryRegion &region = it->second;
        region.num[lrType]++;
    }
}

/**
 * @brief 用户打印输出m_mapMemoryRegions表里面的每个地址段对应的可执行文件，以及申请的内存次数
*/
void Maps::printStat()
{
    for (auto it = m_mapMemoryRegions.begin(); it != m_mapMemoryRegions.end(); it++)
    {
        printf("%s    %p-%p     add %lu      del_add %lu     del %lu\n", it->second.execName.c_str(), it->second.start, it->second.end, it->second.num[0], it->second.num[1], it->second.num[2]);
    }
}
