#pragma once
#include "AnimNode_Base.h"

template<typename TSnapshot>
class UAnimNode_SaveCachedPose : public UAnimNode_Base<TSnapshot>
{
private:
	TArray<FBoneTransform> CachedPose;
	bool IsUpdatedThisFrame = false;
	bool IsEvaluatedThisFrame = false;

public:
	//AnimNode_SaveCachedPose 객체가 저장하는 포즈를 랜더링할 객체 
	UAnimNode_Base<TSnapshot>* Input = nullptr;

	virtual void Update(float DeltaSeconds, const TSnapshot& Snapshot) override
	{
		if (IsUpdatedThisFrame == false)
		{
			if (Input != nullptr)
			{
				Input->Update(DeltaSeconds, Snapshot);
			}
			IsUpdatedThisFrame = true;
		}
	}

	virtual void Evaluate(TArray<FBoneTransform>& OutPose, const TSnapshot& Snapshot) override
	{
		if (IsEvaluatedThisFrame == false)
		{
			if (Input != nullptr)
			{
				Input->Evaluate(CachedPose, Snapshot);
			}
			IsEvaluatedThisFrame = true;
		}
		OutPose = CachedPose;
	}

	void ResetFrame()
	{
		IsUpdatedThisFrame = false;
		IsEvaluatedThisFrame = false;
	}
};