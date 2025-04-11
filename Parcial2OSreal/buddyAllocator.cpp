#include "BuddyAllocator.h"

size_t BuddyAllocator::nextPowerOfTwo(size_t n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

BuddyAllocator::BuddyAllocator(size_t size) 
    : totalSize(nextPowerOfTwo(size)), memoryBlocks(totalSize, 0) {}

void* BuddyAllocator::allocate(size_t size) {
    if (size == 0) return nullptr;
    
    size = nextPowerOfTwo(size);
    if (size > totalSize) return nullptr;

    // Buscar el primer bloque libre del tamaño adecuado
    for (size_t i = 0; i <= totalSize - size; i += size) {
        if (memoryBlocks[i] == 0) {
            bool blockFree = true;
            // Verificar que todo el bloque esté libre
            for (size_t j = i; j < i + size; ++j) {
                if (memoryBlocks[j] != 0) {
                    blockFree = false;
                    break;
                }
            }
            
            if (blockFree) {
                // Marcar como ocupado
                for (size_t j = i; j < i + size; ++j) {
                    memoryBlocks[j] = 1;
                }
                allocatedBlocks[i] = size;
                return &memoryBlocks[i];
            }
        }
    }
    return nullptr;
}

void BuddyAllocator::deallocate(void* ptr) {
    if (!ptr) return;

    size_t index = static_cast<char*>(ptr) - &memoryBlocks[0];
    if (index >= totalSize || !allocatedBlocks.count(index)) return;

    size_t size = allocatedBlocks[index];
    // Marcar como libre
    for (size_t i = index; i < index + size; ++i) {
        memoryBlocks[i] = 0;
    }
    allocatedBlocks.erase(index);
    
    // Intentar fusionar con buddies
    mergeBuddies(index, size);
}

void BuddyAllocator::mergeBuddies(size_t index, size_t size) {
    size_t buddyIndex = index ^ size;
    
    while (size < totalSize) {
        // Verificar si el buddy está libre y es del mismo tamaño
        if (buddyIndex < totalSize && memoryBlocks[buddyIndex] == 0) {
            bool buddyCompletelyFree = true;
            for (size_t i = buddyIndex; i < buddyIndex + size; ++i) {
                if (memoryBlocks[i] != 0) {
                    buddyCompletelyFree = false;
                    break;
                }
            }
            
            if (buddyCompletelyFree) {
                // Fusionar bloques
                size *= 2;
                index = std::min(index, buddyIndex);
                buddyIndex = index ^ size;
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

// Funciones de monitoreo
size_t BuddyAllocator::getTotalMemory() const {
    return totalSize;
}

size_t BuddyAllocator::getUsedMemory() const {
    size_t used = 0;
    for (const auto& block : allocatedBlocks) {
        used += block.second;
    }
    return used;
}

size_t BuddyAllocator::getFreeMemory() const {
    return totalSize - getUsedMemory();
}

void BuddyAllocator::printMemoryStatus() const {
    std::cout << "Estado de memoria:\n";
    std::cout << "Total: " << getTotalMemory() << " bytes\n";
    std::cout << "En uso: " << getUsedMemory() << " bytes\n";
    std::cout << "Libres: " << getFreeMemory() << " bytes\n";
    
    std::cout << "Bloques asignados:\n";
    for (const auto& block : allocatedBlocks) {
        std::cout << " - Dirección: " << block.first 
                  << ", Tamaño: " << block.second << " bytes\n";
    }
}