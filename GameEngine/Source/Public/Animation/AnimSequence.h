#pragma once
#include "../Core/CoreMinimal.h"
#include "AnimTrack.h"

class UAnimSequence
{
public:
	FString SequenceName;
	float PlayLength = 0.0f;
	bool bLoop = true;

	bool bForceRootLock = false;

	TArray<FAnimTrack> BoneTracks;

	FBoneTransform GetBoneTransform(int32 BoneIndex, float Time) const;
};
