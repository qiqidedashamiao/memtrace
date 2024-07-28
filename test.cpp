#include <iostream>
#include <thread>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <atomic>
#include <execinfo.h>
#include <condition_variable>
#include <dlfcn.h>
#include <cstdio>
#include <unistd.h>
#include <sys/syscall.h>

using namespace std;


#define gettid() syscall(SYS_gettid)

// 用于控制线程停止的全局标志和条件变量
std::atomic<bool> running(true);
std::condition_variable cv;
std::mutex cv_m;


char cache[1024*1024*100] = {2,2,3,4,5,6,7,8,9,10};
// 每个线程执行的函数
void allocateAndFreeMemory(int threadId) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)) + threadId);
    while (running) {
        // 随机分配内存大小
        //int size = std::rand() + 1; // 分配1到1024字节的内存
        int size = 1024; // 分配1到1024字节的内存
        std::cout << "线程 " << gettid() << " 申请 " << size << " 字节内存" << std::endl;
        char* buffer = new char[size];
        std::cout << "线程 " << gettid() << " 分配了 " << size << " 字节内存" << std::endl;

        for (int i = 0; i < size; i++) {
            buffer[i] = i;
        }
        //foo();

        // 模拟一些处理时间
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));

        std::cout << "线程 " << gettid() << " 开始释放内存" << std::endl;
        // 释放内存
        delete[] buffer;
        std::cout << "线程 " << gettid() << " 释放了内存" << std::endl;

        // 模拟一些处理时间
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
    std::cout << "线程 " << gettid() << " 结束" << std::endl;
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
    void* array[10];
    char **strings;
    printf("Stack trace start:\n");
    size_t size = backtrace(array, 10);
    printf("Stack trace size: %d\n", size);
    //strings = backtrace_symbols(array, size);
    for (size_t i = 0; i < size; i++) {
        //printSymbol(array[i]);
        //printf("Addr: %x\n", array[i]);
        //printf("Symbol: %s\n", strings[i]);
    }
}
void test();
void monitorInput();
void foo()
{
    //printStackTrace();
    printf("[foo] start malloc\n");
    int* ptr = (int*)malloc(10 * sizeof(int));
    printf("[foo] end malloc\n");
    free(ptr);
    printf("[foo] end free\n");
    test();
    monitorInput();
}
// 用于接收用户输入并结束程序的函数
void monitorInput() {
    int input;
    //foo();
    while (true)
    {
        std::cin >> input;
        if (input == 0)
        {
            {
                std::lock_guard<std::mutex> lk(cv_m);
                running = false;
            }
            std::cout << "[tid:" << gettid() << "]" << "input " << input << std::endl;
            cv.notify_all();
            break;
        }
    }
}


void test() {
    printf("start\n");
    const int STACK_DEP = 10;
    int frame_count = 0;
    void *addresses[10];
    char **strings;
    frame_count = backtrace(addresses, STACK_DEP); 
    strings = backtrace_symbols(addresses, frame_count);
    for (int i = 0; i < frame_count; i++) {
        printf("addr %d: %p\n", i, addresses[i]);
        printf("frame %d: %s\n", i, strings[i]);
    }
}

int main(int argc, char** argv) {


    cout << "[tid:" << gettid() << "]" << "main start" << endl;
    //foo();
    //test();
    //return 0;
    const int numThreads = 5; // 线程数量
    std::vector<std::thread> threads;

    cout << "[tid:" << gettid() << "]" << "main create thread" << endl;

    // 创建线程
    for (int i = 0; i < numThreads; ++i) {
        threads.push_back(std::thread(allocateAndFreeMemory, i));
    }
    
    cout << "[tid:" << gettid() << "]" << "main create thread end" << endl;
    // 创建监控输入的线程
    std::thread inputThread(monitorInput);
    
    cout << "[tid:" << gettid() << "]" << "main create monitor thread end" << endl;

    // 等待所有线程完成
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    if (inputThread.joinable()) {
        inputThread.join();
    }
    //int end = 0;
    //cin >> end;
    //cout << "main end" << endl;

    return 0;
}
