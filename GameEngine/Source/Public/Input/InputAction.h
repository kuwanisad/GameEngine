#pragma once
#include "../../Public/Core/CoreMinimal.h"
#include "InputActionValue.h"

class UInputAction
{
public:
	UInputAction(const FString& InActionName, EInputActionValueType InValueType)
		: ActionName(InActionName)
		, ValueType(InValueType)
	{

	}

	const FString& GetActionName() const { return ActionName; }
	const EInputActionValueType GetValueType() const { return ValueType; }

private:
	FString ActionName;
	EInputActionValueType ValueType;
};