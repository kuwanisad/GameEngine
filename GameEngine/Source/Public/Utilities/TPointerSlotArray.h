
#pragma once

#include "../Core/CoreMinimal.h"

#include <type_traits>
#include <utility>
#include <cassert>

template<typename T>
    requires std::is_pointer_v<T>
class TPointerSlotArray
{
private:

    TArray<T> Arr;
    TArray<uint32> FreeIndices;

public:

    void Empty()
    {
        Arr.clear();
        FreeIndices.clear();
    }

    void Reserve(uint32 NewCapacity)
    {
        if (NewCapacity <= Arr.size())
            return;

        Arr.reserve(NewCapacity);
        FreeIndices.reserve(NewCapacity);
    }

    bool Resize(uint32 NewSize)
    {
        const uint32 OldSize = static_cast<uint32>(Arr.size());

        if (NewSize == OldSize)
            return true;

        if (NewSize > OldSize)
        {
            Arr.resize(NewSize, nullptr);

            for (uint32 i = OldSize; i < NewSize; ++i)
            {
                FreeIndices.emplace_back(i);
            }

            return true;
        }

        uint32 OccupiedCount = 0;

        for (T Ptr : Arr)
        {
            if (Ptr != nullptr)
                ++OccupiedCount;
        }

        if (OccupiedCount > NewSize)
            return false;

        uint32 WriteIndex = 0;

        for (uint32 ReadIndex = 0; ReadIndex < OldSize; ++ReadIndex)
        {
            if (Arr[ReadIndex] == nullptr)
                continue;

            if (WriteIndex != ReadIndex)
            {
                Arr[WriteIndex] = Arr[ReadIndex];
            }

            ++WriteIndex;
        }

        Arr.resize(NewSize, nullptr);

        for (uint32 i = OccupiedCount; i < NewSize; ++i)
        {
            Arr[i] = nullptr;
        }

        // Free List 재구축
        FreeIndices.clear();

        for (uint32 i = OccupiedCount; i < NewSize; ++i)
        {
            FreeIndices.emplace_back(i);
        }

        return true;
    }

    bool IsValidIndex(uint32 Index) const
    {
        return Index < Arr.size() && Arr[Index] != nullptr;
    }

    void RemoveAll()
    {
        FreeIndices.clear();

        for (uint32 i = 0; i < Arr.size(); ++i)
        {
            Arr[i] = nullptr;
            FreeIndices.emplace_back(i);
        }
    }

    bool Remove(T Value)
    {
        if (Value == nullptr)
            return false;

        for (uint32 i = 0; i < Arr.size(); ++i)
        {
            if (Arr[i] == Value)
            {
                Arr[i] = nullptr;
                FreeIndices.emplace_back(i);

                return true;
            }
        }

        return false;
    }

    bool RemoveAt(uint32 Index)
    {
        if (!IsValidIndex(Index))
            return false;

        Arr[Index] = nullptr;
        FreeIndices.emplace_back(Index);

        return true;
    }

    uint32 Add(T Value)
    {
        assert(Value != nullptr);

        if (!FreeIndices.empty())
        {
            const uint32 Index = FreeIndices.back();
            FreeIndices.pop_back();

            Arr[Index] = Value;

            return Index;
        }

        Arr.emplace_back(Value);

        return static_cast<uint32>(Arr.size() - 1);
    }

    T operator[](uint32 Index) const
    {
        assert(IsValidIndex(Index));
        return Arr[Index];
    }

    T* GetArray()
    {
        return Arr.data();
    }

    const TArray<T>& GetArray() const
    {
        return Arr;
    }

    uint32 Size() const
    {
        return static_cast<uint32>(Arr.size());
    }

    uint32 ValidSize() const
    {
        return Size() - NumFree();
    }

    uint32 NumFree() const
    {
        return static_cast<uint32>(FreeIndices.size());
    }
};
