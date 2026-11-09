#pragma once
#include "../Core/CoreMinimal.h"
#include "AnimSnapshotBase.h"
#include "BoneTransform.h"

//URenderingManager가 이 클래스 객체를 써서 각 애니메이션을 랜더링해야 함
class IAnimGraphBase
{
public:
	virtual void Update(float DeltaSeconds, const FAnimSnapshotBase& Snapshot) = 0;
	virtual void Evaluate(TArray<FBoneTransform>& OutBonePose, const FAnimSnapshotBase& Snapshot) = 0;
};