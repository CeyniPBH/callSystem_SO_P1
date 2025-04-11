#include "BuddyAllocator.h"

size_t BuddyAllocator::nextPowerOfTwo(size_t n) {
    size_t power = 1;
    while (power < n) power *= 2;
    return power;
}

BuddyAllocator::BuddyAllocator(size_t size) 
    : totalSize(nextPowerOfTwo(size)), memoryBlocks(totalSize, 0) {}

void* BuddyAllocator::allocate(size_t size) {
    size = nextPowerOfTwo(size);
    for (size_t i = 0; i + size <= totalSize; i += size) {
        if (memoryBlocks[i] == 0) {
            memoryBlocks[i] = 1;
            allocatedBlocks[i] = size;
            return reinterpret_cast<void*>(&memoryBlocks[i]);
        }
    }
    return nullptr;
}

void BuddyAllocator::deallocate(void* ptr) {
    size_t index = reinterpret_cast<size_t>(ptr) - reinterpret_cast<size_t>(&memoryBlocks[0]);
    if (allocatedBlocks.count(index)) {
        memoryBlocks[index] = 0;
        allocatedBlocks.erase(index);
    }
}
