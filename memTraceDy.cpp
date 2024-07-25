
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
	MEMOP_STRDUP = 9,
}ENUM_MEMOPTYPE;


#ifdef BACKTRACE
// 使用backtrace获取栈信息所记录的深度，默认10
#if defined(USE_DEP) && (USE_DEP > 0)
const int STACK_DEP = USE_DEP;
#else
const int STACK_DEP = 10;
#endif
#else
const int STACK_DEP = 10;
#endif


#define OUTPUT_BUFSIZE_LAST (1024*1024)

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
	void *spinfo[STACK_DEP];
} MemLogInfo;

const int ONE_BUF_SIZE_FREE = sizeof(MemLogInfo) - sizeof(void *) * STACK_DEP -  sizeof(void *) * 2;
const int ONE_BUF_SIZE_MALLOC = sizeof(MemLogInfo) - sizeof(void *) * STACK_DEP -  sizeof(void *) ;
const int ONE_BUF_SIZE_REMALLOC = sizeof(MemLogInfo) - sizeof(void *) * STACK_DEP;
const int ONE_SPINFO_SIZE = (512);
const int ONE_SPINFO_ITEM = (ONE_SPINFO_SIZE/8);

const int ONE_ROW_SP_ITEM = (64);
const int TOTAL_LEN = OUTPUT_BUFSIZE_LAST - sizeof(MemLogInfo);

const int BITLEN = sizeof(void *);

static const char* pathList = "/root/mount/share/memtrace/list";
static const char* pathMap = "/root/mount/share/memtrace/map";
static const char* pathParam = "/root/mount/share/memtrace/param";

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

	char name[128];
	struct tm t;
	time_t tt;
	time(&tt);
	localtime_r(&tt, &t);
	int len = sprintf(name, "%s/maps-%04d%02d%02d_%02d%02d%02d.txt", pathMap, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
	name[len] = '\0';
	FILE *stream;
	char cmd[128] = {0};
	len = sprintf(cmd, "cat /proc/%d/maps > %s", pid, name);
	// cmd[len] = '\0';
	cmd[strlen(cmd) - 1] = '\0';
	fprintf(stdout, "[%s:%d]cmd:%s\n", __FUNCTION__, __LINE__, cmd);
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
	fprintf(stdout,"--zl:start threadFunction--. g_writeThread:%d\n", g_writeThread);
	pthread_mutex_lock(&mwGlobalMutexWrite);
    while (1) {
        printf("Thread %d waiting...\n", g_writeThread);
        pthread_cond_wait(&cond, &mwGlobalMutexWrite);
        printf("Thread %d woken up\n", g_writeThread);
		SaveTraceInfo(LOGFILEOP_WRITE, g_writebuf, g_writelen);
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
void ReadParam(int &isStart , int & trace, size_t traceSize, int &traceTid)
{
	isStart = 0;
	traceTid = 0;
	traceSize = 0u;
	trace = 0;
	FILE * file = NULL;
	file = fopen(pathParam, "r");
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
			sscanf(buf, "%d %d %d %d", &isStart, &trace, &traceSize, &traceTid);
		}
		fclose(file);
		file = NULL;
	}
	fprintf(stdout,"[%s:%d]memtrace_param:%d %d %d %d.\n",__FUNCTION__, __LINE__, isStart,trace, traceSize, traceTid);
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
		// 第一次启动开关打开，创建内存信息写入输出缓存
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

/**
 * purpose: 读取memtrace_param参数线程，更新是否开启内存跟踪，是否跟踪栈信息，跟踪申请的内存大小，跟踪线程id等参数信息
*/
void* threadFunction(void* arg) {
    //int id = *(int*)arg; // 获取传递的参数
	
	fprintf(stdout,"--zl:start read param--.\n");
	
	while(1)
	{
		int lastswitch = 0;
		int traceTid = 0;
		size_t traceSize = 0u;
		int trace = 0;
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

		// 30s读取一次
		sleep(30);
	}
	fprintf(stdout,"--zl:end read param--.\n");
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


/**
 * purpose: 进程启动前初始化内存跟踪信息
 * param:
 * return:
*/
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

/**
 * purpose: 初始化内存跟踪信息
 * param:
 * return:
*/
void MemTraceInit()
{
	fprintf(stdout,"[%s:%d] MemTraceInit 1.\n", __FUNCTION__, __LINE__);
	int ret = 0;
	printMap(getpid());
	//ret = pthread_mutex_init(&mwGlobalMutexMemTrace, NULL);
	fprintf(stdout,"[%s:%d] MemTraceInit 2. ret: %d.\n", __FUNCTION__, __LINE__, ret);
}

/**
 * purpose: 保存内存跟踪信息
 * param: optype 操作类型 buf 内存跟踪信息 len 内存跟踪信息长度
 * return:
*/
void SaveTraceInfo(int optype, void * buf, int len)
{
	fprintf(stdout,"[%s:%d][tid:%d]SaveTraceInfo start\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));
	static int total = 0;
	static unsigned int no = 0u;

	if (LOGFILEOP_CLOSE == optype)
	{
		if (g_file != NULL)
		{
			fprintf(stdout,"[%s:%d][tid:%d]close file no:%d\n", __FUNCTION__, __LINE__, syscall(SYS_gettid), no);
			total = 0;
			fflush(g_file);
			fclose(g_file);
			g_file = NULL;
		}
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
	//fprintf(stdout,"[%s:%d][tid:%d]SaveTraceInfo start 1\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));
	if (NULL == g_file)
	{
		char name[128];
		struct tm t;
		time_t tt;
		time(&tt);
		localtime_r(&tt, &t);
		sprintf(name, "%s/memery-%04d%02d%02d_%02d%02d%02d-%u.log", pathList
			, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec
			, no);
		//fprintf(stdout,"[%s:%d][tid:%d]SaveTraceInfo start 4\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));
		g_file = fopen(name, "a");
		//fprintf(stdout,"[%s:%d][tid:%d]SaveTraceInfo start 5\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));
		if (NULL == g_file)
		{
			fprintf(stdout,"[%s:%d][tid:%d]open %s failed.\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), name);
			return;
		}
		fprintf(stdout,"[%s:%d][tid:%d]create file no:%d\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), no);
		no++;
	}
	// 1表示写入文件数据基本单元大小，len表示写入数据基本单元个数
	fwrite(buf, 1, len, g_file);
	//fflush(g_file);
	total += len;
	return;
}

/**
 * purpose: 写入内存跟踪信息
 * param:
 * return:
*/
// void stacktrace(unsigned int type, void *ptr, void *ptrx, unsigned long ptrlr, size_t size)
// {
// 	if (s_init==0)
// 	{
// 		return;
// 	}
//  	long i;
// 	long* sp = &i;
	
// 	time_t tt;
// 	time(&tt);

// 	unsigned int currtime = (unsigned int)tt;
// 	//fprintf(stdout,"[%s:%d][tid:%d]starttime is %u\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), currtime);

// 	if(0 == g_lastswitch)
// 	{
// 		return;
// 	}
	

// 	unsigned int tid = syscall(SYS_gettid);
// 	if (tid == g_writeThread)
// 	{
// 		return;
// 	}
	
// 	fprintf(stdout,"[%s:%d][tid:%d]currtime is %u\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), currtime);

// 	//printStackTrace();

// 	void *addresses[STACK_DEP];
// 	int frame_count = 0;
// 	in_malloc = 1;
// 	frame_count = backtrace(addresses, STACK_DEP);
// 	for (int i = 0; i < frame_count; i++)
// 	{
// 		fprintf(stdout,"[%s:%d] frame %d: %p\n", __FUNCTION__, __LINE__, i, addresses[i]);
// 	}

// 	if (g_traceTid > 0 && tid != g_traceTid)
// 	{
// 		return;
// 	}
// 	if (type >= MEMOP_MALLOC && g_traceSize > 0u && size != g_traceSize)
// 	{
// 		return;
// 	}
	
// 	fprintf(stdout,"[%s:%d][tid:%d]stacktrace tid: %ld\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), tid);

// 	if (g_outputbuf == NULL)
// 	{
// 		return;
// 	}
	
// 	pthread_mutex_lock(&mwGlobalMutexMemTrace);
// 	//fprintf(stdout,"[%s:%d][tid:%d]stacktrace start\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));

// 	if(g_outputbuf_pos > 0 && (currtime > g_lasttime + 30 || (g_outputbuf_pos + ONE_BUF_SIZE + 512 >= OUTPUT_BUFSIZE_LAST)))
// 	{
// 		pthread_mutex_lock(&mwGlobalMutexWrite);
// 		memcpy(g_writebuf, g_outputbuf, g_outputbuf_pos);
// 		g_writelen = g_outputbuf_pos;
// 		pthread_cond_broadcast(&cond);
// 		pthread_mutex_unlock(&mwGlobalMutexWrite);

// 		//SaveTraceInfo(LOGFILEOP_WRITE, g_outputbuf, g_outputbuf_pos);
// 		g_outputbuf_pos = 0;
// 		g_lasttime = currtime;
// 	}
// 	//fprintf(stdout,"[%s:%d][tid:%d]stacktrace start 1\n",__FUNCTION__, __LINE__, syscall(SYS_gettid));
// 	//if((g_outputbuf_pos > 0) && (g_outputbuf_pos + ONE_BUF_SIZE >= OUTPUT_BUFSIZE_LAST))
// 	//{
// 	//	SaveTraceInfo(LOGFILEOP_WRITE, g_outputbuf, g_outputbuf_pos);
// 		// memcpy(g_outputbuf, &currtime, 4);
// 		// memcpy(g_outputbuf+4, &type, 4);
// 		// memcpy(g_outputbuf+8, &tid, 4);
// 		// memcpy(g_outputbuf+16, &size, 8);
// 		// memcpy(g_outputbuf+24, &ptr, 8);
// 		// memcpy(g_outputbuf+32, &ptrx, 8);
// 		// memcpy(g_outputbuf+40, &ptrlr, 8);
// 		// if (type >= MEMOP_MALLOC && type <= MEMOP_NEW_ARRAY)
// 		// {
// 		// 	if(g_trace != 0)
// 		// 	{
// 		// 		memcpy(g_outputbuf+48, sp, 512);
// 		// 		g_outputbuf_pos = ONE_BUF_WITHSP_SIZE;
// 		// 	}
// 		// 	else
// 		// 	{
// 		// 		g_outputbuf_pos = ONE_BUF_SIZE;	
// 		// 	}
// 		// }
// 		// else
// 		// {
// 		// 	g_outputbuf_pos = ONE_BUF_SIZE;
// 		// }
// 		// g_lasttime = (unsigned int)tt;
// 	//}
// 	//else
// 	// {
// 		memcpy(g_outputbuf+g_outputbuf_pos, &currtime, 4);
// 		memcpy(g_outputbuf+g_outputbuf_pos+4, &type, 4);
// 		memcpy(g_outputbuf+g_outputbuf_pos+8, &tid, 4);
// 		memcpy(g_outputbuf+g_outputbuf_pos+16, &size, 8);
// 		memcpy(g_outputbuf+g_outputbuf_pos+24, &ptr, 8);
// 		memcpy(g_outputbuf+g_outputbuf_pos+32, &ptrx, 8);
// 		memcpy(g_outputbuf+g_outputbuf_pos+40, &ptrlr, 8);
// 		//memcpy(g_outputbuf+g_outputbuf_pos+40, &dep, 8);
// 		if (type >= MEMOP_MALLOC && type <= MEMOP_NEW_ARRAY)
// 		{
// 			if(g_trace != 0)
// 			{
// 				memcpy(g_outputbuf+g_outputbuf_pos+48, &sp, 512);
// 				g_outputbuf_pos += ONE_BUF_WITHSP_SIZE;	
// 			}
// 			else
// 			{
// 				g_outputbuf_pos += ONE_BUF_SIZE;
// 			}
// 		}
// 		else
// 		{
// 			g_outputbuf_pos += ONE_BUF_SIZE;
// 		}
// 	// 	if(currtime > g_lasttime + 30)
// 	// 	{
// 	// 		SaveTraceInfo(LOGFILEOP_WRITE, g_outputbuf, g_outputbuf_pos);
// 	// 		g_outputbuf_pos = 0;
// 	// 		g_lasttime = (unsigned int)tt;
// 	// 	}
// 	// }
// 	//fprintf(stdout," [thread:%d]stacktrace end\n", syscall(SYS_gettid));
// 	pthread_mutex_unlock(&mwGlobalMutexMemTrace);
// 	return;
// }

void stacktrace(MemLogInfo &logInfo)
{
	time_t tt;
	time(&tt);

	unsigned int currtime = (unsigned int)tt;
	logInfo.currtime = currtime;
	//fprintf(stdout,"[%s:%d][tid:%d]starttime is %u\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), currtime);

	unsigned int tid = syscall(SYS_gettid);
	logInfo.tid = tid;
	// if (tid == g_writeThread)
	// {
	// 	return;
	// }
	
	//fprintf(stdout,"[%s:%d][tid:%d]currtime is %u\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), currtime);

	//printStackTrace();

	if (g_traceTid > 0 && tid != g_traceTid)
	{
		return;
	}
	
	fprintf(stdout,"[%s:%d][tid:%d]stacktrace tid: %ld\n",__FUNCTION__, __LINE__, syscall(SYS_gettid), tid);
	
	pthread_mutex_lock(&mwGlobalMutexMemTrace);
	if (logInfo.type >= MEMOP_REALLOC)
	{
		memcpy(g_outputbuf_pos + g_outputbuf, &logInfo, ONE_BUF_SIZE_REMALLOC + logInfo.dep * BITLEN);
		g_outputbuf_pos += ONE_BUF_SIZE_REMALLOC + logInfo.dep * BITLEN;

	}
	else if (logInfo.type >= MEMOP_MALLOC)
	{
		memcpy(g_outputbuf_pos + g_outputbuf, &logInfo, ONE_BUF_SIZE_MALLOC);
		g_outputbuf_pos += ONE_BUF_SIZE_MALLOC;
		memcpy(g_outputbuf_pos + g_outputbuf, logInfo.spinfo, logInfo.dep * BITLEN);
		g_outputbuf_pos += logInfo.dep * BITLEN;
	}
	else
	{
		memcpy(g_outputbuf_pos + g_outputbuf, &logInfo, ONE_BUF_SIZE_FREE);
		g_outputbuf_pos += ONE_BUF_SIZE_FREE;
	}

	if(g_outputbuf_pos > 0 && (currtime > g_lasttime + 30 || (g_outputbuf_pos >= TOTAL_LEN)))
	{
		pthread_mutex_lock(&mwGlobalMutexWrite);
		// 交换g_writebuf 和 g_outputbuf指针的值，不交换内容
		char * tempbuf = g_writebuf;
		g_writebuf = g_outputbuf;
		g_writelen = g_outputbuf_pos;
		g_outputbuf = tempbuf;
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mwGlobalMutexWrite);
		g_outputbuf_pos = 0;
		g_lasttime = currtime;
	}
	pthread_mutex_unlock(&mwGlobalMutexMemTrace);
	
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


#define InsertTraceFree(typeParam, ptrParam) \
{\
	if (g_lastswitch != 0)\
	{\
		if (g_traceSize != 0)\
		{\
			size_t size = malloc_usable_size(ptrParam);\
			if (size >= g_traceSize)\
			{\
				MemLogInfo logInfo;\
				logInfo.type = typeParam;\
				logInfo.ptr = ptrParam;\
				stacktrace(logInfo);\
			}\
		}\
		else\
		{\
			MemLogInfo logInfo;\
			logInfo.type = typeParam;\
			logInfo.ptr = ptrParam;\
			stacktrace(logInfo);\
		}\
	}\
}

#define InsertTraceMalloc(typeParam, ptrParam, sizeParam) \
{\
	if (g_lastswitch != 0 && (g_traceSize == 0 || sizeParam > g_traceSize))\
	{\
		if (gst_in_malloc != 0)\
		{\
			return (void*)__libc_malloc(sizeParam);\
		}\
		gst_in_malloc = 1;\
		MemLogInfo logInfo;\
		logInfo.type = typeParam;\
		logInfo.ptr = ptrParam;\
		logInfo.ptrlr = __builtin_return_address(0);\
		logInfo.size = sizeParam;\
		logInfo.dep = backtrace(logInfo.spinfo, STACK_DEP);\
		for (int i = 0; i < logInfo.dep; i++)\
		{\
			fprintf(stdout,"[%s:%d] address %d: %p\n", __FUNCTION__, __LINE__, i, logInfo.spinfo[i]);\
		}\
		stacktrace(logInfo);\
		gst_in_malloc = 0;\
	}\
}

#define InsertTraceCaMalloc(typeParam, ptrParam, sizeParam) \
{\
	if (g_lastswitch != 0 && (g_traceSize == 0 || sizeParam > g_traceSize))\
	{\
		MemLogInfo logInfo;\
		logInfo.type = typeParam;\
		logInfo.ptr = ptrParam;\
		logInfo.ptrlr = __builtin_return_address(0);\
		logInfo.size = sizeParam;\
		logInfo.dep = backtrace(logInfo.spinfo, STACK_DEP);\
		for (int i = 0; i < logInfo.dep; i++)\
		{\
			fprintf(stdout,"[%s:%d] address %d: %p\n", __FUNCTION__, __LINE__, i, logInfo.spinfo[i]);\
		}\
		stacktrace(logInfo);\
	}\
}

#define InsertTraceReMalloc(typeParam, ptrParam, ptrxParam, sizeParam) \
{\
	if (g_lastswitch != 0 && (g_traceSize == 0 || sizeParam > g_traceSize) && ptrParam != ptrxParam)\
	{\
		MemLogInfo logInfo;\
		logInfo.type = typeParam;\
		logInfo.ptr = ptrParam;\
		logInfo.ptrx = ptrxParam;\
		logInfo.ptrlr = __builtin_return_address(0);\
		logInfo.size = sizeParam;\
		logInfo.dep = backtrace(logInfo.spinfo, STACK_DEP);\
		for (int i = 0; i < logInfo.dep; i++)\
		{\
			fprintf(stdout,"[%s:%d] address %d: %p\n", __FUNCTION__, __LINE__, i, logInfo.spinfo[i]);\
		}\
		stacktrace(logInfo);\
	}\
}

extern "C" void __libc_free(void*);
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


// 定义一个线程本地变量gst_in_malloc，用于标识当前线程是否在malloc中
static __thread int gst_in_malloc = 0;	
extern "C" void *__libc_malloc(size_t size);
extern "C" void * malloc(size_t size)
{
	void* result = NULL;
	result = (void*)__libc_malloc(size);
	InsertTraceMalloc(MEMOP_MALLOC, result, size);
	return result;
}

extern "C" void *__libc_calloc(size_t, size_t);
extern "C" void *calloc(size_t nmemb, size_t size)
{
	void* result = NULL;
	result = (void*)__libc_calloc(nmemb, size);
	InsertTraceCaMalloc(MEMOP_CALLOC, result, nmemb*size);
	return result;
}

extern "C" void *__libc_realloc(void*, size_t);
extern "C" void *realloc(void *ptr, size_t size)
{
	void* result = NULL;
	result = (void*)__libc_realloc(ptr, size);
	InsertTraceReMalloc(MEMOP_REALLOC, result, ptr, size);
	return result;
}

void * operator new(size_t size)
{
	void * result = NULL;
	result = (void*)__libc_malloc(size);
	InsertTraceCaMalloc(MEMOP_NEW, result, size);
	return result;
}
void * operator new (size_t size, const std::nothrow_t&)
{
	void * result = NULL;
	result = (void*)__libc_malloc(size);
	InsertTraceCaMalloc(MEMOP_NEW_NOTHROW, result, size);
	return result;
}

void * operator new[](size_t size)
{
	void * result = NULL;
	result = (void*)__libc_malloc(size);
	InsertTraceCaMalloc(MEMOP_NEW_ARRAY, result, size);
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
	unsigned long lrp = 0u;
	if(ptr != NULL)
	{
		InsertTraceFree(MEMOP_DELETE_ARRAY, ptr);
		__libc_free(ptr);
	}
	return;
}


