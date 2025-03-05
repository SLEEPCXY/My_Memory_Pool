# 基于哈希桶的高并发内存池 - My_Memory_Pool
## 编译方法
### （Linux/mac）
1. 下载整个项目源码
2. 下载cmake和make（如果没下载的话）
3. 在同机目录中创建一个build文档，然后进入build
4. 终端运行cmake ..
5. 然后运行make
6. 在build目录下会产生一个test可执行文件，即为最终结果
### (windows)
不需要下载cmakelists文件，打开编译器即可编译运行
## 文件说明
MemoryPool.cpp - 内存池源码文件
MemoryPool.h - 内存池头文件
test.cpp - 测试文件
CmakeLists.txt - 使用cmake编译的文件

## v2版本
使用atomic优化了freeList_链表 在代码中搜索注释中的"v2"关键字可以查看到修改了哪些地方 其中v2_attention是我卡了一下午的地方需要注意判断是为空，都则会造成越界

