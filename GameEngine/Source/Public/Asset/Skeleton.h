#pragma once
#include "../Core/CoreMinimal.h"
#include "../Core/Math.h"

struct FMeshBoneInfo
{
	FString Name;
	int32 ParentIndex;

	FMeshBoneInfo() : ParentIndex(INDEX_NONE)
	{

	}

	FMeshBoneInfo(const FString& InName, int32 InParentIndex)
		: Name(InName)
		, ParentIndex(InParentIndex)
	{

	}
};

//본을 뺄 일은 없을 것 같아서 Remove는 없고 본은 부모에서 자식 순서대로 들어가야 함
class UReferenceSkeleton
{
private:
	TMap<FString, int32> NameToIndexMap;
	TArray<FMeshBoneInfo> BoneInfos;
	TArray<FTransform> BonePoses;
public:
	void Empty()
	{
		NameToIndexMap.clear();
		BoneInfos.clear();
		BonePoses.clear();
	}

	int32 Add(const FMeshBoneInfo& BoneInfo, const FTransform& BonePose);
	int32 FindBoneIndex(const FString& BoneName) const;

	int32 GetParentIndex(int32 BoneIndex) const
	{
		return BoneInfos[BoneIndex].ParentIndex;
	}

	int32 GetBoneCount() const { return static_cast<int32>(BoneInfos.size()); }

	const FMeshBoneInfo& GetRefBoneInfo(int32 BoneIndex) const { return BoneInfos[BoneIndex]; }
	const FTransform& GetRefBonePose(int32 BoneIndex) const { return BonePoses[BoneIndex]; }

	TArray<FTransform> GetBonePoseComponentSpace() const;
};

class USkeleton
{
public:
	const UReferenceSkeleton& GetReferenceSkeleton() const { return ReferenceSkeleton; }

	const TArray<FMatrix4x4>& GetInverseBindPoseComponentSpace() const;
private:
	UReferenceSkeleton ReferenceSkeleton;
	mutable TArray<FMatrix4x4> CachedInverseBindPose;
	mutable bool IsInverseBindPoseCached = false;
};