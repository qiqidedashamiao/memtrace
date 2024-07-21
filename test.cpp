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
        std::cout << "线程 " << threadId << " 申请 " << size << " 字节内存" << std::endl;
        char* buffer = new char[size];
        std::cout << "线程 " << threadId << " 分配了 " << size << " 字节内存" << std::endl;

        for (int i = 0; i < size; i++) {
            buffer[i] = i;
        }

            // 模拟一些处理时间
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));

        std::cout << "线程 " << threadId << " 开始释放内存" << std::endl;
        // 释放内存
        delete[] buffer;
        std::cout << "线程 " << threadId << " 释放了内存" << std::endl;

        // 模拟一些处理时间
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "线程 " << threadId << " 结束" << std::endl;
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
void foo() {
    //printStackTrace();
    printf("[foo] start malloc\n");
    int* ptr = (int*)malloc(10 * sizeof(int));
    printf("[foo] end malloc\n");
    free(ptr);
    printf("[foo] end free\n");
}
// 用于接收用户输入并结束程序的函数
void monitorInput() {
    int input;
    //foo();
    while (true) {
        std::cin >> input;
        if (input == 0) {
            {
                std::lock_guard<std::mutex> lk(cv_m);
                running = false;
            }
            std::cout << "input " << input << std::endl;
            cv.notify_all();
            break;
        }
    }
}

void test() {
    printf("1\n");
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
    printf("2\n");
    sleep(1);
    printf("3\n");
}

int main(int argc, char** argv) {


    foo();
    //test();
    return 0;
    const int numThreads = 1; // 线程数量
    std::vector<std::thread> threads;

    // 创建线程
    for (int i = 0; i < numThreads; ++i) {
        threads.push_back(std::thread(allocateAndFreeMemory, i));
    }

    // 创建监控输入的线程
    std::thread inputThread(monitorInput);

    // 等待所有线程完成
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    if (inputThread.joinable()) {
        inputThread.join();
    }

    return 0;
}
