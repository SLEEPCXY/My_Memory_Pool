#include "ThreadCache.h"

namespace MyMemoryPool{
    //从线程缓存中分配内存
    void *ThreadCache::allocate(size_t size){
        //如果size为0，则分配对齐大小
        if (size == 0){
            size = ALIGNMENT;
        }
        //如果size大于最大字节数，则使用operator new分配内存
        if (size > MAX_BYTES){
            return operator new(size);
        }
        //计算档位
        size_t index = (size + ALIGNMENT - 1) / ALIGNMENT;          //计算档位

        //如果freeList_中对应档位的内存不为空，则从freeList_中分配内存
        if (freeList_[index] != nullptr){
            void *ret = freeList_[index];
            freeList_[index] = *reinterpret_cast<void **>(ret);
            return ret;
        }
        //否则，从centralCache中获取内存
        else{
            //使用fetchFromcentralCache获取内存
            return fetchFromcentralCache(index);
        }
    }

    // 从中央缓存中获取数据
    void *ThreadCache::fetchFromcentralCache(size_t index){
        // 从中央缓存中获取数据
        void *ret = CentralCache::getInstance().fetchRange(index);
        // 如果获取失败，返回空指针
        if (!ret)
            return nullptr;

        // 将获取到的数据存入freeList_中
        freeList_[index] = *reinterpret_cast<void **>(ret);
        // 返回获取到的数据
        return ret;
    }

    // 释放内存
    void ThreadCache::deallocate(void *ptr, size_t size){
        // 如果大小大于最大字节数，则使用全局的delete操作符
        if (size > MAX_BYTES){
            operator delete(ptr);
        }
        // 计算索引
        size_t index = (size + ALIGNMENT - 1) / ALIGNMENT;

        // 将指针指向freeList_数组中的下一个元素
        *reinterpret_cast<void **>(ptr) = freeList_[index];
        // 将指针赋值给freeList_数组中的对应位置
        freeList_[index] = ptr;
    }

    // 将线程缓存中的内存块归还到中央缓存中
    void ThreadCache::returnToCentralCache(void *start, size_t size, size_t bytes){
        // 计算内存块的大小
        size_t index = (size + ALIGNMENT - 1) / ALIGNMENT;
        // 计算内存块的数量
        size_t blocknum = size / bytes;
        // 计算需要保留的内存块数量
        size_t staynum = blocknum / 4;
        // 计算需要归还的内存块数量
        size_t returnnum = blocknum - staynum;
        // 计算最后一个需要保留的内存块的指针
        void *last_stay_ptr = reinterpret_cast<void *>(reinterpret_cast<char *>(start) + (staynum - 1) * bytes);
        // 计算需要归还的内存块的指针
        void *ReturnPtr = reinterpret_cast<void*>(reinterpret_cast<char *>(last_stay_ptr) + bytes);
        // 将最后一个需要保留的内存块的指针指向freeList_中的第一个元素
        *reinterpret_cast<void **>(last_stay_ptr) = freeList_[index];
        // 将start指针指向freeList_中的第一个元素
        freeList_[index] = start;
        // 将需要归还的内存块归还到中央缓存中
        CentralCache::getInstance().returnRange(ReturnPtr, returnnum * bytes, index);
    }
}