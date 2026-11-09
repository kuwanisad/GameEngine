#include "../../Public/Animation/AnimTrack.h"
#include "../../Public/Core/Math.h"

#include <algorithm>

FBoneTransform FAnimTrack::Sample(float TargetTime) const
{
	if (Times.empty())
	{
		return FBoneTransform();
	}

	if (TargetTime <= Times.front())
	{
		return Values.front();
	}

	if (Times.back() <= TargetTime)
	{
		return Values.back();
	}

	auto UpperItr = std::upper_bound(Times.begin(), Times.end(), TargetTime);
	int32 UpperIndex = UpperItr - Times.begin();
	int32 LowerIndex = Math::Max(UpperIndex - 1, 0);

	float Interval = Times[UpperIndex] - Times[LowerIndex];
	float Alpha = Math::NearZero(Interval) ? 0.f : (TargetTime - Times[LowerIndex]) / Interval;
	return FBoneTransform::Interpolate(Values[UpperIndex], Values[LowerIndex], Alpha);
}