#pragma once
#include "InputActionValue.h"

class UInputModifier
{
public:
	virtual ~UInputModifier() = default;
	virtual FInputActionValue ModifyRaw(const FInputActionValue& CurrentValue) = 0;
};