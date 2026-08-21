// Copyright (c) Red Alert 4 project. High-performance zero-allocation object pool.
#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <vector>
#include <utility>
#include <new>

namespace RA4
{

// Cache-friendly fixed-capacity object pool.
// Provides O(1) allocation and deallocation with zero heap allocations at runtime.
// Suitable for high-frequency objects such as projectiles, alerts, combat events,
// navigation requests, and visual proxies.
template <typename T, size_t Capacity>
class TObjectPool
{
public:
    TObjectPool()
    {
        Clear();
    }

    // Allocate an element and return a pointer to it. Returns nullptr if pool is full.
    template <typename... Args>
    T* Acquire(Args&&... InArgs)
    {
        if (FreeCount == 0)
        {
            return nullptr;
        }

        const uint32_t Index = FreeList[--FreeCount];
        bOccupied[Index] = true;
        T* Slot = reinterpret_cast<T*>(&Storage[Index * sizeof(T)]);
        new (Slot) T(std::forward<Args>(InArgs)...);
        return Slot;
    }

    // Allocate an element and return its index. Returns -1 (0xFFFFFFFF) if full.
    template <typename... Args>
    uint32_t AcquireIndex(Args&&... InArgs)
    {
        if (FreeCount == 0)
        {
            return uint32_t(-1);
        }

        const uint32_t Index = FreeList[--FreeCount];
        bOccupied[Index] = true;
        T* Slot = reinterpret_cast<T*>(&Storage[Index * sizeof(T)]);
        new (Slot) T(std::forward<Args>(InArgs)...);
        return Index;
    }

    // Release an element back to the pool.
    void Release(T* Element)
    {
        if (!Element)
        {
            return;
        }

        const uint8_t* ElemPtr = reinterpret_cast<const uint8_t*>(Element);
        const ptrdiff_t Offset = ElemPtr - &Storage[0];
        if (Offset < 0)
        {
            return;
        }
        const size_t UOffset = static_cast<size_t>(Offset);
        if (UOffset >= Capacity * sizeof(T) || (UOffset % sizeof(T)) != 0)
        {
            return; // Not aligned or not in this pool
        }

        const uint32_t Index = static_cast<uint32_t>(UOffset / sizeof(T));
        ReleaseByIndex(Index);
    }

    // Release an element by its index.
    void ReleaseByIndex(uint32_t Index)
    {
        if (Index >= Capacity || !bOccupied[Index])
        {
            return;
        }

        T* Slot = reinterpret_cast<T*>(&Storage[Index * sizeof(T)]);
        Slot->~T();
        bOccupied[Index] = false;
        FreeList[FreeCount++] = Index;
    }

    // Direct index access.
    T* Get(uint32_t Index)
    {
        return (Index < Capacity && bOccupied[Index]) ? reinterpret_cast<T*>(&Storage[Index * sizeof(T)]) : nullptr;
    }

    const T* Get(uint32_t Index) const
    {
        return (Index < Capacity && bOccupied[Index]) ? reinterpret_cast<const T*>(&Storage[Index * sizeof(T)]) : nullptr;
    }

    bool IsAllocated(uint32_t Index) const
    {
        return Index < Capacity && bOccupied[Index];
    }

    int32_t GetIndex(const T* Element) const
    {
        if (!Element) return -1;
        const uint8_t* ElemPtr = reinterpret_cast<const uint8_t*>(Element);
        const ptrdiff_t Offset = ElemPtr - &Storage[0];
        if (Offset < 0) return -1;
        const size_t UOffset = static_cast<size_t>(Offset);
        if (UOffset >= Capacity * sizeof(T) || (UOffset % sizeof(T)) != 0) return -1;
        return static_cast<int32_t>(UOffset / sizeof(T));
    }


    void Clear()
    {
        for (size_t I = 0; I < Capacity; ++I)
        {
            if (bOccupied[I])
            {
                T* Slot = reinterpret_cast<T*>(&Storage[I * sizeof(T)]);
                Slot->~T();
                bOccupied[I] = false;
            }
            FreeList[I] = static_cast<uint32_t>(Capacity - 1 - I);
        }
        FreeCount = Capacity;
    }

    size_t GetCapacity() const { return Capacity; }
    size_t GetActiveCount() const { return Capacity - FreeCount; }
    size_t GetFreeCount() const { return FreeCount; }
    bool IsFull() const { return FreeCount == 0; }
    bool IsEmpty() const { return FreeCount == Capacity; }

private:
    alignas(alignof(T)) uint8_t Storage[Capacity * sizeof(T)];
    std::array<uint32_t, Capacity> FreeList{};
    std::array<bool, Capacity> bOccupied{};
    size_t FreeCount = Capacity;
};

} // namespace RA4
