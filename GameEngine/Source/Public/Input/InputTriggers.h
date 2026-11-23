#pragma once
#include "InputTrigger.h"

//UInputTrigger 파생 클래스 정리용 헤더

class UInputTriggerDown : public UInputTrigger
{
public:
	ETriggerState UpdateState(const FInputActionValue& ModifiedValue) override
	{
		return ModifiedValue.IsNonZero() ? ETriggerState::Triggered : ETriggerState::None;
	}
};

class UInputTriggerPressed : public UInputTrigger
{
public:
	ETriggerState UpdateState(const FInputActionValue& ModifiedValue) override
	{
		bool IsActive = ModifiedValue.IsNonZero();
		ETriggerState Result = (IsActive && !WasActive) ? ETriggerState::Triggered : ETriggerState::None;
		WasActive = IsActive;
		return Result;
	}

private:
	bool WasActive = false;
};