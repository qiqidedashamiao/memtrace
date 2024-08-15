#include <cstdio>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <dirent.h>

#include "maps.h"

// std::string formatPointerAddress(void* ptr) {
//     std::stringstream ss;
//     ss << "0x" << std::setw(BITLEN) << std::setfill('0') << std::hex << reinterpret_cast<uintptr_t>(ptr);
//     return ss.str();
// }

Maps::Maps():m_execPath(""), m_cross(""), m_bASLR(false)
{
    
}

void Maps::setMapsPath(const string mapsPath) 
{ 
    m_mapsPath = mapsPath; 
    // 获取mapsPath目录下的第一个文件的路径，并打开
    DIR *dir;
    struct dirent *entry;
    char filePath[1024];

    dir = opendir(mapsPath.c_str());
    if (dir == NULL) {
        perror("opendir");
        return ;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // 检查是否为常规文件
            snprintf(filePath, sizeof(filePath), "%s/%s", mapsPath.c_str(), entry->d_name);
            printf("First file: %s\n", filePath);
            break;
        }
    }
    closedir(dir);

    // 读取maps文件
    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        printf("open %s failed\n", mapsPath.c_str());
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
            char pathname[64];
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

// 判断地址是否在m_mapMemoryRegions表里面   
bool Maps::isExist(void *address) 
{
    auto it = m_mapMemoryRegions.lower_bound(address);
    if (it != m_mapMemoryRegions.end() && it->second.start <= address && it->second.end >= address) {

        return true;
    }
    return false;
}

void Maps::addr2symbol(void * addr)
{
    if (m_mapAddr.find(addr) != m_mapAddr.end())
    {
        printf("%s", m_mapAddr[addr].c_str());
        return;
    }
    MemoryRegion region;
    char symbolBuf[40960] = {0};
    if (findMemoryRegion(addr, region))
    {
        FILE * stream;
        char cmd[10240];

        unsigned long long offset = (unsigned long long)addr - (unsigned long long)region.start;
        //sprintf(cmd, "csky-abiv2-ux-linuxv3615-addr2line -a -e %s -f -C %p", soniaPath, addr);
        //sprintf(cmd, "aarch64-himix210-linux-sd3403v100-v1-addr2line -a -e %s -f -C %p", soniaPath, addr);
        sprintf(cmd, "%saddr2line -a -e %s%s -f -C %p", m_cross.c_str(), m_execPath.c_str(), region.execName.c_str(), (void *)offset);
        //printf("cmd %s\n", cmd);
        stream = popen(cmd, "r");
        fread(symbolBuf, sizeof(char), sizeof(symbolBuf), stream);
        pclose(stream);
        printf("%s", symbolBuf);
        m_mapAddr[addr] = symbolBuf;
    }
    else if (region.end != NULL)
    {
        printf("0x%lx\n", (unsigned long)addr - (unsigned long)region.start);
        //m_mapAddr[addr] = formatPointerAddress(addr);
        //printf("%s\n", m_mapAddr[addr].c_str());
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
