
#pragma once

#include "../Core/CoreMinimal.h"

#include <type_traits>
#include <utility>
#include <cassert>

template<typename T>
    requires (!std::is_pointer_v<T>)
class TSlotArray
{
private:

    TArray<T> Arr;
    TArray<bool> Occupied;
    TArray<uint32> FreeIndices;

public:
    void Empty()
    {
        Arr.clear();
        Occupied.clear();
        FreeIndices.clear();
    }

    void Reserve(uint32 NewCapacity)
    {
        if (NewCapacity <= Arr.size())
            return;

        Arr.reserve(NewCapacity);
        Occupied.reserve(NewCapacity);
        FreeIndices.reserve(NewCapacity);
    }

    bool Resize(uint32 NewSize)
    {
        const uint32 OldSize = static_cast<uint32>(Arr.size());

        if (NewSize == OldSize)
            return true;

        // 확장
        if (NewSize > OldSize)
        {
            Arr.resize(NewSize);
            Occupied.resize(NewSize, false);

            for (uint32 i = OldSize; i < NewSize; ++i)
            {
                FreeIndices.emplace_back(i);
            }

            return true;
        }

        // 축소
        uint32 OccupiedCount = 0;

        for (bool bOccupied : Occupied)
        {
            if (bOccupied)
                ++OccupiedCount;
        }

        // 유효한 원소가 새 크기보다 많으면 실패
        if (OccupiedCount > NewSize)
            return false;

        // 유효한 원소 압축
        uint32 WriteIndex = 0;

        for (uint32 ReadIndex = 0; ReadIndex < OldSize; ++ReadIndex)
        {
            if (!Occupied[ReadIndex])
                continue;

            if (WriteIndex != ReadIndex)
            {
                Arr[WriteIndex] = std::move(Arr[ReadIndex]);
            }

            ++WriteIndex;
        }

        // 배열 크기 축소
        Arr.resize(NewSize);
        Occupied.resize(NewSize, false);

        // 압축된 영역을 사용 중으로 설정
        for (uint32 i = 0; i < OccupiedCount; ++i)
        {
            Occupied[i] = true;
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
        return Index < Occupied.size() && Occupied[Index];
    }

    void RemoveAll()
    {
        FreeIndices.clear();

        for (uint32 i = 0; i < Arr.size(); ++i)
        {
            if (!Occupied[i])
                continue;

            Arr[i] = T{};
            Occupied[i] = false;
        }

        FreeIndices.reserve(Arr.size());

        for (uint32 i = 0; i < Arr.size(); ++i)
        {
            FreeIndices.emplace_back(i);
        }
    }

    bool RemoveAt(uint32 Index)
    {
        if (!IsValidIndex(Index))
            return false;

        Arr[Index] = T{};
        Occupied[Index] = false;

        FreeIndices.emplace_back(Index);

        return true;
    }

    uint32 Add(const T& Value)
    {
        if (!FreeIndices.empty())
        {
            const uint32 Index = FreeIndices.back();
            FreeIndices.pop_back();

            Arr[Index] = Value;
            Occupied[Index] = true;

            return Index;
        }

        Arr.emplace_back(Value);
        Occupied.emplace_back(true);

        return static_cast<uint32>(Arr.size() - 1);
    }

    uint32 Add(T&& Value)
    {
        if (!FreeIndices.empty())
        {
            const uint32 Index = FreeIndices.back();
            FreeIndices.pop_back();

            Arr[Index] = std::move(Value);
            Occupied[Index] = true;

            return Index;
        }

        Arr.emplace_back(std::move(Value));
        Occupied.emplace_back(true);

        return static_cast<uint32>(Arr.size() - 1);
    }


    T& operator[](uint32 Index)
    {
        assert(IsValidIndex(Index));
        return Arr[Index];
    }

    const T& operator[](uint32 Index) const
    {
        assert(IsValidIndex(Index));
        return Arr[Index];
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
