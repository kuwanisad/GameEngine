#include "../../Public/Input/InputSubsystem.h"
#include "../../Public/Input/EnhancedInputComponent.h"

void UInputSubsystem::RegisterInputComponent(UEnhancedInputComponent* Comp)
{
	RegisteredComponents.push_back(Comp);
}

void UInputSubsystem::UnregisterInputComponent(UEnhancedInputComponent* Comp)
{
	auto it = std::find(RegisteredComponents.begin(), RegisteredComponents.end(), Comp);
	if (it != RegisteredComponents.end())
	{
		RegisteredComponents.erase(it);
	}
}

void UInputSubsystem::ProcessInput(const FRawInputState& RawInput)
{
	TArray<UEnhancedInputComponent*> Snapshot = RegisteredComponents;
	for (auto Comp : Snapshot)
	{
		Comp->ProcessInput(RawInput);
	}
}
