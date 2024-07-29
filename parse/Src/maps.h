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
        MemoryRegion() : start(NULL), end(NULL), execName("") {}
    };
    bool findMemoryRegion(void *address, MemoryRegion &region);
    void addr2symbol(void *addr);
    void setPreCmd(const char* preCmd) { m_preCmd = preCmd; }
    void setASLR(bool aslr) { m_bASLR = aslr; }
    void setExecPath(const char * execPath) { m_execPath = execPath; }
    void setMapsPath(const char * mapsPath) { m_mapsPath = mapsPath; }

private:
    map<void *, MemoryRegion> m_mapMemoryRegions;
    string m_execPath;
    string m_preCmd;
    bool m_bASLR;   //是否支持地址空间布局随机化（ASLR）
};