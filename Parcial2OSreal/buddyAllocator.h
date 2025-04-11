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
    std::map<size_t, size_t> allocatedBlocks; // Bloques asignados: índice -> tamaño
    std::map<size_t, size_t> freeBlocks;      // Bloques libres: índice -> tamaño

    size_t nextPowerOfTwo(size_t n);
    void mergeBuddies(size_t index, size_t size);

public:
    BuddyAllocator(size_t size);
    void* allocate(size_t size);
    void deallocate(void* ptr);
    
    // Funciones de monitoreo
    size_t getTotalMemory() const;
    size_t getUsedMemory() const;
    size_t getFreeMemory() const;
    void printMemoryStatus() const;
};

#endif // BUDDYALLOCATOR_H