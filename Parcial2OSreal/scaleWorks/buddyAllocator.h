#ifndef BUDDYALLOCATOR_H
#define BUDDYALLOCATOR_H

#include <vector>
#include <map>
#include <cstddef>
#include <iostream>

class BuddyAllocator {
private:
    size_t totalSize;
    std::vector<char> memoryBlocks;
    std::map<size_t, size_t> allocatedBlocks;

    size_t nextPowerOfTwo(size_t n);

public:
    BuddyAllocator(size_t size);
    void* allocate(size_t size);
    void deallocate(void* ptr);
};

#endif // BUDDYALLOCATOR_H
