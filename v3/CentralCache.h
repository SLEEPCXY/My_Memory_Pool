#pragma once
#include <iostream>
#include <array>
#include <atomic>
#include <thread>
#include "common.h"
#include "PageCache.h"



namespace MyMemoryPool{
    const size_t SPAN_SIZE = 8;
    class CentralCache
    {
    private:
        std::array<std::atomic<void *>, FREE_LIST_SIZE> centralFreeList_;       //用于存储各个档位的空闲内存，每8字节为1个档位
        std::array<std::atomic_flag, FREE_LIST_SIZE> locks_;                          //存储各个档位空闲链表的锁
    public:
        CentralCache(){
            for (int i = 0; i < centralFreeList_.size();i++){
                centralFreeList_[i] = nullptr;
                locks_[i].clear();
            }
        }
        static CentralCache &getInstance()                 //维护一个全局的CentralCache
        {
            static CentralCache instance;
            return instance;
        }

        void *fetchRange(size_t index);                     //获取index档位的内存块

        void *fetchFromPageCache(size_t size);                  //从页缓存获取内存，应该是存储到空闲链表里去吧

        void returnRange(void *start, size_t size, size_t index);           //归还size大小的内存，起始地址为start
    };
}