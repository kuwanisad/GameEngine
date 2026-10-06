#pragma once
#include "InputActionValue.h"
#include "TriggerState.h"

//UE UInputTrigger 축소 버전
//조정된 입력값을 보고 발동 여부 판정
class UInputTrigger
{
public:
	virtual ~UInputTrigger() = default;
	virtual ETriggerState UpdateState(const FInputActionValue& ModifiedValue) = 0;
};
