#pragma once
#include "../Core/CoreMinimal.h"
#include "../Core/Math.h"
#include "BoneTransform.h"

//여러 타입의 트랙이 있어야 하지만 먼저 트랜스폼을 끝내고 해야 함
struct FAnimTrack
{
	TArray<float> Times;
	TArray<FBoneTransform> Values;

	FBoneTransform Sample(float TargetTime) const;
};