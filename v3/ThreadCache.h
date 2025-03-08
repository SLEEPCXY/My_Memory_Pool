#pragma once
#include <iostream>
#include <array>
#include "common.h"
#include "CentralCache.h"

namespace MyMemoryPool{

    class ThreadCache{
        private:
            std::array<void *, FREE_LIST_SIZE> freeList_;
            ThreadCache(){
                for (int i = 0; i < FREE_LIST_SIZE;i++){
                    freeList_[i] = nullptr;
                }
            }
        public:
            static ThreadCache& getInstance(){
                static thread_local ThreadCache instance;
                return instance;
            }

            void *allocate(size_t size);                    //申请大小为size的空间

            void deallocate(void *ptr, size_t size);            //归还大小为size的空间，首地址是ptr

            void *fetchFromcentralCache(size_t index);              //从centralCache获取一块内存，档位是index

            //bool shouldReturnToCentralCache(size_t index);              //判断档位index的内存链表是否需要归还内存给中心缓存

            void returnToCentralCache(void *start, size_t size, size_t bytes);      //归还内存给中心缓存，首地址是start，大小是size，每一块的大小是bytes
    };
}