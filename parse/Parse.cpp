#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <map>



const int ONE_BUF_SIZE = (48);
const int ONE_SPINFO_SIZE = (512);
const int ONE_BUF_WITHSP_SIZE = (ONE_BUF_SIZE + ONE_SPINFO_SIZE);
const int ONE_SPINFO_ITEM = (ONE_SPINFO_SIZE/8);

const int ONE_ROW_SP_ITEM = (64);


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

typedef struct MemLogInfo{
	unsigned int currtime;
	int type;
	int tid;
	int resover;
	unsigned long size;
	unsigned long ptr;
	unsigned long ptrx;
	unsigned long ptrlr;
	unsigned long spinfo[ONE_SPINFO_ITEM];
}MemLogInfo;

void output_info(const char * soniaPath, int isAddr2Symbol);
void parse_logfile(const     char * name, int isAddr2Symbol, int logDetail = 0, int isFullPath = 0);
void get_logfilelist(const char * name, int isAddr2Symbol, int logDetail = 0, int isFullPath = 0);
void get_spfile(const     char * name, int isFullPath, const char * soniaPath);


typedef std::map<long, MemLogInfo> AddrMap;
typedef std::map<long, MemLogInfo>::value_type AddrMapType;
typedef std::map<long, MemLogInfo>::iterator AddrMapIter;

typedef std::map<int, unsigned int> TidStatMap;
typedef std::map<int, unsigned int>::value_type TidStatMapType;
typedef std::map<int, unsigned int>::iterator TidStatMapIter;

typedef std::map<unsigned long, unsigned int> LRStatMap;
typedef std::map<unsigned long, unsigned int>::value_type LRStatMapType;
typedef std::map<unsigned long, unsigned int>::iterator LRStatMapIter;

AddrMap g_addrMap;

unsigned long int g_addNum = 0llu;
unsigned long int g_delNum = 0llu;
unsigned long int g_deladdNum = 0llu;
unsigned long int g_itemNum = 0llu;
unsigned long int g_errorItemNum1 = 0llu;
unsigned long int g_errorItemNum2 = 0llu;


int main(int argc, char * argv[])
{
	printf("hello argc: %d\n", argc);
	for (int i = 1; i < argc; i++)
	{
		printf("hello argv[%d]: %s\n", i, argv[i]);
	}

	printf("mem_log_parse parseType(0) logListFileName showLogDetail addr2symbol isFullPath soniaPath\n");
	printf("mem_log_parse parseType(1) spInfoFileName isFullPath soniaPath\n");
	int parseType = atoi(argv[1]);

	if (0 == parseType)
	{
		if (argc < 7)
		{
			printf("invalid input param\n");
			return 0;
		}

		int logDetail = atoi(argv[3]);
		int isAddr2Symbol = atoi(argv[4]); 
		int isFullPath = atoi(argv[5]);
		get_logfilelist(argv[2], isAddr2Symbol, logDetail, isFullPath);
		output_info(argv[6], isAddr2Symbol);
		printf("itemNum: %lu;%lu;%lu; g_addNum: %lu; g_delNum: %lu; g_deladdNum: %lu.\n", g_itemNum, g_errorItemNum1, g_errorItemNum2, g_addNum, g_delNum, g_deladdNum);
	}
	else if(1 == parseType)
	{
		if (argc < 5)
		{
			printf("invalid input param\n");
			return 0;
		}
		int isFullPath = atoi(argv[3]); 
		get_spfile(argv[2], isFullPath, argv[4]);
	}
	else
	{
		return 0;
	}

	
	return 0;
}

void add_addr(MemLogInfo & info)
{
	AddrMapIter iter;
	iter = g_addrMap.find(info.ptr);
	if (iter != g_addrMap.end())
	{
		g_addrMap.erase(iter);
	}
	g_addrMap.insert(AddrMapType(info.ptr, info));
}

void addx_addr(MemLogInfo & info)
{
	AddrMapIter iter;
	iter = g_addrMap.find(info.ptr);
	if (iter != g_addrMap.end())
	{
		g_addrMap.erase(iter);
	}
	iter = g_addrMap.find(info.ptrx);
	if (iter != g_addrMap.end())
	{
		g_addrMap.erase(iter);
	}
	g_addrMap.insert(AddrMapType(info.ptr, info));
}

void del_addr(MemLogInfo & info)
{
	AddrMapIter iter;
	iter = g_addrMap.find(info.ptr);
	if (iter != g_addrMap.end())
	{
		g_addrMap.erase(iter);
	}
}

void addr2symbol(const char * soniaPath, void * addr, char * strSymbol, unsigned int bufSize)
{
	FILE * stream;
	char cmd[10240];
	
	//sprintf(cmd, "csky-abiv2-ux-linuxv3615-addr2line -a -e %s -f -C %p", soniaPath, addr);
	sprintf(cmd, "aarch64-himix210-linux-sd3403v100-v1-addr2line -a -e %s -f -C %p", soniaPath, addr);
	
	stream = popen(cmd, "r");
	fread(strSymbol, sizeof(char), bufSize, stream);
	pclose(stream);
	
	
}

void addrstr2symbol(const char * soniaPath, char *addrstr, char * strSymbol, unsigned int bufSize)
{
	FILE * stream;
	char cmd[10240];
	
	//sprintf(cmd, "csky-abiv2-ux-linuxv3615-addr2line -a -e %s -f -C %s", soniaPath, addrstr);
	sprintf(cmd, "aarch64-himix210-linux-sd3403v100-v1-addr2line -a -e %s -f -C %s", soniaPath, addrstr);
	printf("cmd %s\n", cmd);
	stream = popen(cmd, "r");
	fread(strSymbol, sizeof(char), bufSize, stream);
	pclose(stream);
	printf("strSymbol is %s\n",strSymbol);
	
}

void output_info(const char * soniaPath, int isAddr2Symbol)
{
	unsigned int total = 0u;
	TidStatMap tidStatMap;
	LRStatMap lrStatMap;
	int len = 0;
	int offset = 0;
	char szSPAddr[10240];
	char symbolBuf[40960];
	//infof("46885 g_addrMap size is %d\n",g_addrMap.size())
	for (AddrMapIter iter = g_addrMap.begin(); iter != g_addrMap.end(); iter++)
	{
		MemLogInfo &info = iter->second;
		total += info.size;
		printf("currtime:\t%u\ttype:\t%d\ttid:\t%d\tsize:\t%lu\tptr:\t%p\tptrx:\t%p\tptrlr:\t%p\t\n",
			(info.currtime), (info.type), (info.tid), (info.size), (void*)(info.ptr), (void*)(info.ptrx), (void*)(info.ptrlr));

		if (isAddr2Symbol)
		{
			offset = 0;
			memset(szSPAddr, 0, sizeof(szSPAddr));
			for (int i = 0; i < ONE_SPINFO_ITEM; i++)
			{
				void * p = (void *)(info.spinfo[i]);
				if (p)
				{
					len = sprintf(szSPAddr + offset, "\t%p", p);
				}
				else
				{
					len = sprintf(szSPAddr + offset, "\t0x0");
				}
				if (len > 0)
				{
					offset += len;
				}
				//printf("i: %d, offset :%d, len :%d\n", i, offset, len);
			}
			szSPAddr[offset] = 0;
			//printf("ok.\n");
			addrstr2symbol(soniaPath, szSPAddr, symbolBuf, sizeof(symbolBuf));
			printf("addrlist:%s\n", szSPAddr);
			printf("%s\n", symbolBuf);
		}
		else
		{
			for (int i = 0; i < ONE_SPINFO_ITEM; i++)
			{
				if (!(i%ONE_ROW_SP_ITEM))
				{
					printf("\n");
				}
				void * p = (void *)(info.spinfo[i]);
				if (p)
				{
					printf("\t%p", p);
				}
				else
				{
					printf("\t0x0");
				}
			}
			printf("\n");
		}

		
		TidStatMapIter tiditer;
		tiditer = tidStatMap.find(info.tid);
		if (tiditer != tidStatMap.end())
		{
			tiditer->second += info.size;
		}
		else
		{
			tidStatMap.insert(TidStatMapType(info.tid, info.size));
		}
		LRStatMapIter lriter;
		lriter = lrStatMap.find(info.ptrlr);
		if (lriter != lrStatMap.end())
		{
			lriter->second += info.size;
		}
		else
		{
			lrStatMap.insert(LRStatMapType(info.ptrlr, info.size));
		}

	}
	printf("all total: %u[B]\t = %u[KB]\t = %u[MB]\n", total, (total >> 10), (total >> 20));

	for (TidStatMapIter iter = tidStatMap.begin(); iter != tidStatMap.end(); iter++)
	{
		total = iter->second;
		printf("tid[%u]\t total: %u[B]\t = %u[KB]\t = %u[MB]\n", iter->first, total, (total >> 10), (total >> 20));
	}
	for (LRStatMapIter iter = lrStatMap.begin(); iter != lrStatMap.end(); iter++)
	{
		total = iter->second;
		printf("LR[%p]\t total: %u[B]\t = %u[KB]\t = %u[MB]\n", (void *)(iter->first), total, (total >> 10), (total >> 20));
		memset(symbolBuf, '\0', sizeof(symbolBuf));
		addr2symbol(soniaPath, (void *)(iter->first), symbolBuf, sizeof(symbolBuf));
		printf("\t\t\t\t %s\n", symbolBuf);
	}

}

void get_spfile(const char * name, int isFullPath, const char * soniaPath)
{
	char symbolBuf[4096];

	if (NULL == name)
	{
		printf("get_logfilelist name is null!\n");
		return;
	}

	printf("get_spfile name %s!\n", name);
	char szFileName[1024];
	if (isFullPath)
	{
		sprintf(szFileName, "%s", name);
	}
	else
	{
		sprintf(szFileName, "./%s", name);
	}
	printf("get_spfile name %s;\n", szFileName);
	
	FILE * file = NULL;
	file = fopen(szFileName, "r");
	if(file != NULL)
	{
		char * line = NULL;
		size_t len = 0;
		ssize_t read;
		char szSPAddr[ONE_ROW_SP_ITEM][64];
		while ((read = getline(&line, &len, file)) != -1) 
		{
			printf("Retrieved line of length %zu :\n", read);
			if ((read >= 1) && ('\n' == line[read-1]))
			{
				line[read-1] = '\0';
			}
			memset(szSPAddr, 0 , sizeof(szSPAddr));
			sscanf(line, "\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s"
				, szSPAddr[0], szSPAddr[1], szSPAddr[2], szSPAddr[3]
				, szSPAddr[4], szSPAddr[5], szSPAddr[6], szSPAddr[7]
				, szSPAddr[8], szSPAddr[9], szSPAddr[10], szSPAddr[11]
				, szSPAddr[12], szSPAddr[13], szSPAddr[14], szSPAddr[15]
				, szSPAddr[16], szSPAddr[17], szSPAddr[18], szSPAddr[19]
				, szSPAddr[20], szSPAddr[21], szSPAddr[22], szSPAddr[23]
				, szSPAddr[24], szSPAddr[25], szSPAddr[26], szSPAddr[27]
				, szSPAddr[28], szSPAddr[29], szSPAddr[30], szSPAddr[31]);
			for (int i = 0; i < ONE_ROW_SP_ITEM; i++)
			{
				if (strcmp("(nil)", szSPAddr[i]))
				{
					memset(symbolBuf, '\0', sizeof(symbolBuf));
					addrstr2symbol(soniaPath, szSPAddr[i], symbolBuf, sizeof(symbolBuf));
					printf("\t\t%s:\t\t %s\n", szSPAddr[i], symbolBuf);
				}
				else
				{
					printf("\t\t%s:\t\t NULL\n", szSPAddr[i]);
				}
			}
		}
		if (line)
		{
			free(line);
			line = NULL;
		}
	
		fclose(file);
		file = NULL;
	}
	return;
}
void get_logfilelist(const char * name, int isAddr2Symbol, int logDetail, int isFullPath)
{
	if (NULL == name)
	{
		printf("get_logfilelist name is null!\n");
		return;
	}

	printf("get_logfilelist name %s!\n", name);

	FILE * file = NULL;
	file = fopen(name, "r");
	if(file != NULL)
	{
		char * line = NULL;
		size_t len = 0;
		ssize_t read;
		while ((read = getline(&line, &len, file)) != -1) {
			printf("Retrieved line of length %zu :\n", read);
			if ((read >= 1) && ('\n' == line[read-1]))
			{
				line[read-1] = '\0';
			}			printf("%s\n", line);
			parse_logfile(line, isAddr2Symbol, logDetail, isFullPath);
		}
		if (line)
		{
			free(line);
			line = NULL;
		}
	
		fclose(file);
		file = NULL;
	}
	return;

}


void parse_logfile(const char * name, int isAddr2Symbol, int logDetail, int isFullPath)
{
	if (NULL == name)
	{
		printf("parse_logfile name is null!\n");
		return;
	}

	printf("parse_logfile name %s,\n", name);
	char szFileName[1024];
	if (isFullPath)
	{
		sprintf(szFileName, "%s", name);
	}
	else
	{
		sprintf(szFileName, "./%s", name);
	}
	printf("parse_logfile name %s\n", szFileName);
	
	FILE * file = NULL;
	file = fopen(szFileName, "r");
	if(file != NULL)
	{
		size_t len = 0;
		size_t spInfoLen = 0;
		MemLogInfo info;
		do
		{
			memset(&info, 0, sizeof(info));
			len = fread(&info, 1, ONE_BUF_SIZE, file);
			if (ONE_BUF_SIZE == len) 
			{
				if (logDetail)
				{
					printf("<Detail>\tcurrtime:\t%u\ttype:\t%d\ttid:\t%d\tsize:\t%lu\tptr:\t%p\tptrx:\t%p\tptrlr:\t%p\t\n",
						(info.currtime), (info.type), (info.tid), (info.size), (void*)(info.ptr), (void*)(info.ptrx), (void*)(info.ptrlr));
				}
				if (info.type >= MEMOP_FREE && info.type <= MEMOP_NEW_ARRAY)
				{
					if (info.type >= MEMOP_MALLOC)
					{
						spInfoLen = fread(info.spinfo, 1, ONE_SPINFO_SIZE, file);
						if (ONE_SPINFO_SIZE != spInfoLen) 
						{
							printf("error spInfoLen[%lu]\n", spInfoLen);
							g_errorItemNum2++;
							break;
						}
					}
					
					if (info.type >= MEMOP_MALLOC && info.type != MEMOP_REALLOC)
					{
						add_addr(info);
						g_addNum++;
					}
					else if(info.type == MEMOP_REALLOC)
					{
						addx_addr(info);
						g_deladdNum++;
					}
					else
					{
						del_addr(info);
						g_delNum++;
					}
					g_itemNum++;
				}
				else
				{
					printf("error info.type[%d]\n", info.type);
					g_errorItemNum1++;
				}
			}
			else if (len > 0)
			{
				printf("error len[%lu]\n", len);
				g_errorItemNum2++;
			}
			
		} while(ONE_BUF_SIZE == len);
	
		fclose(file);
		file = NULL;

		printf("parse_logfile item num : %lu; %lu; %lu.\n", g_itemNum, g_errorItemNum1, g_errorItemNum2);
	}
	else
	{
		printf("parse_logfile fopen error!\n");
	}
	return;
}
