#include "ThreadCache.h"

namespace MyMemoryPool{
    void *ThreadCache::allocate(size_t size){
        if (size == 0){
            size = ALIGNMENT;
        }
        if (size > MAX_BYTES){
            return operator new(size);
        }
        size_t index = (size + ALIGNMENT - 1) / ALIGNMENT;          //计算档位

        if (freeList_[index] != nullptr){
            void *ret = freeList_[index];
            freeList_[index] = *reinterpret_cast<void **>(ret);
            return ret;
        }
        else{
            //使用fetchFromcentralCache获取内存
            return fetchFromcentralCache(index);
        }
    }

    void *ThreadCache::fetchFromcentralCache(size_t index){
        void *ret = CentralCache::getInstance().fetchRange(index);
        if (!ret)
            return nullptr;

        freeList_[index] = *reinterpret_cast<void **>(ret);
        return ret;
    }

    void ThreadCache::deallocate(void *ptr, size_t size){
        if (size > MAX_BYTES){
            operator delete(ptr);
        }
        size_t index = (size + ALIGNMENT - 1) / ALIGNMENT;

        *reinterpret_cast<void **>(ptr) = freeList_[index];
        freeList_[index] = ptr;
    }

    void ThreadCache::returnToCentralCache(void *start, size_t size, size_t bytes){
        size_t index = (size + ALIGNMENT - 1) / ALIGNMENT;
        size_t blocknum = size / bytes;
        size_t staynum = blocknum / 4;
        size_t returnnum = blocknum - staynum;
        void *last_stay_ptr = reinterpret_cast<void *>(reinterpret_cast<char *>(start) + (staynum - 1) * bytes);
        void *ReturnPtr = reinterpret_cast<void*>(reinterpret_cast<char *>(last_stay_ptr) + bytes);
        *reinterpret_cast<void **>(last_stay_ptr) = freeList_[index];
        freeList_[index] = start;
        CentralCache::getInstance().returnRange(ReturnPtr, returnnum * bytes, index);
    }
}