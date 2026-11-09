#pragma once
#include "SceneComponent.h"
#include "../Physics/BodyInstance.h"

class UPrimitiveComponent : public USceneComponent
{
public:
	UPrimitiveComponent(AActor* _Owner);

	// UE의 UPrimitiveComponent::BodyInstance에 대응 - 이 컴포넌트의 물리 상태(질량, 속도,
	// 힘/충격량 등). 지금은 데이터만 있고 매 프레임 이걸 실제로 적분하는 로직은 아직 없음
	// (FBodyInstance.h의 [범위] 주석 참고).
	FBodyInstance BodyInstance;

	virtual void OnDestroy() override;
};