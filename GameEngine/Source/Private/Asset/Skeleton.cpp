#include "../../Public/Asset/Skeleton.h"

const TArray<FMatrix4x4>& USkeleton::GetInverseBindPoseComponentSpace() const
{
	if (IsInverseBindPoseCached == false)
	{
		TArray<FTransform> BindPose = ReferenceSkeleton.GetBonePoseComponentSpace();
		CachedInverseBindPose.resize(BindPose.size());

		for (int32 i = 0; i < BindPose.size(); ++i)
		{
			FMatrix4x4 BindMatrix = BindPose[i].ToMatrix();
			BindMatrix.Invert();
			CachedInverseBindPose[i] = BindMatrix;
		}

		IsInverseBindPoseCached = true;
	}

	return CachedInverseBindPose;
}

int32 UReferenceSkeleton::Add(const FMeshBoneInfo& BoneInfo, const FTransform& BonePose)
{
	int32 NewIndex = static_cast<int32>(BoneInfos.size());

	BoneInfos.emplace_back(BoneInfo);
	BonePoses.emplace_back(BonePose);

	NameToIndexMap[BoneInfo.Name] = NewIndex;

	return NewIndex;
}

int32 UReferenceSkeleton::FindBoneIndex(const FString& BoneName) const
{
	auto It = NameToIndexMap.find(BoneName);
	return (It != NameToIndexMap.end()) ? It->second : INDEX_NONE;
}

TArray<FTransform> UReferenceSkeleton::GetBonePoseComponentSpace() const
{
	TArray<FTransform> Out(BonePoses.size());

	for (int32 BoneIndex = 0; BoneIndex < BonePoses.size(); ++BoneIndex)
	{
		int32 ParentIndex = BoneInfos[BoneIndex].ParentIndex;

		if (ParentIndex == INDEX_NONE)
		{
			Out[BoneIndex] = BonePoses[BoneIndex];
		}
		else
		{
			Out[BoneIndex] = FTransform::Multiply(BonePoses[BoneIndex], Out[ParentIndex]);
		}
	}

	return Out;
}
