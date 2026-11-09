#pragma once
#include "../Core/CoreMinimal.h"

#include <cstdint>

using FAssetHandle = uint32;

//동적 에셋을 위한 핸들
struct FInstanceHandle
{
	uint32 Index = 0;
	uint32 Generation = 0;

	bool operator==(const FInstanceHandle& Other) const
	{
		return Index == Other.Index && Generation == Other.Generation;
	}
};
