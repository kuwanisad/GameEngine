#include "../../Public/Animation/AnimSequence.h"

FBoneTransform UAnimSequence::GetBoneTransform(int32 BoneIndex, float Time) const
{
	if (BoneIndex < 0 || BoneIndex >= BoneTracks.size())
	{
		return FBoneTransform();
	}

	return BoneTracks[BoneIndex].Sample(Time);
}
