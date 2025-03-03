#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

#include "MemoryPool.h"

using namespace Memory_Pool;

// 测试用例
class P1 
{
    int id_;
};

class P2 
{
    int id_[5];
};

class P3
{
    int id_[10];
};

class P4
{
    int id_[20];
};

// 单轮次申请释放次数 线程数 轮次
long long BenchmarkMemoryPool(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks); // 线程池
	//size_t total_costtime = 0;
	auto start = std::chrono::steady_clock::now();						//开始时间
	for (size_t k = 0; k < nworks; ++k) // 创建 nworks 个线程
	{
		vthread[k] = std::thread([&]() {
			for (size_t j = 0; j < rounds; ++j)
			{
				//size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
                    P1* p1 = newElement<P1>(); // 内存池对外接口
                    deleteElement<P1>(p1);
                    P2* p2 = newElement<P2>();
                    deleteElement<P2>(p2);
                    P3* p3 = newElement<P3>();
                    deleteElement<P3>(p3);
                    P4* p4 = newElement<P4>();
                    deleteElement<P4>(p4);
				}
				//size_t end1 = clock();

				//total_costtime += end1 - begin1;
			}
		});
	}
	auto end = std::chrono::steady_clock::now();				//结束时间
	for (auto &t : vthread)
	{
		t.join();
	}
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	std::cout << nworks << "个线程并发执行" << rounds << "轮次，每轮次newElement&deleteElement " << ntimes << "次，总计花费：" << duration.count() << " us" << std::endl;
	//printf("%lu个线程并发执行%lu轮次，每轮次newElement&deleteElement %lu次，总计花费：%lu ms\n", nworks, rounds, ntimes, total_costtime);
	return duration.count();
}

long long BenchmarkNew(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	//size_t total_costtime = 0;
	auto start = std::chrono::steady_clock::now();						//开始时间
	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&]() {
			for (size_t j = 0; j < rounds; ++j)
			{
				//size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
                    P1* p1 = new P1;
                    delete p1;
                    P2* p2 = new P2;
                    delete p2;
                    P3* p3 = new P3;
                    delete p3;
                    P4* p4 = new P4;
                    delete p4;
				}
				//size_t end1 = clock();
				
				//total_costtime += end1 - begin1;
			}
		});
	}
	auto end = std::chrono::steady_clock::now();				//结束时间
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	for (auto& t : vthread)
	{
		t.join();
	}
	std::cout << nworks << "个线程并发执行" << rounds << "轮次，每轮次malloc&free " << ntimes << "次，总计花费：" << duration.count() << " us哈哈哈哈" << std::endl;
	//printf("%lu个线程并发执行%lu轮次，每轮次malloc&free %lu次，总计花费：%lu ms\n", nworks, rounds, ntimes, total_costtime);
	return duration.count();
}

void test01(size_t ntimes, size_t nworks, size_t rounds){
	long long sum_new = 0, sum_memory = 0;
	for (int i = 0; i < 20; i++)
	{
		if (i < 5){
			std::cout << "***********************热身开始！！！*****************" << std::endl;
		}
		else{
			std::cout << "***********************正式测试！！！*****************" << std::endl;
		}
		std::cout << "starting! go go go!!!" << std::endl;
		sum_new += BenchmarkNew(ntimes, nworks, rounds); // 测试 new delete
		
		std::cout << "===========================================================================" << std::endl;
		std::cout << "===========================================================================" << std::endl;
		sum_memory += BenchmarkMemoryPool(ntimes, nworks, rounds); // 测试内存池
		std::cout << std::endl
				<< std::endl;
	}
	std::cout << std::endl
			  << "New: " << sum_new / 20 << " us" << std::endl
			  << "Memory: " << sum_memory / 20 << " us" << std::endl;
}

// int main()
// {
//     HashBucket::initMemoryPool(); // 使用内存池接口前一定要先调用该函数
// 	test01(10000, 4, 10);
	
// 	return 0;
// }