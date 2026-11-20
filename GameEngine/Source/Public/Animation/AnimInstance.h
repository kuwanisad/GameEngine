#pragma once
#include "AnimSnapshotBase.h"
#include "AnimGraphRegistry.h"
#include "../Asset/AssetHandle.h"

class AActor;

class UAnimInstance
{
public:
	FAnimGraphHandle AnimGraphHandle;
	FAssetHandle SkeletonHandle = 0;
	FAssetHandle SkeletalMeshHandle = 0;
	FInstanceHandle RenderInstanceHandle;

	AActor* OwningActor = nullptr;

	virtual FAnimSnapshotBase* GetSnapshot() const = 0;
	virtual void NativeUpdateAnimation(float DeltaSeconds) {}
};