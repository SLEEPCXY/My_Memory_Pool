#pragma once
#include <iostream>


namespace MyMemoryPool{
    const size_t ALIGNMENT = 8;                       //每个槽大小为8字节
    const size_t MAX_BYTES = 256 * 1024;              //能使用内存池开辟的最大空间
    const size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT;
}