
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <dlfcn.h>
#include <unistd.h>
#include <malloc.h>
#include <new>

//如果定义宏USE_BACKTRACK，则包含头文件<execinfo.h>
#ifdef USE_BACKTRACE
#include <execinfo.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
void __libc_free(void*);
void * __libc_malloc(size_t);
void *__libc_calloc(size_t, size_t);
void *__libc_realloc(void*, size_t);

void MemTraceInit();

#ifdef __cplusplus
}
#endif


#define gettid() syscall(SYS_gettid)

void MemTraceInit();

void SaveTraceInfo(int optype, void * buf, int len);

void * operator new(size_t size);
void * operator new (size_t size, const std::nothrow_t&);

//是否重载取决于C++库的实现
void * operator new[](size_t size);
//__externally_visible__ 属性告诉编译器，即使这个函数在本翻译单元中未被引用，它也不应该被删除或优化掉，因为它可能会被外部链接使用（例如通过动态链接或插件机制）
void operator delete(void *ptr) _GLIBCXX_USE_NOEXCEPT
  __attribute__((__externally_visible__));

//是否重载取决于C++库的实现
void operator delete[](void *ptr) _GLIBCXX_USE_NOEXCEPT
  __attribute__((__externally_visible__));


// backtrace_symbols()
// backtrace_symbols_fd()


//内存申请释放类型

typedef enum ENUM_MEMOPTYPE
{
	MEMOP_FREE = 0,
	MEMOP_DELETE = 1,
	MEMOP_DELETE_ARRAY = 2,
	MEMOP_MALLOC = 3,
	MEMOP_CALLOC = 4,
	MEMOP_NEW = 5,
	MEMOP_NEW_NOTHROW = 6,
	MEMOP_NEW_ARRAY = 7,
	MEMOP_REALLOC = 8,
	MEMOP_MALLOC_BIG = 9,
	MEMOP_CALLOC_BIG = 10,
	MEMOP_NEW_BIG = 11,
	MEMOP_NEW_NOTHROW_BIG = 12,
	MEMOP_NEW_ARRAY_BIG = 13,
	MEMOP_REALLOC_BIG = 14,
	MEMOP_MAX = 15,
	//MEMOP_STRDUP = 16,
}ENUM_MEMOPTYPE;

//栈深度，在配置文件config.cfg中定义
const int STACK_DEP = STACK_DEPTH;


#define OUTPUT_BUFSIZE_LAST (1024*1024)
const int BITLEN = sizeof(void *);
// 结构体大小32位系统为32字节，64位系统为48字节 ，不包含栈深度
typedef struct __attribute__((packed)) MemLogInfo {
	int8_t type;
	int8_t dep;
	int16_t tid;
	unsigned int currtime;
	int size;
	void *ptr;    // 内存指针
	void *ptrlr;  // 调用堆栈指针
	void *ptrx;   // remalloc 之前使用指针
	//void *spinfo[STACK_DEP];
} MemLogInfo;

typedef struct MemLogInfoEx {
	MemLogInfo logInfo;
#ifdef USE_BACKTRACE
	void *spinfo[STACK_DEP];
#else
	void *lp;
#endif
} MemLogInfoEx;

const int ONE_BUF_SIZE_FREE = sizeof(MemLogInfo) -  sizeof(void *) * 2;
const int ONE_BUF_SIZE_MALLOC = sizeof(MemLogInfo) -  sizeof(void *) ;
const int ONE_BUF_SIZE_REMALLOC = sizeof(MemLogInfo);
const int ONE_SPINFO_SIZE = (512);
const int ONE_SPINFO_ITEM = (ONE_SPINFO_SIZE/8);

const int ONE_ROW_SP_ITEM = (64);
const int TOTAL_LEN = OUTPUT_BUFSIZE_LAST - sizeof(MemLogInfo);

// static const char* g_path_list_dir = "/root/mount/share/memtrace/list";
// static const char* g_path_map_dir = "/root/mount/share/memtrace/map";
// static const char* g_path_param = "/root/mount/share/memtrace/param";

static const char* g_path_list_dir = PATH_LIST_DIR;
static const char* g_path_map_dir = PATH_MAP_DIR;
static const char* g_path_param = PATH_PARAM;

typedef enum ENUM_LOGFILEOPTYPE
{
	LOGFILEOP_CLOSE = 0,
	LOGFILEOP_WRITE = 1,
}ENUM_LOGFILEOPTYPE;

//#define ONE_BUF_SIZE (48+512)

static pthread_mutex_t	mwGlobalMutexMemTrace = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t	mwGlobalMutexWrite = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;
static int s_init = 0;

static unsigned int g_lasttime = 0;
static int g_lastswitch = 0;
static int g_traceTid = 0;
static size_t g_traceSize = 0u;
static size_t g_bigSize = 0u;
static int g_trace = 0;
static FILE * g_file = NULL;
pthread_t thread_id;


static int g_outputbuf_pos = 0;
static char * g_outputbuf = NULL;
static char * g_writebuf = NULL;
static int g_writelen = 0;
int g_writeThread = -1;
static __thread int gst_in_malloc = 0;	
// 定义一个线程本地变量gst_in_malloc，用于标识当前线程是否在malloc中

// 清理函数（如果需要）
__attribute__((destructor)) void cleanup() {
    // ... 在dlclose时执行的清理代码（如果有）
}

void printMap(int pid)
{
	fprintf(stdout,"[%s:%d]pid:%d tid:%ld\n",__FUNCTION__, __LINE__, pid, gettid());
	if (pid <= 0)
	{
		return;
	}

	char name[128];
	struct tm t;
	time_t tt;
	time(&tt);
	localtime_r(&tt, &t);
	int len = sprintf(name, "%s/maps-%04d%02d%02d_%02d%02d%02d-%d.txt", g_path_map_dir, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, pid);
	name[len] = '\0';
	FILE *stream;
	char cmd[128] = {0};
	len = sprintf(cmd, "cat /proc/%d/maps > %s", pid, name);
	// cmd[len] = '\0';
	cmd[strlen(cmd) - 1] = '\0';
	fprintf(stdout, "[%s:%d][pid:%d][tid:%ld]cmd:%s\n", __FUNCTION__, __LINE__, pid, gettid(), cmd);
	stream = popen(cmd, "w");
	if (stream == NULL)
	{
		fprintf(stdout, "[%s:%d]error\n", __FUNCTION__, __LINE__);
		return;
	}
	fclose(stream);
}
void* writeFunction(void* arg) 
{
	g_writeThread = syscall(SYS_gettid);
	fprintf(stdout,"--[pid:%d][tid:%ld]zl:start writeFunction--\n", getpid(), gettid());
	pthread_mutex_lock(&mwGlobalMutexWrite);
    while (1) {
        //printf("[pid:%d][tid:%ld]waiting write... release write\n", getpid(), gettid(), g_writeThread);
        pthread_cond_wait(&cond, &mwGlobalMutexWrite);
        //printf("[pid:%d][tid:%ld]write woken up get write\n", getpid(), gettid(), g_writeThread);
		SaveTraceInfo(LOGFILEOP_WRITE, g_writebuf, g_writelen);
    }
	pthread_mutex_unlock(&mwGlobalMutexWrite);
	fprintf(stdout,"[pid:%d][tid:%ld]--zl:end writeFunction--.\n", getpid(), gettid());
    pthread_exit(NULL);
	return NULL;
}
/**
参数：ipaddr port 函数名：建立tcp连接
目的：建立tcp连接
返回值：成功返回0，失败返回-1
*/
// int tcp_connect(const char *ipaddr, int port)
// {
// 	int sockfd;
// 	struct sockaddr_in servaddr;
// 	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
// 	{
// 		perror("socket");
// 		return -1;
// 	}
// 	bzero(&servaddr, sizeof(servaddr));
// 	servaddr.sin_family = AF_INET;
// 	servaddr.sin_port = htons(port);
// 	if (inet_pton(AF_INET, ipaddr, &servaddr.sin_addr) <= 0)
// 	{
// 		perror("inet_pton");
// 		return -1;
// 	}
// 	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
// 	{
// 		perror("connect");
// 		return -1;
// 	}
// 	return sockfd;
// }

// /**
//  * purpose: 通过tcp_connect建立链接，发送g_writebuf里面的数据
//  * param:ipaddr port 
//  * return:
// */
// void send_data(const char *ipaddr, int port)
// {
// 	int sockfd = tcp_connect(ipaddr, port);
// 	if (sockfd < 0)
// 	{
// 		return;
// 	}
// 	int len = 0;
// 	while (1)
// 	{
// 		pthread_mutex_lock(&mwGlobalMutexWrite);
// 		if (g_writelen > 0)
// 		{
// 			len = write(sockfd, g_writebuf, g_writelen);
// 			if (len < 0)
// 			{
// 				perror("write");
// 				break;
// 			}
// 			g_writelen = 0;
// 		}
// 		pthread_mutex_unlock(&mwGlobalMutexWrite);
// 	}
// 	close(sockfd);
// }

/**
 * purpose: 读取memtrace_param文件，获取是否开启内存跟踪，是否跟踪栈信息，跟踪申请的内存大小，跟踪线程id
*/
void ReadParam(int &isStart , int & trace, int &traceTid, size_t &traceSize, size_t &bigSize)
{
	isStart = 0;
	traceTid = 0;
	traceSize = 0u;
	trace = 0;
	bigSize = 0;
	FILE * file = NULL;
	file = fopen(g_path_param, "r");
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
			sscanf(buf, "%d %d %zu %d %zu", &isStart, &trace, &traceSize, &traceTid, &bigSize);
		}
		fclose(file);
		file = NULL;
	}
	fprintf(stdout,"[%s:%d]memtrace_param:%d %d %zu %d %zu.\n",__FUNCTION__, __LINE__, isStart,trace, traceSize, traceTid, bigSize);
	if (g_lastswitch != isStart)
	{
		// 启动开关变化，打印map
		printMap(getpid());
	}
	
	if (isStart == 1)
	{
		// 第一次启动开关打开，创建内存信息输出缓存
		if(g_outputbuf == NULL)
		{
			fprintf(stdout,"[%s:%d][pid:%d][tid:%ld]malloc g_outputbuf\n",__FUNCTION__, __LINE__, getpid(), gettid());
			char *outputbuf = NULL;
			outputbuf = (char *)malloc(OUTPUT_BUFSIZE_LAST);
			if(outputbuf == NULL)
			{
				fprintf(stdout,"[%s:%d][pid:%d][tid:%ld]=========>malloc failed!\n",__FUNCTION__, __LINE__, getpid(), gettid());
				return;
			}
			memset(outputbuf, 0, OUTPUT_BUFSIZE_LAST);
			g_outputbuf = outputbuf;
		}
		// 第一次启动开关打开，创建内存信息写入输出缓存
		if(g_writebuf == NULL)
		{
			fprintf(stdout,"[%s:%d][pid:%d][tid:%ld]malloc g_writebuf\n",__FUNCTION__, __LINE__, getpid(), gettid());
			char *outputbuf = NULL;
			outputbuf = (char *)malloc(OUTPUT_BUFSIZE_LAST);
			if(outputbuf == NULL)
			{
				fprintf(stdout,"[%s:%d][pid:%d][tid:%ld]=========>malloc failed!\n",__FUNCTION__, __LINE__, getpid(), gettid());
				return;
			}
			memset(outputbuf, 0, OUTPUT_BUFSIZE_LAST);
			g_writebuf = outputbuf;
		}
		// 第一次启动开关打开，创建写入线程
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

void updateParam()
{
	int lastswitch = 0;
	int traceTid = 0;
	size_t traceSize = 0u;
	size_t bigSize = 0u;
	int trace = 0;
	ReadParam(lastswitch, trace, traceTid, traceSize, bigSize);
	if (lastswitch != g_lastswitch)
	{
		// g_lastswitch = lastswitch;
		__sync_bool_compare_and_swap(&g_lastswitch, g_lastswitch, lastswitch);
		fprintf(stdout, "[%s:%d][pid:%d][tid:%ld]--g_lastswitch:%d--.\n", __FUNCTION__, __LINE__, getpid(), gettid(), g_lastswitch);
		if (lastswitch == 0)
		{
			// 保存文件
			pthread_mutex_lock(&mwGlobalMutexMemTrace);
			pthread_mutex_lock(&mwGlobalMutexWrite);
			SaveTraceInfo(LOGFILEOP_CLOSE, g_writebuf, g_writelen);
			g_outputbuf_pos = 0;
			g_writelen = 0;
			pthread_mutex_unlock(&mwGlobalMutexWrite);
			pthread_mutex_unlock(&mwGlobalMutexMemTrace);
		}
	}
	if (traceTid != g_traceTid)
	{
		// g_traceTid = traceTid;
		__sync_bool_compare_and_swap(&g_traceTid, g_traceTid, traceTid);
	}
	if (traceSize != g_traceSize)
	{
		// g_traceSize = traceSize;
		__sync_bool_compare_and_swap(&g_traceSize, g_traceSize, traceSize);
	}
	if (trace != g_trace)
	{
		// g_trace = trace;
		__sync_bool_compare_and_swap(&g_trace, g_trace, trace);
	}
	if (bigSize != g_bigSize)
	{
		// g_traceSize = traceSize;
		__sync_bool_compare_and_swap(&g_bigSize, g_bigSize, bigSize);
	}
}

/**
 * purpose: 读取memtrace_param参数线程，更新是否开启内存跟踪，是否跟踪栈信息，跟踪申请的内存大小，跟踪线程id等参数信息
*/
void* threadFunction(void* arg)
 {
    //int id = *(int*)arg; // 获取传递的参数
	
	fprintf(stdout,"[pid:%d][tid:%ld]--zl:start read param--.\n", getpid(), gettid());
	
	while(1)
	{
		updateParam();

		// 30s读取一次
		sleep(30);
	}
	fprintf(stdout,"[pid:%d][tid:%ld]--zl:end read param--.\n", getpid(), gettid());
    pthread_exit(NULL);
	return NULL;
}


/**
 * purpose: 创建读取参数线程
 * param: sonit_pid 当前进程id
 * return:
*/
void createThread(int sonit_pid)
{
	fprintf(stdout,"[pid:%d][tid:%ld]--zl:createThread--.\n", getpid(), gettid());
	pthread_t thread;
	int ret = pthread_create(&thread, NULL, threadFunction, NULL);
    if (ret != 0) {
		fprintf(stdout,"[pid:%d][tid:%ld]zl:Failed to create thread.\n", getpid(), gettid());
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
        fprintf(stdout, "[pid:%d][tid:%ld]zl:Failed to detach thread\n", getpid(), gettid());
    }
	fprintf(stdout,"[pid:%d][tid:%ld]--zl:createThread end--.\n", getpid(), gettid());
}


/**
 * purpose: 进程启动前初始化内存跟踪信息
 * param:
 * return:
*/
__attribute__ ((constructor))
void _main(int argc, char** argv)
{
	fprintf(stdout,"[pid:%d][tid:%ld]zl: malloctest init argc:%d\n",getpid(), gettid(), argc);
	char path[1024];
	memset(path, 0 ,sizeof(path));
	if (readlink("/proc/self/exe", path, sizeof(path) - 1) > 0)
	{
		char *pName = strrchr(path, '/');
		if (pName != NULL)
		{
			fprintf(stdout,"[pid:%d][tid:%ld]zl:pname:%s\n",getpid(), gettid(), pName);
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
						updateParam();
						s_init = 1;
						fprintf(stdout,"[pid:%d][tid:%ld]zl: s_init:%d\n",getpid(), gettid(),s_init);
						createThread(getpid());
						break;
					}
				}
			}
		}
	}
}

/**
 * purpose: 初始化内存跟踪信息
 * param:
 * return:
*/
void MemTraceInit()
{
	fprintf(stdout,"[%s:%d][pid:%d][tid:%ld] MemTraceInit 1.\n", __FUNCTION__, __LINE__,getpid(), gettid());
	int ret = 0;
	//printMap(getpid());
	//ret = pthread_mutex_init(&mwGlobalMutexMemTrace, NULL);
	fprintf(stdout,"[%s:%d][pid:%d][tid:%ld] MemTraceInit 2. ret: %d.\n", __FUNCTION__, __LINE__,getpid(), gettid(), ret);
}

/**
 * purpose: 保存内存跟踪信息
 * param: optype 操作类型 buf 内存跟踪信息 len 内存跟踪信息长度
 * return:
*/
void SaveTraceInfo(int optype, void * buf, int len)
{
	//fprintf(stdout,"[%s:%d][tid:%ld] optype:%d SaveTraceInfo start\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), optype);
	static int total = 0;
	static unsigned int no = 0u;

	if (LOGFILEOP_CLOSE == optype)
	{
		if (g_file != NULL)
		{
			if (len > 0)
			{
				fwrite(buf, 1, len, g_file);
			}
			fprintf(stdout,"[%s:%d][tid:%ld]close file no:%d\n", __FUNCTION__, __LINE__, syscall(SYS_gettid), no-1);
			total = 0;
			fflush(g_file);
			fclose(g_file);
			g_file = NULL;
		}
		//fprintf(stdout,"[%s:%d][tid:%ld]end close file no:%d\n", __FUNCTION__, __LINE__, syscall(SYS_gettid), no-1);
		return;
	}
	
	if (g_file != NULL)
	{
		if (total >= 209715200)
		{//>=200MB
			total = 0;
			fflush(g_file);
			fclose(g_file);
			g_file = NULL;
		}
	}
	//fprintf(stdout,"[%s:%d][tid:%ld]SaveTraceInfo start 1\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));
	if (NULL == g_file)
	{
		char name[128];
		struct tm t;
		time_t tt;
		time(&tt);
		localtime_r(&tt, &t);
		sprintf(name, "%s/memery-%04d%02d%02d_%02d%02d%02d-%d-%u.log", g_path_list_dir
			, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
			getpid(), no);
		//fprintf(stdout,"[%s:%d][tid:%ld]SaveTraceInfo start 4\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));
		g_file = fopen(name, "a");
		//fprintf(stdout,"[%s:%d][tid:%ld]SaveTraceInfo start 5\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));
		if (NULL == g_file)
		{
			fprintf(stdout,"[%s:%d][tid:%ld]open %s failed.\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), name);
			return;
		}
		fprintf(stdout,"[%s:%d][tid:%ld]create file no:%d\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), no);
		no++;
	}
	// 1表示写入文件数据基本单元大小，len表示写入数据基本单元个数
	fwrite(buf, 1, len, g_file);
	//fflush(g_file);
	total += len;
	//fprintf(stdout,"[%s:%d][tid:%ld]SaveTraceInfo start 2\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));
	return;
}

void stacktrace(MemLogInfoEx &logInfoEx)
{
	time_t tt;
	time(&tt);

	MemLogInfo &logInfo = logInfoEx.logInfo;
	unsigned int currtime = (unsigned int)tt;
	logInfo.currtime = currtime;
	//fprintf(stdout,"[%s:%d][tid:%ld]starttime is %u\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), currtime);

	// if (tid == g_writeThread)
	// {
	// 	return;
	// }
	
	//fprintf(stdout,"[%s:%d][tid:%ld]currtime is %u\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), currtime);

	//printStackTrace();

	logInfo.tid = gettid();

#ifdef USE_THREAD
	if (g_traceTid > 0 && logInfo.tid != g_traceTid)
	{
		return;
	}
#endif
	
	//fprintf(stdout,"[%s:%d][tid:%ld] type:%d stacktrace tid: %d wait global\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), logInfo.type, tid);
	
	pthread_mutex_lock(&mwGlobalMutexMemTrace);
	//fprintf(stdout,"[%s:%d][tid:%ld] type:%d stacktrace0 tid: %d get global\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), logInfo.type, tid);
	if (logInfo.type >= MEMOP_MALLOC_BIG)
	{
#ifndef USE_BACKTRACE
		logInfo.dep = STACK_DEP;
#endif
		memcpy(g_outputbuf_pos + g_outputbuf, &logInfo, ONE_BUF_SIZE_MALLOC);
		g_outputbuf_pos += ONE_BUF_SIZE_MALLOC;
#ifdef USE_BACKTRACE
		memcpy(g_outputbuf_pos + g_outputbuf, logInfoEx.spinfo, logInfo.dep * BITLEN);
		g_outputbuf_pos += logInfo.dep * BITLEN;
#else
		fprintf(stdout,"[%s:%d][tid:%ld] type:%d logInfo.dep: %d STACK_DEP:%d\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), logInfo.type, logInfo.dep, STACK_DEP);
		memcpy(g_outputbuf_pos + g_outputbuf, logInfoEx.lp, logInfo.dep * BITLEN);
		g_outputbuf_pos += logInfo.dep * BITLEN;
#endif
		//fprintf(stdout,"[%s:%d][tid:%ld] type:%d stacktrace00 tid: %d get global wait write\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), logInfo.type, tid);
		pthread_mutex_lock(&mwGlobalMutexWrite);
		//fprintf(stdout,"[%s:%d][tid:%ld] type:%d stacktrace01 tid: %d get global get write\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), logInfo.type, tid);
		// 交换g_writebuf 和 g_outputbuf指针的值，不交换内容
		char * tempbuf = g_writebuf;
		g_writebuf = g_outputbuf;
		g_writelen = g_outputbuf_pos;
		g_outputbuf = tempbuf;
		g_outputbuf_pos = 0;
		g_lasttime = currtime;
		pthread_mutex_unlock(&mwGlobalMutexMemTrace);
		//fprintf(stdout,"[%s:%d][tid:%ld] type:%d stacktrace1 tid: %d release global get write\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), logInfo.type, tid);
		SaveTraceInfo(LOGFILEOP_WRITE, g_writebuf, g_writelen);
		SaveTraceInfo(LOGFILEOP_CLOSE, g_writebuf, 0);
		//fprintf(stdout,"[%s:%d][tid:%ld] type:%d stacktrace2 tid: %d release global get write\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), logInfo.type, tid);
		g_writelen = 0;
		pthread_mutex_unlock(&mwGlobalMutexWrite);
		//fprintf(stdout,"[%s:%d][tid:%ld] type:%d stacktrace3 tid: %d release global release write\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), logInfo.type, tid);
		return;
	}
	else if (logInfo.type >= MEMOP_REALLOC)
	{
		memcpy(g_outputbuf_pos + g_outputbuf, &logInfo, ONE_BUF_SIZE_REMALLOC);
		g_outputbuf_pos += ONE_BUF_SIZE_REMALLOC;
#ifdef USE_BACKTRACE
		memcpy(g_outputbuf_pos + g_outputbuf, logInfoEx.spinfo, logInfo.dep * BITLEN);
		g_outputbuf_pos += logInfo.dep * BITLEN;
#else
		logInfo.dep = STACK_DEP;
		memcpy(g_outputbuf_pos + g_outputbuf, logInfoEx.lp, logInfo.dep * BITLEN);
		g_outputbuf_pos += logInfo.dep * BITLEN;
#endif
	}
	else if (logInfo.type >= MEMOP_MALLOC)
	{
		memcpy(g_outputbuf_pos + g_outputbuf, &logInfo, ONE_BUF_SIZE_MALLOC);
		g_outputbuf_pos += ONE_BUF_SIZE_MALLOC;
#ifdef USE_BACKTRACE
		memcpy(g_outputbuf_pos + g_outputbuf, logInfoEx.spinfo, logInfo.dep * BITLEN);
		g_outputbuf_pos += logInfo.dep * BITLEN;
#else
		logInfo.dep = STACK_DEP;
		memcpy(g_outputbuf_pos + g_outputbuf, logInfoEx.lp, logInfo.dep * BITLEN);
		g_outputbuf_pos += logInfo.dep * BITLEN;
#endif
	}
	else
	{
		memcpy(g_outputbuf_pos + g_outputbuf, &logInfo, ONE_BUF_SIZE_FREE);
		g_outputbuf_pos += ONE_BUF_SIZE_FREE;
	}

	if(g_outputbuf_pos > 0 && (currtime > g_lasttime + 30 || (g_outputbuf_pos >= TOTAL_LEN)))
	{
		//fprintf(stdout,"[%s:%d][tid:%ld] type:%d stacktrace4 tid: %d get global waite write\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), logInfo.type, tid);
		pthread_mutex_lock(&mwGlobalMutexWrite);
		//fprintf(stdout,"[%s:%d][tid:%ld] type:%d stacktrace5 tid: %d get global get write\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), logInfo.type, tid);
		// 交换g_writebuf 和 g_outputbuf指针的值，不交换内容
		char * tempbuf = g_writebuf;
		g_writebuf = g_outputbuf;
		g_writelen = g_outputbuf_pos;
		g_outputbuf = tempbuf;
		pthread_mutex_unlock(&mwGlobalMutexWrite);
		//fprintf(stdout, "[%s:%d][tid:%ld] type:%d stacktrace6 tid: %d get global release write\n", __FUNCTION__, __LINE__, syscall(SYS_gettid), logInfo.type, tid);
		pthread_cond_broadcast(&cond);
		g_outputbuf_pos = 0;
		g_lasttime = currtime;
	}
	//fprintf(stdout,"[%s:%d][tid:%ld] type:%d stacktrace7 tid: %d get global release write\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), logInfo.type, tid);
	pthread_mutex_unlock(&mwGlobalMutexMemTrace);
	//fprintf(stdout,"[%s:%d][tid:%ld] type:%d stacktrace8 tid: %d release global release write\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), logInfo.type, tid);
	
	return;
}

/**
 * 根据申请的内存指针地址，获取内存申请的大小
*/
size_t getMallocSize(void *ptr)
{
	size_t size = 0u;
	if (ptr == NULL)
	{
		return size;
	}
	size = malloc_usable_size(ptr);
	return size;
}

#ifdef USE_ALL
#define InsertTraceFree(typeParam, ptrParam) \
{\
	if (g_lastswitch != 0 && gst_in_malloc == 0) \
	{\
		MemLogInfoEx logInfoEx;\
		MemLogInfo &logInfo = logInfoEx.logInfo;\
		logInfo.type = typeParam;\
		logInfo.ptr = ptrParam;\
		stacktrace(logInfoEx);\
	}\
}
#elif defined(USE_SIZE)
#define InsertTraceFree(typeParam, ptrParam) \
{\
	if (g_traceSize != 0 && gst_in_malloc == 0) \
	{\
		size_t size = getMallocSize(ptrParam);\
		if (size >= g_traceSize)\
		{\
			MemLogInfoEx logInfoEx;\
			MemLogInfo &logInfo = logInfoEx.logInfo;\
			logInfo.type = typeParam;\
			logInfo.ptr = ptrParam;\
			stacktrace(logInfoEx);\
		}\
	}\
}
#else
#define InsertTraceFree(typeParam, ptrParam)
#endif

#ifdef USE_ALL
#define Contition() (g_lastswitch != 0)
#elif defined(USE_SIZE)
#define Contition(sizeParamTemp) (g_traceSize != 0 && sizeParamTemp > g_traceSize)
#endif

#ifdef USE_BACKTRACE
#define Trace() logInfo.dep = backtrace(logInfoEx.spinfo, STACK_DEP) ;\
		/*fprintf(stdout,"zl:[%s:%d][tid:%ld]type:%d gst_in_malloc1:%d dep:%d\n", __FUNCTION__, __LINE__, gettid(), typeParam, gst_in_malloc, logInfo.dep);*/\
		/*char **strs = backtrace_symbols(logInfoEx.spinfo, logInfo.dep);*/ \
		/*for (int i = 0; i < logInfo.dep; i++)*/\
		/*{*/\
		/*	fprintf(stdout,"[%s:%d][tid:%ld] address %d: %p\n", __FUNCTION__, __LINE__, gettid(), i, logInfoEx.spinfo[i]);*/\
		/*	fprintf(stdout,"[%s:%d][tid:%ld] frame %d: %s\n", __FUNCTION__, __LINE__, gettid(), i, strs[i]);*/\
		/*}*/\
		/*free(strs);*/
#else
#define Trace() logInfoEx.lp = &result;
#endif

#if defined(USE_ALL) || defined(USE_SIZE)
#define InsertTraceMalloc(typeParam, ptrParam, sizeParam) \
{\
	if (Contition(sizeParam))\
	{\
		/*fprintf(stdout,"[%s:%d][tid:%ld]type:%d gst_in_malloc:%d\n", __FUNCTION__, __LINE__, gettid(), typeParam, gst_in_malloc);*/\
		if (gst_in_malloc == 0)\
		{\
			gst_in_malloc = 1;\
			MemLogInfoEx logInfoEx;\
			MemLogInfo &logInfo = logInfoEx.logInfo;\
			logInfo.type = typeParam;\
			logInfo.ptr = ptrParam;\
			logInfo.ptrlr = __builtin_return_address(0);\
			logInfo.size = sizeParam;\
			/*fprintf(stdout,"[%s:%d][tid:%ld]type:%d gst_in_malloc0:%d\n", __FUNCTION__, __LINE__, gettid(), typeParam, gst_in_malloc);*/\
			Trace();\
			stacktrace(logInfoEx);\
			/*fprintf(stdout,"[%s:%d][tid:%ld]type:%d gst_in_malloc2:%d\n", __FUNCTION__, __LINE__, gettid(), typeParam, gst_in_malloc);*/\
			gst_in_malloc = 0;\
		}\
	}\
}

#define InsertTraceReMalloc(typeParam, ptrParam, ptrxParam, sizeParam) \
{\
	if (Contition(sizeParam))\
	{\
		if (gst_in_malloc == 0)\
		{\
			gst_in_malloc = 1;\
			MemLogInfoEx logInfoEx;\
			MemLogInfo &logInfo = logInfoEx.logInfo;\
			logInfo.type = typeParam;\
			logInfo.ptr = ptrParam;\
			logInfo.ptrx = ptrxParam;\
			logInfo.ptrlr = __builtin_return_address(0);\
			logInfo.size = sizeParam;\
			/*fprintf(stdout,"[%s:%d][tid:%ld]gst_in_malloc0:%d\n", __FUNCTION__, __LINE__, gettid(), gst_in_malloc);*/\
			Trace();\
			stacktrace(logInfoEx);\
			gst_in_malloc = 0;\
		}\
	}\
}
#else
#define InsertTraceMalloc(typeParam, ptrParam, sizeParam)
#define InsertTraceReMalloc(typeParam, ptrParam, ptrxParam, sizeParam)
#endif

#ifdef USE_LARGE
#define InsertTraceMallocBig(typeParam, ptrParam, sizeParam) \
{\
	if (g_bigSize != 0 && size >= g_bigSize)\
	{\
		/*fprintf(stdout,"[%s:%d][tid:%ld]type:%d gst_in_malloc:%d\n", __FUNCTION__, __LINE__, gettid(), typeParam, gst_in_malloc);*/\
		if (gst_in_malloc == 0)\
		{\
			gst_in_malloc = 1;\
			MemLogInfoEx logInfoEx;\
			MemLogInfo &logInfo = logInfoEx.logInfo;\
			logInfo.type = typeParam;\
			logInfo.ptr = ptrParam;\
			logInfo.ptrlr = __builtin_return_address(0);\
			logInfo.size = sizeParam;\
			/*fprintf(stdout,"[%s:%d][tid:%ld]type:%d gst_in_malloc0:%d\n", __FUNCTION__, __LINE__, gettid(), typeParam, gst_in_malloc);*/\
			Trace();\
			stacktrace(logInfoEx);\
			/*fprintf(stdout,"[%s:%d][tid:%ld]type:%d gst_in_malloc2:%d\n", __FUNCTION__, __LINE__, gettid(), typeParam, gst_in_malloc);*/\
			gst_in_malloc = 0;\
		}\
	}\
}
#else
#define InsertTraceMallocBig(typeParam, ptrParam, sizeParam)
#endif


extern "C" void * malloc(size_t size)
{
	void* result = NULL;

	InsertTraceMallocBig(MEMOP_MALLOC_BIG, NULL, size);
	result = (void*)__libc_malloc(size);
	InsertTraceMalloc(MEMOP_MALLOC, result, size);
	return result;
}

extern "C" void free(void *ptr)
{
	if (ptr == NULL)
	{
		return;
	}
	InsertTraceFree(MEMOP_FREE, ptr);
	__libc_free(ptr);
	return;
}


extern "C" void *calloc(size_t nmemb, size_t size)
{
	void* result = NULL;
	InsertTraceMallocBig(MEMOP_CALLOC_BIG, NULL, size);
	result = (void*)__libc_calloc(nmemb, size);
	InsertTraceMalloc(MEMOP_CALLOC, result, nmemb*size);
	return result;
}

extern "C" void *realloc(void *ptr, size_t size)
{
	void* result = NULL;
	InsertTraceMallocBig(MEMOP_REALLOC_BIG, NULL, size);
	result = (void*)__libc_realloc(ptr, size);
	InsertTraceReMalloc(MEMOP_REALLOC, result, ptr, size);
	return result;
}

void * operator new(size_t size)
{
	void * result = NULL;
	InsertTraceMallocBig(MEMOP_NEW_BIG, NULL, size);
	result = (void*)__libc_malloc(size);
	InsertTraceMalloc(MEMOP_NEW, result, size);
	return result;
}
void * operator new (size_t size, const std::nothrow_t&)
{
	void * result = NULL;
	InsertTraceMallocBig(MEMOP_NEW_NOTHROW_BIG, NULL, size);
	result = (void*)__libc_malloc(size);
	InsertTraceMalloc(MEMOP_NEW_NOTHROW, result, size);
	return result;
}

void * operator new[](size_t size)
{
	void * result = NULL;
	InsertTraceMallocBig(MEMOP_NEW_ARRAY_BIG, NULL, size);
	result = (void*)__libc_malloc(size);
	InsertTraceMalloc(MEMOP_NEW_ARRAY, result, size);
	return result;
}

void operator delete(void* ptr) _GLIBCXX_USE_NOEXCEPT 
{
	if(ptr != NULL)
	{
		InsertTraceFree(MEMOP_DELETE, ptr);
		__libc_free(ptr);
	}
	return;
}

void operator delete[](void *ptr) _GLIBCXX_USE_NOEXCEPT
{
	if(ptr != NULL)
	{
		InsertTraceFree(MEMOP_DELETE_ARRAY, ptr);
		__libc_free(ptr);
	}
	return;
}


