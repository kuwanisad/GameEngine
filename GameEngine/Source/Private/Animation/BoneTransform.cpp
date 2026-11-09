#include "../../Public/Animation/BoneTransform.h"

FMatrix4x4 FBoneTransform::ToMatrix() const
{
	return FMatrix4x4::TransformQuaternionToMatrix(Rotation) * FMatrix4x4::TransformTranslationToMatrix(Translation);
}

FBoneTransform FBoneTransform::Interpolate(const FBoneTransform& a, const FBoneTransform& b, float f)
{
	FBoneTransform Result;
	Result.Rotation = FQuaternion::Slerp(a.Rotation, b.Rotation, f);
	Result.Translation = FVector3::Lerp(a.Translation, b.Translation, f);
	return Result;
}
