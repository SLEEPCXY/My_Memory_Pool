#pragma once
#include <iostream>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <memory>
#include <cassert>


namespace Memory_Pool{
#define MEMORY_POOL_NUM 64      //总共有64个档位：1～8 / 9～16 / 17～24 。。。 505～512
#define SLOT_BASE_SIZE 8        //每个内存块中每个槽位的基本大小，也就是说每个槽位的大小是8的倍数
#define MAX_SLOT_SIZE 512          //内存池最大支持开辟512字节

    struct Slot{                    //既是存储数据，也是位置指针
        Slot *next;
    };

    //每个档位的内存池模版
    class MemoryPool{
        private:
            int BlockSize_;             //内存块的大小固定是4096
            int SlotSize_;              //记录当前内存块中，每个槽的大小
            Slot *firstBlock_;          //记录当前所在的内存块链表的首地址
            Slot *curSlot_;             //记录当前的槽位，用于分配槽位
            std::atomic<Slot*> freeList_;            //用于储存每次被用户释放的槽位，不还给操作系统 //v2类型修改为原子变量，少了一个锁
            Slot *lastSlot_;            //记录内存块的结尾界限，如果curSlot_超过这个，就需要开辟一个新的内存块了

            //v2使用原子变量之后就不需要这一行了 std::mutex mutex_for_freeList_; //用在空闲槽位链表的锁，保证多线程下的原子性
            std::mutex mutex_for_Block_;        //用在内存块的锁，防止多线程下的重复开辟
        public:
            //construction
            MemoryPool(size_t BlockSize = 4096):BlockSize_(BlockSize){}
            ~MemoryPool(){
                Slot *temp = firstBlock_;
                while (temp != nullptr){                        //释放所在档位的所有内存块
                    Slot *next = temp->next;
                    delete temp;
                    temp = next;
                }
            }

            void init(size_t size);                                 //初始化成员变量
            void *allocate();                                   //拿一个新的槽位返回void*
            void deallocate(void *ptr);                             //释放该槽位（ptr）
            void allocateNewBlock();                            //开辟一个新的内存块
            size_t padPointer(char *p, size_t align);               //计算需要对齐的空间大小并返回
    };


    //哈希数组，储存不同档位的内存池
    class HashBucket{
        private:
            

        public:
            static void initMemoryPool(){                                   //初始化每个档位对象的内存内存池
                for (int i = 0; i < MEMORY_POOL_NUM;i++){
                    getMemoryPool(i).init((i + 1) * SLOT_BASE_SIZE);
                }
            }

            static MemoryPool& getMemoryPool(int index){                //取回第index个档位的内存池
                static MemoryPool memoryPool[MEMORY_POOL_NUM];          //在函数内定义否则就需要在类外定义
                return memoryPool[index];
            }

            static void *useMemory(size_t size);                        //向内存池要一块size大小的内存

            static void freeMemory(void *ptr, size_t size);             //释放ptr这块内存，大小是size

            template <typename T,typename... Args>friend T *newElement(Args&&... args);
            template <typename T>friend void deleteElement(T *p);
    };


    //用户接口
    template <typename T,typename... Args>
    T *newElement(Args&&... args){

        void *ptr = HashBucket::useMemory(sizeof(T));
        T *p = reinterpret_cast<T *>(ptr);
        if (p != nullptr){
            new(p) T(std::forward<Args>(args)...);
        }
        return p;
    }

    template <typename T>
    void deleteElement(T *p){
        if (!p)
            return;
        p->~T();
        HashBucket::freeMemory(p, sizeof(T));
    }
}