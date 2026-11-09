#pragma once
#include "../Core/Math.h"

//모델의 뼈
struct FBoneTransform
{
	FQuaternion Rotation;
	FVector3 Translation;

	FBoneTransform()
		: Rotation(FQuaternion::Identity)
		, Translation(FVector3::Zero)
	{
	}

	FBoneTransform(const FQuaternion& InRotation, const FVector3& InTranslation)
		: Rotation(InRotation)
		, Translation(InTranslation)
	{
	}

	FMatrix4x4 ToMatrix() const;

	static FBoneTransform Interpolate(const FBoneTransform& a, const FBoneTransform& b, float f);
};