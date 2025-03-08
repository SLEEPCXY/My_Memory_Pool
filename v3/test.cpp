#include "MemoryPool.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>
#include <thread>

using namespace MyMemoryPool;
using namespace std::chrono;

// 计时器类
class Timer 
{
    high_resolution_clock::time_point start; // 定义一个高精度时钟点
public:
    Timer() : start(high_resolution_clock::now()) {} // 构造函数，初始化开始时间点为当前时间点
    
    double elapsed() 
    {
        auto end = high_resolution_clock::now(); // 获取当前时间点
        return duration_cast<microseconds>(end - start).count(); // 转换为毫秒
    }
};

// 性能测试类
class PerformanceTest 
{
private:
    // 测试统计信息
    // 定义一个结构体，用于存储测试统计信息
    struct TestStats 
    {
        // 内存池时间
        double memPoolTime{0.0};
        // 系统时间
        double systemTime{0.0};
        // 总分配次数
        size_t totalAllocs{0};
        // 总分配字节数
        size_t totalBytes{0};
    };

public:
    // 1. 系统预热
    static void warmup() 
    {
        // 预热内存系统
        std::cout << "Warming up memory systems...\n";
        std::vector<void*> warmupPtrs;
        
        // 预热内存池
        for (int i = 0; i < 1000; ++i) 
        {
            // 预热不同大小的内存块
            for (size_t size : {32, 64, 128, 256, 512}) {
                void* p = MemoryPool::allocate(size);
                warmupPtrs.push_back(p);
            }
        }
        
        // 释放预热内存
        for (void* ptr : warmupPtrs) 
        {
            MemoryPool::deallocate(ptr, 32);  // 使用默认大小
        }
        
        std::cout << "Warmup complete.\n\n";
    }

    // 2. 小对象分配测试
    static void testSmallAllocation() 
    {
        // 定义测试次数和对象大小
        constexpr size_t NUM_ALLOCS = 100000;
        constexpr size_t SMALL_SIZE = 32;
        
        // 输出测试信息
        std::cout << "\n小对象分配测试\nTesting small allocations (" << NUM_ALLOCS << " allocations of " 
                  << SMALL_SIZE << " bytes):" << std::endl;
        
        // 测试内存池
        {
            Timer t;
            std::vector<void*> ptrs;
            ptrs.reserve(NUM_ALLOCS);
            
            // 分配NUM_ALLOCS个大小为SMALL_SIZE的对象
            for (size_t i = 0; i < NUM_ALLOCS; ++i) 
            {
                ptrs.push_back(MemoryPool::allocate(SMALL_SIZE));
                
                // 模拟真实使用：部分立即释放
                if (i % 4 == 0) 
                {
                    MemoryPool::deallocate(ptrs.back(), SMALL_SIZE);
                    ptrs.pop_back();
                }
            }
            
            // 释放所有对象
            for (void* ptr : ptrs) 
            {
                MemoryPool::deallocate(ptr, SMALL_SIZE);
            }
            
            // 输出内存池测试结果
            std::cout << "Memory Pool: " << std::fixed << std::setprecision(3) 
                      << t.elapsed() << " us" << std::endl;
        }
        
        // 测试new/delete
        {
            Timer t;
            std::vector<void*> ptrs;
            ptrs.reserve(NUM_ALLOCS);
            
            // 分配NUM_ALLOCS个大小为SMALL_SIZE的对象
            for (size_t i = 0; i < NUM_ALLOCS; ++i) 
            {
                ptrs.push_back(new char[SMALL_SIZE]);
                
                if (i % 4 == 0) 
                {
                    delete[] static_cast<char*>(ptrs.back());
                    ptrs.pop_back();
                }
            }
            
            // 释放所有对象
            for (void* ptr : ptrs) 
            {
                delete[] static_cast<char*>(ptr);
            }
            
            // 输出new/delete测试结果
            std::cout << "New/Delete: " << std::fixed << std::setprecision(3) 
                      << t.elapsed() << " us" << std::endl;
        }
    }
    
    // 3. 多线程测试
    // 测试多线程
    static void testMultiThreaded() 
    {
        // 定义线程数、每个线程分配的内存块数和最大内存块大小
        constexpr size_t NUM_THREADS = 4;
        constexpr size_t ALLOCS_PER_THREAD = 25000;
        constexpr size_t MAX_SIZE = 256;
        
        // 输出测试信息
        std::cout << "\n多线程测试\nTesting multi-threaded allocations (" << NUM_THREADS 
                  << " threads, " << ALLOCS_PER_THREAD << " allocations each):" 
                  << std::endl;
        
        // 定义线程函数
        auto threadFunc = [](bool useMemPool) 
        {
            // 定义随机数生成器
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(8, MAX_SIZE);
            // 定义存储指针和对应大小的向量
            std::vector<std::pair<void*, size_t>> ptrs;
            ptrs.reserve(ALLOCS_PER_THREAD);
            
            // 分配内存
            for (size_t i = 0; i < ALLOCS_PER_THREAD; ++i) 
            {
                // 生成随机大小
                size_t size = dis(gen);
                // 根据useMemPool参数选择分配方式
                void* ptr = useMemPool ? MemoryPool::allocate(size) 
                                     : new char[size];
                // 存储指针和大小
                ptrs.push_back({ptr, size});
                
                // 随机释放一些内存
                if (rand() % 100 < 75) 
                {  // 75%的概率释放
                    size_t index = rand() % ptrs.size();
                    if (useMemPool) 
                    {
                        // 释放内存
                        MemoryPool::deallocate(ptrs[index].first, ptrs[index].second);
                    } 
                    else 
                    {
                        // 释放内存
                        delete[] static_cast<char*>(ptrs[index].first);
                    }
                    // 将最后一个元素移到被释放的位置
                    ptrs[index] = ptrs.back();
                    ptrs.pop_back();
                }
            }
            
            // 清理剩余内存
            for (const auto& [ptr, size] : ptrs) 
            {
                if (useMemPool) 
                {
                    // 释放内存
                    MemoryPool::deallocate(ptr, size);
                } 
                else 
                {
                    // 释放内存
                    delete[] static_cast<char*>(ptr);
                }
            }
        };
        
        // 测试内存池
        {
            Timer t;
            std::vector<std::thread> threads;
            
            for (size_t i = 0; i < NUM_THREADS; ++i) 
            {
                // 创建线程并传入参数
                threads.emplace_back(threadFunc, true);
            }
            
            for (auto& thread : threads) 
            {
                // 等待线程结束
                thread.join();
            }
            
            // 输出测试结果
            std::cout << "Memory Pool: " << std::fixed << std::setprecision(3) 
                      << t.elapsed() << " us" << std::endl;
        }
        
        // 测试new/delete
        {
            Timer t;
            std::vector<std::thread> threads;
            
            for (size_t i = 0; i < NUM_THREADS; ++i) 
            {
                // 创建线程并传入参数
                threads.emplace_back(threadFunc, false);
            }
            
            for (auto& thread : threads) 
            {
                // 等待线程结束
                thread.join();
            }
            
            // 输出测试结果
            std::cout << "New/Delete: " << std::fixed << std::setprecision(3) 
                      << t.elapsed() << " us" << std::endl;
        }
    }
    
    // 4. 混合大小测试
    static void testMixedSizes() 
    {
        // 定义测试次数
        constexpr size_t NUM_ALLOCS = 50000;
        // 定义不同大小的数组
        const size_t SIZES[] = {16, 32, 64, 128, 256, 512, 1024, 2048};
        
        // 输出测试信息
        std::cout << "\n混合大小测试\nTesting mixed size allocations (" << NUM_ALLOCS 
                  << " allocations):" << std::endl;
        
        // 测试内存池
        {
            Timer t;
            std::vector<std::pair<void*, size_t>> ptrs;
            ptrs.reserve(NUM_ALLOCS);
            
            // 循环测试
            for (size_t i = 0; i < NUM_ALLOCS; ++i) 
            {
                // 随机选择大小
                size_t size = SIZES[rand() % 8];
                // 分配内存
                void* p = MemoryPool::allocate(size);
                // 存储指针和大小
                ptrs.emplace_back(p, size);
                
                // 批量释放
                if (i % 100 == 0 && !ptrs.empty()) 
                {
                    size_t releaseCount = std::min(ptrs.size(), size_t(20));
                    for (size_t j = 0; j < releaseCount; ++j) 
                    {
                        // 释放内存
                        MemoryPool::deallocate(ptrs.back().first, ptrs.back().second);
                        // 移除指针
                        ptrs.pop_back();
                    }
                }
            }
            
            // 释放剩余内存
            for (const auto& [ptr, size] : ptrs) 
            {
                MemoryPool::deallocate(ptr, size);
            }
            
            // 输出测试结果
            std::cout << "Memory Pool: " << std::fixed << std::setprecision(3) 
                      << t.elapsed() << " us" << std::endl;
        }
        
        // 测试new/delete
        {
            Timer t;
            std::vector<std::pair<void*, size_t>> ptrs;
            ptrs.reserve(NUM_ALLOCS);
            
            // 循环测试
            for (size_t i = 0; i < NUM_ALLOCS; ++i) 
            {
                // 随机选择大小
                size_t size = SIZES[rand() % 8];
                // 分配内存
                void* p = new char[size];
                // 存储指针和大小
                ptrs.emplace_back(p, size);
                
                // 批量释放
                if (i % 100 == 0 && !ptrs.empty()) 
                {
                    size_t releaseCount = std::min(ptrs.size(), size_t(20));
                    for (size_t j = 0; j < releaseCount; ++j) 
                    {
                        // 释放内存
                        delete[] static_cast<char*>(ptrs.back().first);
                        // 移除指针
                        ptrs.pop_back();
                    }
                }
            }
            
            // 释放剩余内存
            for (const auto& [ptr, size] : ptrs) 
            {
                delete[] static_cast<char*>(ptr);
            }
            
            // 输出测试结果
            std::cout << "New/Delete: " << std::fixed << std::setprecision(3) 
                      << t.elapsed() << " us" << std::endl;
        }
    }
};

int main() 
{
    std::cout << "Starting performance tests..." << std::endl;
    
    // 预热系统
    PerformanceTest::warmup();
    
    // 运行测试
    PerformanceTest::testSmallAllocation();
    PerformanceTest::testMultiThreaded();
    PerformanceTest::testMixedSizes();
    
    return 0;
}