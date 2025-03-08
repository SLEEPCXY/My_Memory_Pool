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

        // 从centralFreeList_中获取索引为index的元素，并使用std::memory_order_acquire内存顺序加载
        void* span = centralFreeList_[index].load(std::memory_order_acquire);
        
        if (span){
            void *next = *reinterpret_cast<void **>(span);
            centralFreeList_[index].store(next, std::memory_order_release);
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
                    void *current = reinterpret_cast<char*>(span) + i * size;
                    void *next = reinterpret_cast<char*>(span) + (i + 1) * size;
                    *reinterpret_cast<void **>(current) = next;
                }
                *reinterpret_cast<void **>(reinterpret_cast<char*>(span) + (num-1) * size) = nullptr;        //最后一个的next置为空，这里注意指针是左操作数啊
                void *next = reinterpret_cast<char*>(span) + size;
                centralFreeList_[index].store(next, std::memory_order_release);
            }
        }
        locks_[index].clear();
        return span;
    }
    void *CentralCache::fetchFromPageCache(size_t size)                  //从页缓存获取内存，应该是存储到空闲链表里去吧
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
        //如果index大于FREE_LIST_SIZE，则直接返回
        if (index > FREE_LIST_SIZE){
            return;
        }
        //获取锁
        while (locks_[index].test_and_set(std::memory_order_acquire)){
            //如果获取锁失败，则让出CPU时间片
            std::this_thread::yield();
        }
        //将start地址存储到centralFreeList_中
        *reinterpret_cast<void **>(start) = centralFreeList_[index].load(std::memory_order_acquire);
        //将start地址存储到centralFreeList_中
        centralFreeList_[index].store(start, std::memory_order_release);
        //释放锁
        locks_[index].clear();
    }
}