#pragma once
#include <cstdint>

#include "../Core/CoreMinimal.h"

using FAssetHandle = uint32;

struct FInstanceHandle
{
	uint32 Index = 0;
	uint32 Generation = 0;

	bool operator==(const FInstanceHandle& Other) const
	{
		return Index == Other.Index && Generation == Other.Generation;
	}
};