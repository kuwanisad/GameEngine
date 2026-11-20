#pragma once
#include "AnimNode_Base.h"
#include "AnimNode_SaveCachedPose.h"

// UE의 "Cached Pose" 참조 노드에 대응(축소판). 자기 자신은 아무 계산도 안 하고,
// 그냥 어딘가에 있는 FAnimNode_SaveCachedPose를 가리키기만 함 - Update/Evaluate가
// 불릴 때마다 그 대상한테 그대로 전달하고, 실제 메모이제이션(이번 프레임에 이미
// 계산했는지 판단)은 전부 SaveCachedPose 쪽 책임.
template<typename TSnapshot>
class FAnimNode_UseCachedPose : public FAnimNode_Base<TSnapshot>
{
public:
	FAnimNode_SaveCachedPose<TSnapshot>* Source = nullptr;

	void Update(float DeltaSeconds, const TSnapshot& Snapshot) override
	{
		if (Source != nullptr)
		{
			Source->Update(DeltaSeconds, Snapshot);
		}
	}

	void Evaluate(TArray<FBoneTransform>& OutPose, const TSnapshot& Snapshot) override
	{
		if (Source != nullptr)
		{
			Source->Evaluate(OutPose, Snapshot);
		}
	}
};
