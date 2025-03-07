#pragma once
#include <iostream>
#include <map>
#include <mutex>
#include <sys/mman.h>

namespace MyMemoryPool{
    class PageCache{
        private:
            struct Span{
                void *SpanHeadAddr;             //当前内存块的首地址
                int numPages;                   //当前内存块有多少页
                Span *next;                     //下一块内存块的地址
            };
            std::map<size_t, Span *> freeSpans_;                //用来存储被归还的页内存块，1页的存一个链表，2页的存一个链表，以此类推
            std::map<void *, Span *> spanMap_;          //用来存储被归还的页内存块，按照首地址存储
            std::mutex PageCache_mutex;

        public:            
            static const size_t PAGE_SIZE = 4096;       //一页的大小设置为4KB
            static PageCache& getInstance(){
                static PageCache instance;
                return instance;
            }
            void *allocateSpan(size_t numPages);                //分配numPages页的内存块，返回首地址

            void *systemAlloc(size_t numPages);                 //向操作系统申请numPages页的内存块

            void deallocateSpan(void *ptr, size_t numPages);                //归还numPages的内存，首地址是ptr
    };
}