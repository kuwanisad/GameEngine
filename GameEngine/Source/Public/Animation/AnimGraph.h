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
private:
	//애님 블루프린트의 OutPose
	UAnimNode_Base<TSnapshot>* OutPose = nullptr;
public:
	TSet<UAnimNode_Base<TSnapshot>*> Nodes;
	TSet<UAnimNode_SaveCachedPose<TSnapshot>*> CachedPoseNodes;
};