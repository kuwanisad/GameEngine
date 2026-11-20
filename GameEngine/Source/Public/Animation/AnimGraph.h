#pragma once
#include "AnimGraphBase.h"
#include "AnimNode_Base.h"
#include "AnimNode_SaveCachedPose.h"
#include "../Core/CoreMinimal.h"

//각 애님 블루프린트는 독립된 클래스로써 존재한다고 하여 템플릿으로 모작했음
//각 스냅샷은 메인 스레드에서 가속도, 액터 이동 속도 등으로 채워진 값
template<typename TSnapshot>
class TAnimGraph : public IAnimGraphBase
{
public:
	~TAnimGraph()
	{
		for (UAnimNode_Base<TSnapshot>* Node : AllNodes)
		{
			delete Node;
		}
	}

	void SetOutPose(UAnimNode_Base<TSnapshot>* Value) { OutPose = Value; }
	UAnimNode_Base<TSnapshot>* GetOutPose() const { return OutPose; }

	void RegisterNode(UAnimNode_Base<TSnapshot>* Node)
	{
		Node->OwningGraph = this;
		AllNodes.insert(Node);
	}

	void RegisterCachedPoseNode(UAnimNode_SaveCachedPose<TSnapshot>* Node)
	{
		RegisterNode(Node);
		CachedPoseNodes.insert(Node);
	}

	void Update(float DeltaSeconds, const FAnimSnapshotBase& Snapshot) override
	{
		UpdateTyped(DeltaSeconds, static_cast<const TSnapshot&>(Snapshot));
	}

	void Evaluate(TArray<FBoneTransform>& OutBonePose, const FAnimSnapshotBase& Snapshot) override
	{
		EvaluateTyped(OutBonePose, static_cast<const TSnapshot&>(Snapshot));
	}

	void UpdateTyped(float DeltaSeconds, const TSnapshot& Snapshot)
	{
		for (UAnimNode_SaveCachedPose<TSnapshot>* Node : CachedPoseNodes)
		{
			Node->ResetFrame();
		}

		if (OutPose != nullptr)
		{
			OutPose->Update(DeltaSeconds, Snapshot);
		}
	}

	void EvaluateTyped(TArray<FBoneTransform>& OutBonePose, const TSnapshot& Snapshot)
	{
		if (OutPose != nullptr)
		{
			OutPose->Evaluate(OutBonePose, Snapshot);
		}
	}

private:
	UAnimNode_Base<TSnapshot>* OutPose = nullptr;
	TSet<UAnimNode_Base<TSnapshot>*> AllNodes;
	TSet<UAnimNode_SaveCachedPose<TSnapshot>*> CachedPoseNodes;
};
