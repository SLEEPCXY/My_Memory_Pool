#include "MemoryPool.h"

namespace Memory_Pool{

    void MemoryPool::init(size_t size){
        BlockSize_ = 4096;
        SlotSize_ = size;
        firstBlock_ = nullptr;
        curSlot_ = nullptr;
        freeList_ = nullptr;                       //v2修改了类型值后重新复制
        lastSlot_ = nullptr;
    }

    void *MemoryPool::allocate()                                   //拿一个新的槽位返回void*
    {

        if (freeList_.load(std::memory_order_acquire) != nullptr){                           //v2 判断条件获取值的方式修改一下
                //v2 修改为原子变量，原本这里的锁就不需要了   std::lock_guard<std::mutex> lock(mutex_for_freeList_);    
        //        if (freeList_!= nullptr){         v2，类型修改为原子变量后这里就不需要再次判断了
                    Slot *expected = freeList_.load(std::memory_order_acquire);          //v2    修改获取值的方式
                    do{
                        if(expected == nullptr)
                            break;
                    } while (!freeList_.compare_exchange_weak(expected, expected->next, std::memory_order_seq_cst, std::memory_order_seq_cst));
                    //freeList_.store(freeList_.load()->next); // v2变更获取值的方式以及修改值的方式但是编译后会报错所以这里不能这样写了，需要引用上面的写法，用内存序确保freeList_的原子性
                    return expected;
        //    }
        }
        if (curSlot_ >= lastSlot_){
            allocateNewBlock();
        }
        Slot *temp;
        {
            std::lock_guard<std::mutex> lock(mutex_for_Block_);
            temp = curSlot_;
            curSlot_ += SlotSize_ / sizeof(Slot *);                 //相当于让curSlot_移向下一个槽位
            return temp;
        }
    }

    void MemoryPool::allocateNewBlock()                            //开辟一个新的内存块
    {
        std::lock_guard<std::mutex> lock(mutex_for_Block_);

        void *newBlock = operator new(BlockSize_);
        Slot *head = reinterpret_cast<Slot *>(newBlock);
        head->next = firstBlock_;
        firstBlock_ = head;
        char *body = reinterpret_cast<char*>(newBlock) + sizeof(Slot*);
        size_t pading_num = padPointer(body, SlotSize_);
        curSlot_ = reinterpret_cast<Slot *>(body + pading_num);

        lastSlot_ = reinterpret_cast<Slot*>( reinterpret_cast<size_t>(newBlock) + BlockSize_ - SlotSize_ + 1);

        freeList_.store(nullptr);           //v2 更改 写值的方式
    }

    size_t MemoryPool::padPointer(char *p, size_t align)               //计算需要对齐的空间大小并返回
    {
        return (align - (reinterpret_cast<size_t>(p) % align)) % align;
    }

    void MemoryPool::deallocate(void *ptr)                             //释放该槽位（ptr）
    {
        if(ptr == nullptr)
            return;
        //std::lock_guard<std::mutex> lock(mutex_for_freeList_);            v2因为修改了类型所以就不用这个锁了
        if (ptr == nullptr)
            return;
        Slot *temp = reinterpret_cast<Slot *>(ptr);
        temp->next = freeList_.load(std::memory_order_acquire);                                  //v2 更改获取值的方式
        freeList_.store(temp,std::memory_order_release);                                          //v2更改写值的方式
    }

    void *HashBucket::useMemory(size_t size)                        //向内存池要一块size大小的内存
    {
        if (size > 512){
            void *temp = operator new(size);
            return temp;
        }
        else{
            return getMemoryPool((size + 7) / SLOT_BASE_SIZE - 1).allocate();
        }
    }

    void HashBucket::freeMemory(void *ptr, size_t size)             //释放ptr这块内存，大小是size
    {
        if (!ptr)
            return;
        if (size > 512)
        {
            operator delete(ptr);
            return;
        }
        else{
            getMemoryPool((size + 7) / SLOT_BASE_SIZE - 1).deallocate(ptr);
        }
    }
}

// int main(){
//     std::cout << 123 << std::endl;
// #if __cplusplus >= 201103L
//     std::cout << "C++11 已启用" << std::endl;
// #else
//     std::cout << "C++11 未启用" << std::endl;
// #endif
//     return 0;
// }