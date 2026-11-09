#pragma once
#include "../Core/CoreMinimal.h"
#include "BoneTransform.h"

template<typename TSnapshot>
class TAnimGraph;

//랜더링측의 애니메이션을 랜더링할 워커 스레드가 호출할 코드로 TSnapshot는 특정 틱에서 특정 객체의 랜더링에 필요한 데이터 사본
template<typename TSnapshot>
class UAnimNode_Base
{
public:
	virtual void Update(float DeltaSeconds, const TSnapshot& Snapshot) = 0;
	virtual void Evaluate(TArray<FBoneTransform>& OutPose, const TSnapshot& Snapshot) = 0;

	//각 애님 블루프린트의 OutPose에서 시작해 DFS를 돌 텐데 한 노드가 여러 노드와 연결됐을 수 있으니 재랜더링 방지를 위해 추가
	virtual bool IsFinished() const { return false; }
	
	//매 틱 랜더링 전에 호출됨
	virtual void ResetForStateEntry() {}

	//Automatic Rule Based on Sequence Player in State을 위해 추가
	virtual float GetPlayRatio() const { return 0.f; }

	TAnimGraph<TSnapshot>* OwningGraph = nullptr;
};