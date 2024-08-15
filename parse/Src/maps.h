#include <string>
#include <map>
using namespace std;
class Maps {
public:
    Maps();
public:
    struct MemoryRegion {
        void *start;
        void *end;
        std::string execName;
        unsigned long num[3];  // 0:add 1:del_add 2:del
        MemoryRegion() : start(NULL), end(NULL), execName("") {num[0] = 0; num[1] = 0; num[2] = 0;}
    };
    bool findMemoryRegion(void *address, MemoryRegion &region);
    void addr2symbol(void *addr);
    void setCross(const string cross) { m_cross = cross; }
    void setASLR(bool aslr) { m_bASLR = aslr; }
    void setExecPath(const string execPath) { m_execPath = execPath; }
    void setMapsPath(const string mapsPath);
    void stat(void *ptrlr, int lrType);
    void printStat();
    bool isExist(void *address);

private:
    map<void *, MemoryRegion> m_mapMemoryRegions;
    map<void *, string> m_mapAddr;
    string m_execPath;
    string m_cross;
    string m_mapsPath;
    bool m_bASLR;   //是否支持地址空间布局随机化（ASLR）
};