#include "PageCache.h"

namespace MyMemoryPool{
    void *PageCache::allocateSpan(size_t numPages)                //分配numPages页的内存块，返回首地址
    {
        std::lock_guard<std::mutex> lock(PageCache_mutex);

        std::map<size_t, Span *>::iterator it = freeSpans_.lower_bound(numPages);

        //*********************注意这里的if块代码写的不一样******************** */
        if (it != freeSpans_.end()){
            Span *head = it->second;
            it->second = head->next;                //将头节点去除，下一个节点作为新头节点存入freeSpans_


            if (head->numPages > numPages){         //如果内存块页数大于需要的页数，需要将它切小一点，多余的放回去，杜绝浪费
                Span *next = new Span;
                next->SpanHeadAddr = reinterpret_cast<char *>(head) + numPages * PAGE_SIZE;
                next->numPages = head->numPages - numPages;
                head->numPages = numPages;
                
                //将多余的next部分放入freeSpans_中
                next->next = freeSpans_[next->numPages];
                freeSpans_[next->numPages] = next;
            }

            if (spanMap_.find(head->SpanHeadAddr) != spanMap_.end())
                spanMap_[head->SpanHeadAddr] = head;
            return head->SpanHeadAddr;
        }
        else{
            void *memory = systemAlloc(numPages);
            if (!memory)
                return nullptr;         //开辟失败

            Span *head = new Span;
            head->SpanHeadAddr = memory;
            head->next = nullptr;
            head->numPages = numPages;

            return memory;
        }
    }

    void *PageCache::systemAlloc(size_t numPages)                 //向操作系统申请numPages页的内存块
    {
        size_t size = numPages * PAGE_SIZE;
        void *memory = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (memory == MAP_FAILED)
            return nullptr;
        return memory;
    }
    void PageCache::deallocateSpan(void *ptr, size_t numPages)                //归还numPages的内存，首地址是ptr
    {
        std::lock_guard<std::mutex> lock(PageCache_mutex);

        if (spanMap_.find(ptr) == spanMap_.end()){
            return;
        }
        Span *head = spanMap_.find(ptr)->second;

        Span *nextSpan = reinterpret_cast<Span *>(reinterpret_cast<char *>(ptr) + numPages * PAGE_SIZE);
        if (spanMap_.find(nextSpan->SpanHeadAddr) != spanMap_.end()){

            //检查是否是空闲内存
            std::map<size_t, Span *>::iterator it = freeSpans_.find(nextSpan->numPages);
            bool flag = false;
            if (it != freeSpans_.end())
            {
                //判断是否存在于空闲链表中，如果存在则将其删除
                if (it->second == nextSpan){       //存在且作为头节点存储
                    flag = true;
                    freeSpans_[nextSpan->numPages] = nextSpan->next;
                }
                else{
                    Span *temp = it->second;
                    while (temp){
                        if (temp->next == nextSpan){
                            flag = true;
                            temp->next = nextSpan->next;
                            break;
                        }
                        temp = temp->next;
                    }
                }
            }

            //如果是空闲的则与后面一块内存合并
            if (flag){
                head->numPages = numPages + nextSpan->numPages;
                spanMap_.erase(nextSpan->SpanHeadAddr);
                delete nextSpan;
            }
        }
        head->next = freeSpans_[head->numPages];
        freeSpans_[head->numPages] = head;
    }
}