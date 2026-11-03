#pragma once
#include "../Core/CoreMinimal.h"
#include "../Asset/AssetHandle.h"
#include "../Core/Math.h"
#include "../Animation/AnimTaskFence.h"
#include "../Animation/AnimSnapshotBase.h"

#include <atomic>

class UAnimInstance;
class IAnimGraphBase;

struct FSkeletalMeshInstance
{
	uint32 SelfIndex = 0;
	uint32 Generation = 0;
	bool IsAlive = true;
	bool IsSlotOccupied = false;

	FAssetHandle SkeletonHandle = 0;
	FAssetHandle SkeletalMeshHandle = 0;

	//애님 그래프는 각 인스턴스마다 하나
	IAnimGraphBase* Graph = nullptr;

	//메인 스레드가 쓰고 랜더링측은 쓰면 안 됨
	UAnimInstance* OwningAnimInstance = nullptr;

	//메인스레드측에서 업데이트시킨 멤버
	FVector3 LatestWorldPosition = FVector3::Zero;
	FQuaternion LatestWorldRotation = FQuaternion::Identity;
	FVector3 LatestWorldScale = FVector3::One;

	//렌더링에 사용 중인 멤버
	FVector3 DrawWorldPosition = FVector3::Zero;
	FQuaternion DrawWorldRotation = FQuaternion::Identity;
	FVector3 DrawWorldScale = FVector3::One;

	FAnimSnapshotBase* CurrentSnapshot = nullptr;
	std::atomic<FCompleteCounter> CompletedCount{ 0 };
	FAnimTaskFence RenderFence;

	//큐로 하는 편이 낫지 않을까 싶지만 다른 객체가 아직 안 된 경우 기존값을 써야 하므로 일단 존치
	TArray<TArray<FMatrix4x4>> RenderingPalette;
	int32 ReadIndex = 0;

	//작업이 진행돼서 ReadIndex ++해야 함
	bool IsHasPendingSwap = false;
};