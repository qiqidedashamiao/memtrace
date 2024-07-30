#include <string>
#include <map>
using namespace std;
class Maps {
public:
    Maps(const char * mapsPath, const char * execPath, const char* preCmd = "", bool aslr = true);
public:
    struct MemoryRegion {
        void *start;
        void *end;
        std::string execName;
        unsigned long num[3];  // 0:add 1:del_add 2:del
        MemoryRegion() : start(NULL), end(NULL), execName(""),add(0),del(0),del_add(0) {num[0] = 0; num[1] = 0; num[2] = 0;}
    };
    bool findMemoryRegion(void *address, MemoryRegion &region);
    void addr2symbol(void *addr);
    void setPreCmd(const char* preCmd) { m_preCmd = preCmd; }
    void setASLR(bool aslr) { m_bASLR = aslr; }
    void setExecPath(const char * execPath) { m_execPath = execPath; }
    void stat(void *ptrlr, int lrType);

private:
    map<void *, MemoryRegion> m_mapMemoryRegions;
    string m_execPath;
    string m_preCmd;
    bool m_bASLR;   //是否支持地址空间布局随机化（ASLR）
};