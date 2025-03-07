#include "CentralCache.h"

namespace MyMemoryPool{
    void* CentralCache::fetchRange(size_t index)                     //获取index档位的内存块
    {   
        if (index > FREE_LIST_SIZE)
            return nullptr;
        while (locks_[index].test_and_set(std::memory_order_acquire))       //自旋锁，一直等待
        {
            std::this_thread::yield();              //主动让出CPU
        }

        void* span = centralFreeList_[index].load(std::memory_order_acquire);
        
        if (span){
            void *next = *reinterpret_cast<void **>(span);
            centralFreeList_[index].store(next, std::memory_order_acq_rel);
        }
        else{
            size_t size = (index + 1) * ALIGNMENT;              //计算当前档位内存块的大小
            span = fetchFromPageCache(size);
            if (!span){
                //locks_[index].clear();
                return nullptr;
            }
            size_t num = (SPAN_SIZE * PageCache::PAGE_SIZE) / size;     //计算获得的大块内存可以分为多少块
            if (num > 1){
                for (int i = 1; i < num;i++){                       //除开要返回的那个，其它的都串成链表
                    void *current = span + i * size;
                    void *next = span + (i + 1) * size;
                    *reinterpret_cast<void **>(current) = next;
                }
                *reinterpret_cast<void **>(span + (num-1) * size) = nullptr;        //最后一个的next置为空，这里注意指针是左操作数啊
                void *next = span + size;
                centralFreeList_[index].store(next, std::memory_order_acq_rel);
            }
        }
        locks_[index].clear();
        return span;
    }
    void *fetchFromPageCache(size_t size)                  //从页缓存获取内存，应该是存储到空闲链表里去吧
    {   
        if (size <= SPAN_SIZE * PageCache::PAGE_SIZE){                  //最小分配页数，防止频繁向os申请内存
            return PageCache::getInstance().allocateSpan(SPAN_SIZE);
        }
        else{
            size_t numPages = (size + PageCache::PAGE_SIZE - 1) / PageCache::PAGE_SIZE;                //计算所需页数
            return PageCache::getInstance().allocateSpan(numPages);
        }
    }
    void CentralCache::returnRange(void *start, size_t size, size_t index)           //归还size大小的内存，起始地址为start
    {
        if (index > FREE_LIST_SIZE){
            return;
        }
        while (locks_[index].test_and_set(std::memory_order_acquire)){
            std::this_thread::yield();
        }
        *reinterpret_cast<void **>(start) = centralFreeList_[index].load(std::memory_order_acquire);
        centralFreeList_[index].store(start, std::memory_order_acq_rel);
        locks_[index].clear();
    }
}