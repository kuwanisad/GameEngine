#pragma once
#include "../Core/CoreMinimal.h"
#include "RawInputState.h"

class UEnhancedInputComponent;

class UInputSubsystem
{
public:
	void RegisterInputComponent(UEnhancedInputComponent* Comp);
	void UnregisterInputComponent(UEnhancedInputComponent* Comp);

	void ProcessInput(const FRawInputState& RawInput);

private:
	TArray<UEnhancedInputComponent*> RegisteredComponents;
};