
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <unistd.h>
#include <malloc.h>

#include <new>

// #ifdef __cplusplus
// extern "C" {
// #endif

// // void *__wrap_calloc(size_t nmemb, size_t size);
// // void *__wrap_malloc(size_t size);
// // void __wrap_free(void *ptr);
// // void *__wrap_realloc(void *ptr, size_t size);
// // char *__wrap_strdup(const char *ptr);

// void MemTraceInit();

// #ifdef __cplusplus
// }
// #endif

void MemTraceInit();

void SaveTraceInfo(int optype, void * buf, int len);

// void * operator new(size_t size);
// void * operator new (size_t size, const std::nothrow_t&);

// //是否重载取决于C++库的实现
// void * operator new[](size_t size);
// void operator delete(void *ptr) _GLIBCXX_USE_NOEXCEPT
//   __attribute__((__externally_visible__));

// //是否重载取决于C++库的实现
// void operator delete[](void *ptr) _GLIBCXX_USE_NOEXCEPT
//   __attribute__((__externally_visible__));

typedef enum ENUM_MEMOPTYPE
{
	MEMOP_FREE = 0,
	MEMOP_DELETE = 1,
	MEMOP_DELETE_ARRAY = 2,
	MEMOP_MALLOC = 3,
	MEMOP_CALLOC = 4,
	MEMOP_REALLOC = 5,
	MEMOP_STRDUP = 6,
	MEMOP_NEW = 7,
	MEMOP_NEW_NOTHROW = 8,
	MEMOP_NEW_ARRAY = 9,
}ENUM_MEMOPTYPE;

#if defined(USE_DEP) && (USE_DEP > 0)
const int STACK_DEP = USE_DEP;
#else
const int STACK_DEP = 10;
#endif
#define OUTPUT_BUFSIZE_LAST (1024*1024)

// 结构体大小32位系统为32字节，64位系统为48字节 ，不包含栈深度
typedef struct __attribute__((packed)) MemLogInfo {
	unsigned int currtime;
	int size;
	int16_t tid;
	int8_t type;
	int8_t dep;
	void *ptr;
	void *ptrx;
	void *ptrlr;
	void *spinfo[STACK_DEP];
} MemLogInfo;

const int ONE_BUF_SIZE = sizeof(MemLogInfo) - sizeof(void *) * STACK_DEP;
const int ONE_SPINFO_SIZE = (512);
const int ONE_BUF_WITHSP_SIZE = (ONE_BUF_SIZE + ONE_SPINFO_SIZE);
const int ONE_SPINFO_ITEM = (ONE_SPINFO_SIZE/8);

const int ONE_ROW_SP_ITEM = (64);
const int TOTAL_LEN = OUTPUT_BUFSIZE_LAST - sizeof(MemLogInfo); 

const int CACHE_LEN = 4 * 1024 * 1024;

const int LIST_LEN = CACHE_LEN / sizeof(MemLogInfo); 

static pthread_mutex_t	g_mutexFree = PTHREAD_MUTEX_INITIALIZER;
std::list<MemLogInfo> g_freeMemList;
static pthread_mutex_t	g_mutexUsed = PTHREAD_MUTEX_INITIALIZER;
std::list<MemLogInfo> g_usedMemList;
static pthread_mutex_t	g_mutexAvila = PTHREAD_MUTEX_INITIALIZER;
std::list<MemLogInfo> g_avilaMemList;

typedef enum ENUM_LOGFILEOPTYPE
{
	LOGFILEOP_CLOSE = 0,
	LOGFILEOP_WRITE = 1,
}ENUM_LOGFILEOPTYPE;

//#define ONE_BUF_SIZE (48+512)

static pthread_mutex_t	mwGlobalMutexMemTrace = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t	mwGlobalMutexWrite = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;
static const char* rootPath = "/root/mallocupstat/map";
static int s_init = 0;

static unsigned int starttime = 0;
static unsigned int g_lasttime = 0;
static int8_t g_lastswitch = 0;
static int g_traceTid = 0;
static size_t g_traceSize = 0u;
static int8_t g_trace = 0;
static FILE * g_file = NULL;
pthread_t thread_id;


static int g_outputbuf_pos = 0;
static char * g_outputbuf = NULL;
static char * g_writebuf = NULL;
static int g_writelen = 0;
int g_writeThread = -1;
static __thread int in_malloc = 0;


// 清理函数（如果需要）
__attribute__((destructor)) void cleanup() {
    // ... 在dlclose时执行的清理代码（如果有）
}

void printMap(int pid)
{
	fprintf(stdout,"[%s:%d]pid:%ld pid:%d\n",__FUNCTION__, __LINE__, pid, pid);
	if (pid <= 0)
	{
		return;
	}
	//int pid = getpid();
	//if(0 != s_MemTraceMode.PrintMaps)
	//{
		char name[128];
		struct tm t;
		time_t tt;
		time(&tt);
		localtime_r(&tt, &t);
		int len = sprintf(name, "%s/maps-%04d%02d%02d_%02d%02d%02d.txt", rootPath
			, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
		name[len] = '\0';
		FILE * stream;
		char cmd[128] = {0};
		len = sprintf(cmd, "cat /proc/%d/maps > %s", pid, name);
		//cmd[len] = '\0';
		cmd[strlen(cmd) - 1] = '\0';
		fprintf(stdout,"[%s:%d]cmd:%s\n",__FUNCTION__, __LINE__, cmd);
		stream = popen(cmd, "w");
		if (stream == NULL)
		{
			fprintf(stdout,"[%s:%d]error\n",__FUNCTION__, __LINE__);
			return;
		}
		fclose(stream);
	//}
}
void* writeFunction(void* arg) 
{
	g_writeThread = syscall(SYS_gettid);
	fprintf(stdout,"--zl:start threadFunction--. g_writeThread:%d\n", g_writeThread);
	pthread_mutex_lock(&mwGlobalMutexWrite);
    while (1) {
		#ifdef USE_FREE
		pthread_cond_wait(&cond, &mwGlobalMutexWrite);
		
		
		// 判断是否需要从usedMemList中删除数据
		// 获取g_freeMemList的缓存数据
		pthread_mutex_lock(&g_mutexFree);
		std::list<MemLogInfo> freeMemList;
		freeMemList.splice(freeMemList, g_freeMemList);
		pthread_mutex_unlock(&g_mutexFree);

		if (usedMemList.size() == 0)
		{
			pthread_mutex_lock(&g_mutexAvila);
			g_avilaMemList.splice(g_avilaMemList, freeMemList);
			pthread_mutex_unlock(&g_mutexAvila);
			continue;
		}

		// 获取g_usedMemList的数据
		pthread_mutex_lock(&g_mutexUsed);
		std::list<MemLogInfo> usedMemList;
		usedMemList.splice(usedMemList, g_usedMemList);
		pthread_mutex_unlock(&g_mutexUsed);
		for (auto &it : freeMemList)
		{
			if (it.ptr < usedMemList.front().ptr || it.ptr > usedMemList.back().ptr)
			{
				// 从usedMemList中删除数据
				pthread_mutex_lock(&g_mutexAvila);
				g_avilaMemList.splice(g_avilaMemList.end(), freeMemList, it);
				pthread_mutex_unlock(&g_mutexAvila);
			}
			else
			{
				auto itUse = std::find_if(usedMemList.begin(), usedMemList.end(), [it.ptr](const MemLogInfo& info) {
					return info.ptr == ptr;
				});
				if (itUse != usedMemList.end()) {
					pthread_mutex_lock(&g_mutexAvila);
					g_avilaMemList.splice(g_avilaMemList.end(), usedMemList, itUse);
					g_avilaMemList.splice(g_avilaMemList.end(), freeMemList, it);
					pthread_mutex_unlock(&g_mutexAvila);
				}
				else
				{
					pthread_mutex_lock(&g_mutexAvila);
					g_avilaMemList.splice(g_avilaMemList.end(), freeMemList, it);
					pthread_mutex_unlock(&g_mutexAvila);
				}
			}
		}
		pthread_mutex_lock(&g_mutexUsed);
		g_usedMemList.merge(usedMemList);
		//g_usedMemList.splice(g_usedMemList, usedMemList);
		pthread_mutex_unlock(&g_mutexUsed);
		#elif
        printf("Thread %d waiting...\n", g_writeThread);
        pthread_cond_wait(&cond, &mwGlobalMutexWrite);
        printf("Thread %d woken up\n", g_writeThread);
		SaveTraceInfo(LOGFILEOP_WRITE, g_writebuf, g_writelen);
		#endif
    }
	pthread_mutex_unlock(&mwGlobalMutexWrite);
	fprintf(stdout,"--zl:end threadFunction--.\n");
    pthread_exit(NULL);
	return NULL;
}
/**
参数：ipaddr port 函数名：建立tcp连接
目的：建立tcp连接
返回值：成功返回0，失败返回-1
*/
int tcp_connect(const char *ipaddr, int port)
{
	int sockfd;
	struct sockaddr_in servaddr;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return -1;
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	if (inet_pton(AF_INET, ipaddr, &servaddr.sin_addr) <= 0)
	{
		perror("inet_pton");
		return -1;
	}
	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("connect");
		return -1;
	}
	return sockfd;
}

/**
 * purpose: 通过tcp_connect建立链接，发送g_writebuf里面的数据
 * param:ipaddr port 
 * return:
*/
void send_data(const char *ipaddr, int port)
{
	int sockfd = tcp_connect(ipaddr, port);
	if (sockfd < 0)
	{
		return;
	}
	int len = 0;
	while (1)
	{
		pthread_mutex_lock(&mwGlobalMutexWrite);
		if (g_writelen > 0)
		{
			len = write(sockfd, g_writebuf, g_writelen);
			if (len < 0)
			{
				perror("write");
				break;
			}
			g_writelen = 0;
		}
		pthread_mutex_unlock(&mwGlobalMutexWrite);
	}
	close(sockfd);
}

void ReadParam(int8_t &isCheck, int &traceTid, size_t traceSize , int8_t& trace)
{
	isCheck = 0;
	traceTid = 0;
	traceSize = 0u;
	trace = 0;
	int init = 0;
	FILE * file = NULL;
	file = fopen("/root/mallocupstat/param", "r");
	if(file != NULL)
	{
		size_t len = 0;
		char buf[128];
		memset(buf, 0, sizeof(buf));
		len = fread(buf, sizeof(char), sizeof(buf), file);
		if (len > 0)
		{
			//isCheck = atoi(buf);
			//0:off/1:on tid size
			//fprintf(stdout,"46885 start change memtrace_param\n");
			sscanf(buf, "%d %d %u %d %d", &isCheck, &traceTid, &traceSize, &trace, &init);
		}
		fclose(file);
		file = NULL;
	}
	fprintf(stdout,"[%s:%d]memtrace_param:%d %d %u %d %d.\n",__FUNCTION__, __LINE__, isCheck, traceTid, traceSize,trace,init);
	if (g_lastswitch != isCheck)
	{
		printMap(getpid());
	}
	
	if (isCheck == 1)
	{
		#ifdef USE_FREE
		if (g_freeMemList.size() == 0 && g_useMemList.size() == 0
		{
			g_freeMemList.assign(LIST_LEN, MemLogInfo());
			fprintf(stdout,"[%s:%d]malloc g_freeMemList size:%d\n",__FUNCTION__, __LINE__, LIST_LEN);
		}
#else
		if(g_outputbuf == NULL)
		{
			fprintf(stdout,"[%s:%d]malloc g_outputbuf\n",__FUNCTION__, __LINE__);
			char *outputbuf = NULL;
			outputbuf = (char *)malloc(OUTPUT_BUFSIZE_LAST);
			if(outputbuf == NULL)
			{
				fprintf(stdout,"[%s:%d]=========>malloc failed!\n",__FUNCTION__, __LINE__);
				return;
			}
			memset(outputbuf, 0, OUTPUT_BUFSIZE_LAST);
			g_outputbuf = outputbuf;
		}
		if(g_writebuf == NULL)
		{
			fprintf(stdout,"[%s:%d]malloc g_writebuf\n",__FUNCTION__, __LINE__);
			char *outputbuf = NULL;
			outputbuf = (char *)malloc(OUTPUT_BUFSIZE_LAST);
			if(outputbuf == NULL)
			{
				fprintf(stdout,"[%s:%d]=========>malloc failed!\n",__FUNCTION__, __LINE__);
				return;
			}
			memset(outputbuf, 0, OUTPUT_BUFSIZE_LAST);
			g_writebuf = outputbuf;

		}
		#endif
		if (g_writeThread < 0)
		{
			pthread_t thread;
			int ret = pthread_create(&thread, NULL, writeFunction, NULL);
			if (ret != 0)
			{
				fprintf(stdout, "zl:Failed to create writeFunction.\n");
				// std::cerr << "Failed to create thread." << std::endl;
				return;
			}
			// g_writeThread = thread;
			if (pthread_detach(thread) != 0)
			{
				fprintf(stdout, "zl:Failed to detach writeFunction\n");
			}
			else
			{
				fprintf(stdout, "--zl:writeFunction(%d) detach--.\n", g_writeThread);
			}
		}
		
	}
	
	return;
}
void* threadFunction(void* arg) {
    //int id = *(int*)arg; // 获取传递的参数
	
	fprintf(stdout,"--zl:start read param--.\n");
    //std::cout << "Hello from thread " << id << std::endl;
	
	while(1)
	{
		int8_t lastswitch = 0;
		int traceTid = 0;
		size_t traceSize = 0u;
		int8_t trace = 0;
		ReadParam(lastswitch, traceTid, traceSize, trace);
		if (lastswitch != g_lastswitch)
		{
			//g_lastswitch = lastswitch;
			__sync_bool_compare_and_swap(&g_lastswitch, g_lastswitch, lastswitch);
			fprintf(stdout,"[%s:%d]--g_lastswitch:%d--.\n",__FUNCTION__, __LINE__, g_lastswitch);
			if (lastswitch == 0)
			{
				//保存文件
				pthread_mutex_lock(&mwGlobalMutexMemTrace);
				SaveTraceInfo(LOGFILEOP_CLOSE, NULL, 0);
				pthread_mutex_unlock(&mwGlobalMutexMemTrace);
			}
		}
		if (traceTid != g_traceTid)
		{
			//g_traceTid = traceTid;
			__sync_bool_compare_and_swap(&g_traceTid, g_traceTid, traceTid);
		}
		if (traceSize != g_traceSize)
		{
			//g_traceSize = traceSize;
			__sync_bool_compare_and_swap(&g_traceSize, g_traceSize, traceSize);
		}
		if (trace != g_trace)
		{
			//g_trace = trace;
			__sync_bool_compare_and_swap(&g_trace, g_trace, trace);
		}

		sleep(30);
	}
	fprintf(stdout,"--zl:end read param--.\n");
    pthread_exit(NULL);
	return NULL;
}

void createThread(int sonit_pid)
{
	fprintf(stdout,"--zl:createThread--.\n");
	pthread_t thread;
	int param = 1;
	int ret = pthread_create(&thread, NULL, threadFunction, NULL);
    if (ret != 0) {
		fprintf(stdout,"zl:Failed to create thread.\n");
        //std::cerr << "Failed to create thread." << std::endl;
        return ;
    }

    //ret = pthread_join(thread, NULL);
    //if (ret != 0) {
	//	fprintf(stdout,"zl:Failed to join thread.\n");
    //    //std::cerr << "Failed to join thread." << std::endl;
    //    return ;
    //}
	
	if (pthread_detach(thread) != 0)
    {
        fprintf(stdout, "zl:Failed to detach thread\n");
    }
	fprintf(stdout,"--zl:createThread end--.\n");
}



__attribute__ ((constructor))
void _main(int argc, char** argv)
{
	fprintf(stdout,"zl: malloctest init argc:%d\n", argc);
	char path[1024];
	memset(path, 0 ,sizeof(path));
	if (readlink("/proc/self/exe", path, sizeof(path) - 1) > 0)
	{
		char *pName = strrchr(path, '/');
		if (pName != NULL)
		{
			fprintf(stdout,"zl:pname:%s\n", pName);
			int len = strlen(pName);
			for( int i = 0; i < len-4; ++i)
			{
				if (pName[i] == 's')
				{
					if (pName[i+1] == 'o'
					&& pName[i+2] == 'n'
					&& pName[i+3] == 'i'
					&& pName[i+4] == 'a')
					{
						MemTraceInit();
						s_init = 1;
						fprintf(stdout,"zl: s_init:%d\n",s_init);
						//createThread(getpid());
						break;
					}
				}
			}
		}
	}
}


void MemTraceInit()
{
	fprintf(stdout,"[%s:%d] MemTraceInit 1.\n", __FUNCTION__, __LINE__);
	int ret = 0;
	printMap(getpid());
	ret = pthread_mutex_init(&mwGlobalMutexMemTrace, NULL);
	fprintf(stdout,"[%s:%d] MemTraceInit 2. ret: %d.\n", __FUNCTION__, __LINE__, ret);
}


void SaveTraceInfo(int optype, void * buf, int len)
{
	fprintf(stdout,"[%s:%d][tid:%d]SaveTraceInfo start\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));
	static int total = 0;
	static unsigned int no = 0u;

	// if (LOGFILEOP_CLOSE == optype)
	// {
	// 	if (g_file != NULL)
	// 	{
	// 		fprintf(stdout,"[%s:%d][tid:%d]close file no:%d\n", __FUNCTION__, __LINE__, syscall(SYS_gettid), no);
	// 		total = 0;
	// 		fflush(g_file);
	// 		fclose(g_file);
	// 		g_file = NULL;
	// 	}
	// 	return;
	// }
	
	if (g_file != NULL)
	{
		if (total >= 209715200)
		{//>=200MB
			total = 0;
			fclose(g_file);
			g_file = NULL;
		}
	}
	fprintf(stdout,"[%s:%d][tid:%d]SaveTraceInfo start 1\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));
	if (NULL == g_file)
	{
		char name[128];
		struct tm t;
		time_t tt;
		time(&tt);
		localtime_r(&tt, &t);
		sprintf(name, "/root/mallocupstat/list/memery-%04d%02d%02d_%02d%02d%02d-%u.log"
			, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec
			, no);
		fprintf(stdout,"[%s:%d][tid:%d]SaveTraceInfo start 4\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));
		g_file = fopen(name, "a");
		fprintf(stdout,"[%s:%d][tid:%d]SaveTraceInfo start 5\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));
		if (NULL == g_file)
		{
			fprintf(stdout,"[%s:%d][tid:%d]open %s failed.\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), name);
			return;
		}
		fprintf(stdout,"[%s:%d][tid:%d]create file no:%d\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), no);
		no++;
	}
	fwrite(buf, 1, len, g_file);
	fflush(g_file);
	total += len;
	return;
}

void printSymbol(void* address) {
    Dl_info info;
    if (dladdr(address, &info)) {
        printf("Function: %s\n", info.dli_sname);
        printf("Library: %s\n", info.dli_fname);
    } else {
        printf("No symbol information available.\n");
    }
}

void printStackTrace() {
    void *addresses[10];
    unsigned int i = 0, frame_count = 0;

    // 获取当前栈帧的地址
    void *current_frame = nullptr;

#define frametrace(index, index1)                                                                                        \
    addresses[index] = __builtin_return_address(index);                                                                  \
    current_frame = __builtin_frame_address(index1);                                                                     \
    fprintf(stderr,"[%s:%d] current_frame:%p, return:%p\n",__FUNCTION__, __LINE__, current_frame, addresses[index]); \
    if (current_frame == 0 || addresses[index] == 0)                                                                     \
    {                                                                                                                    \
        frame_count = index;                                                                                             \
        goto end;                                                                                                        \
    }                                                                                                                    \
    frame_count = index1;


    frametrace(0, 1);
    frametrace(1, 2);
    frametrace(2, 3);
    frametrace(3, 4);
    frametrace(4, 5);
    frametrace(5, 6);
    frametrace(6, 7);
    frametrace(7, 8);
    frametrace(8, 9);
    frametrace(9, 10);
end:
    fprintf(stderr,"[%s:%d] frame_count:%d\n",__FUNCTION__, __LINE__, frame_count);
    // 打印地址
    for (i = 0; i < frame_count; ++i) {
        fprintf(stderr,"Frame %d: %p\n", i, (void *)addresses[i]);
    }
}

void stacktrace(unsigned int type, void *ptr, void *ptrx, unsigned long ptrlr, size_t size)
{
	if (s_init==0)
	{
		return;
	}
 	long i;
	long* sp = &i;
	
	time_t tt;
	time(&tt);

	unsigned int currtime = (unsigned int)tt;
	//fprintf(stdout,"[%s:%d][tid:%d]starttime is %u\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), currtime);

	if(0 == g_lastswitch)
	{
		return;
	}
	

	unsigned int tid = syscall(SYS_gettid);
	if (tid == g_writeThread)
	{
		return;
	}
	
	fprintf(stdout,"[%s:%d][tid:%d]currtime is %u\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), currtime);

	//printStackTrace();

	void *addresses[STACK_DEP];
	int frame_count = 0;
	in_malloc = 1;
	frame_count = backtrace(addresses, STACK_DEP);
	for (int i = 0; i < frame_count; i++)
	{
		fprintf(stdout,"[%s:%d] frame %d: %p\n", __FUNCTION__, __LINE__, i, addresses[i]);
	}

	if (g_traceTid > 0 && tid != g_traceTid)
	{
		return;
	}
	if (type >= MEMOP_MALLOC && g_traceSize > 0u && size != g_traceSize)
	{
		return;
	}
	
	fprintf(stdout,"[%s:%d][tid:%d]stacktrace tid: %ld\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), tid);

	if (g_outputbuf == NULL)
	{
		return;
	}
	
	pthread_mutex_lock(&mwGlobalMutexMemTrace);
	//fprintf(stdout,"[%s:%d][tid:%d]stacktrace start\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));

	if(g_outputbuf_pos > 0 && (currtime > g_lasttime + 30 || (g_outputbuf_pos + ONE_BUF_SIZE + 512 >= OUTPUT_BUFSIZE_LAST)))
	{
		pthread_mutex_lock(&mwGlobalMutexWrite);
		memcpy(g_writebuf, g_outputbuf, g_outputbuf_pos);
		g_writelen = g_outputbuf_pos;
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mwGlobalMutexWrite);

		//SaveTraceInfo(LOGFILEOP_WRITE, g_outputbuf, g_outputbuf_pos);
		g_outputbuf_pos = 0;
		g_lasttime = currtime;
	}
	//fprintf(stdout,"[%s:%d][tid:%d]stacktrace start 1\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));
	//if((g_outputbuf_pos > 0) && (g_outputbuf_pos + ONE_BUF_SIZE >= OUTPUT_BUFSIZE_LAST))
	//{
	//	SaveTraceInfo(LOGFILEOP_WRITE, g_outputbuf, g_outputbuf_pos);
		// memcpy(g_outputbuf, &currtime, 4);
		// memcpy(g_outputbuf+4, &type, 4);
		// memcpy(g_outputbuf+8, &tid, 4);
		// memcpy(g_outputbuf+16, &size, 8);
		// memcpy(g_outputbuf+24, &ptr, 8);
		// memcpy(g_outputbuf+32, &ptrx, 8);
		// memcpy(g_outputbuf+40, &ptrlr, 8);
		// if (type >= MEMOP_MALLOC && type <= MEMOP_NEW_ARRAY)
		// {
		// 	if(g_trace != 0)
		// 	{
		// 		memcpy(g_outputbuf+48, sp, 512);
		// 		g_outputbuf_pos = ONE_BUF_WITHSP_SIZE;
		// 	}
		// 	else
		// 	{
		// 		g_outputbuf_pos = ONE_BUF_SIZE;	
		// 	}
		// }
		// else
		// {
		// 	g_outputbuf_pos = ONE_BUF_SIZE;
		// }
		// g_lasttime = (unsigned int)tt;
	//}
	//else
	// {
		memcpy(g_outputbuf+g_outputbuf_pos, &currtime, 4);
		memcpy(g_outputbuf+g_outputbuf_pos+4, &type, 4);
		memcpy(g_outputbuf+g_outputbuf_pos+8, &tid, 4);
		memcpy(g_outputbuf+g_outputbuf_pos+16, &size, 8);
		memcpy(g_outputbuf+g_outputbuf_pos+24, &ptr, 8);
		memcpy(g_outputbuf+g_outputbuf_pos+32, &ptrx, 8);
		memcpy(g_outputbuf+g_outputbuf_pos+40, &ptrlr, 8);
		//memcpy(g_outputbuf+g_outputbuf_pos+40, &dep, 8);
		if (type >= MEMOP_MALLOC && type <= MEMOP_NEW_ARRAY)
		{
			if(g_trace != 0)
			{
				memcpy(g_outputbuf+g_outputbuf_pos+48, &sp, 512);
				g_outputbuf_pos += ONE_BUF_WITHSP_SIZE;	
			}
			else
			{
				g_outputbuf_pos += ONE_BUF_SIZE;
			}
		}
		else
		{
			g_outputbuf_pos += ONE_BUF_SIZE;
		}
	// 	if(currtime > g_lasttime + 30)
	// 	{
	// 		SaveTraceInfo(LOGFILEOP_WRITE, g_outputbuf, g_outputbuf_pos);
	// 		g_outputbuf_pos = 0;
	// 		g_lasttime = (unsigned int)tt;
	// 	}
	// }
	//fprintf(stdout," [thread:%d]stacktrace end\n", syscall(SYS_gettid));
	pthread_mutex_unlock(&mwGlobalMutexMemTrace);
	return;
}

extern "C" void __libc_free(void*);
// extern "C" void free(void *ptr)
// {
// 	if (ptr == NULL)
// 	{
// 		return;
// 	}
// 	if(s_init==1)
// 	{
// 		fprintf(stdout,"[%s:%d] start\n", __FUNCTION__, __LINE__);
// 	}
// 	unsigned long lrp = 0u;
// 	//stacktrace(MEMOP_FREE, ptr, NULL, lrp, 0);
// 	__libc_free(ptr);
// 	if (s_init==1)
// 	{
// 		fprintf(stdout,"[%s:%d] end\n", __FUNCTION__, __LINE__);
// 	}
// 	return;
// }


extern "C" void *__libc_malloc(size_t size);
extern "C" void * malloc(size_t size)
{
	if(s_init==1)
	{
		fprintf(stdout,"[%s:%d] start malloc:%d\n", __FUNCTION__, __LINE__,in_malloc);
		if (in_malloc) {
			fprintf(stdout,"[%s:%d] end malloc\n", __FUNCTION__, __LINE__);
        	return (void*)__libc_malloc(size);
    	}
		// void *addresses[10];
		// int frame_count = 0;
		// in_malloc = 1;
		// frame_count = backtrace(addresses, 10);
		// for (int i = 0; i < frame_count; i++)
		// {
		// 	fprintf(stdout,"[%s:%d] frame %d: %p\n", __FUNCTION__, __LINE__, i, addresses[i]);
		// }
		//printStackTrace();
		
	}
	
	unsigned long lrp = 0u;
	#ifdef SYSTEM64
		__asm__ __volatile__("mov %0, X30": "=g"(lrp));
	#elif defined(SYSTEM32)
		__asm__ __volatile__("mov %0, lr": "=r"(lrp));
	#else
	#endif
	
	void* result = NULL;
	result = (void*)__libc_malloc(size);

	//result = original_malloc(size);
	stacktrace(MEMOP_MALLOC, result, NULL, lrp, size);
	
	if (s_init==1)
	{
		fprintf(stdout,"[%s:%d] end\n", __FUNCTION__, __LINE__);
		in_malloc = 0;
	}
	return result;
}

extern "C" void *__libc_calloc(size_t, size_t);
extern "C" void *calloc(size_t nmemb, size_t size)
{
	unsigned long lrp = 0u;
	#ifdef SYSTEM64
		__asm__ __volatile__("mov %0, X30": "=g"(lrp));
	#elif defined(SYSTEM32)
		__asm__ __volatile__("mov %0, lr": "=r"(lrp));
	#else
	#endif
	//lrp = (unsigned long)__builtin_return_address(1);
	void* result = NULL;
	result = (void*)__libc_calloc(nmemb, size);
	stacktrace(MEMOP_CALLOC, result, NULL, lrp, nmemb*size);
	return result;
}

extern "C" void *__libc_realloc(void*, size_t);
extern "C" void *realloc(void *ptr, size_t size)
{
	unsigned long lrp = 0u;
	#ifdef SYSTEM64
		__asm__ __volatile__("mov %0, X30": "=g"(lrp));
	#elif defined(SYSTEM32)
		__asm__ __volatile__("mov %0, lr": "=r"(lrp));
	#else
	#endif
	void* result = NULL;
	result = (void*)__libc_realloc(ptr, size);
	stacktrace(MEMOP_REALLOC, result, ptr, lrp, size);
	return result;
}

void * operator new(size_t size)
{
	if(s_init==1)
	{
		fprintf(stdout,"[%s:%d] start\n", __FUNCTION__, __LINE__);
	}
	unsigned long lrp = 0u;
	#ifdef SYSTEM64
		__asm__ __volatile__("mov %0, X30": "=g"(lrp));
	#elif defined(SYSTEM32)
		__asm__ __volatile__("mov %0, lr": "=r"(lrp));
	#else
	#endif
	void * result = NULL;
	result = (void*)__libc_malloc(size);
	stacktrace(MEMOP_NEW, result, NULL, lrp, size);
	if(result == NULL)
	{
		fprintf(stdout,"=========>malloc failed!\n");
		throw std::bad_alloc();
	}
	if (s_init==1)
	{
		fprintf(stdout,"[%s:%d] end\n", __FUNCTION__, __LINE__);
	}
	return result;
}
void * operator new (size_t size, const std::nothrow_t&)
{
	if(s_init==1)
	{
		fprintf(stdout,"[%s:%d] start\n", __FUNCTION__, __LINE__);
	}
	unsigned long lrp = 0u;
	#ifdef SYSTEM64
		__asm__ __volatile__("mov %0, X30": "=g"(lrp));
	#elif defined(SYSTEM32)
		__asm__ __volatile__("mov %0, lr": "=r"(lrp));
	#else
	#endif
	void * result = NULL;
	result = (void*)__libc_malloc(size);
	stacktrace(MEMOP_NEW_NOTHROW, result, NULL, lrp, size);
	if(result == NULL)
	{
		fprintf(stdout,"=========>malloc failed!\n");
	}
	
	if (s_init==1)
	{
		fprintf(stdout,"[%s:%d] end\n", __FUNCTION__, __LINE__);
	}
	return result;
}

void * operator new[](size_t size)
{
	if(s_init==1)
	{
		fprintf(stdout,"[%s:%d] start\n", __FUNCTION__, __LINE__);
	}

	unsigned long lrp = 0u;
	#ifdef SYSTEM64
		__asm__ __volatile__("mov %0, X30": "=g"(lrp));
	#elif defined(SYSTEM32)
		__asm__ __volatile__("mov %0, lr": "=r"(lrp));
	#else
	#endif
	void * result = NULL;
	result = (void*)__libc_malloc(size);
	stacktrace(MEMOP_NEW_ARRAY, result, NULL, lrp, size);
	if(result == NULL)
	{
		fprintf(stdout,"=========>malloc failed!\n");
	}
	if (s_init==1)
	{
		fprintf(stdout,"[%s:%d] end\n", __FUNCTION__, __LINE__);
	}
	return result;
}

void operator delete(void* ptr) _GLIBCXX_USE_NOEXCEPT 
{
	if(s_init==1)
	{
		fprintf(stdout,"[%s:%d] start ptr:%p\n", __FUNCTION__, __LINE__, ptr);
	}
	unsigned long lrp = 0u;
	if(ptr != NULL)
	{
		stacktrace(MEMOP_DELETE, ptr, NULL, lrp, 0);
		__libc_free(ptr);
	}
	if (s_init==1)
	{
		fprintf(stdout,"[%s:%d] end\n", __FUNCTION__, __LINE__);
	}
	return;
}

void operator delete[](void *ptr) _GLIBCXX_USE_NOEXCEPT
{
	if (s_init==1)
	{
		fprintf(stdout,"[%s:%d] start ptr:%p\n", __FUNCTION__, __LINE__, ptr);
	}

	unsigned long lrp = 0u;
	if(ptr != NULL)
	{
		stacktrace(MEMOP_DELETE_ARRAY, ptr, NULL, lrp, 0);
		__libc_free(ptr);
	}
	if(s_init==1)
	{
		fprintf(stdout,"[%s:%d] end\n", __FUNCTION__, __LINE__);
	}
	return;
}


