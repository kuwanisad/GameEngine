#pragma once
#include "../Component/ActorComponent.h"
#include "../Core/CoreMinimal.h"
#include "InputAction.h"
#include "RawInputState.h"
#include "InputActionValue.h"
#include "TriggerState.h"
#include "TriggerEvent.h"
#include <unordered_map>
#include <functional>

class UInputMappingContext;
class UInputTrigger;

// FRawInputState를 받아 UInputMappingContext(키 --> 액션 + Modifier 체인)를 거쳐 값을 계산하고
// BindAction 콜백에게 UInputTrigger로 판정한 ETriggerEvent가 맞을 때만 FInputActionValue를 전달
class UEnhancedInputComponent : public UActorComponent
{
public:
	using FActionDelegate = std::function<void(const FInputActionValue&)>;

	UEnhancedInputComponent(AActor* _Owner);
	virtual ~UEnhancedInputComponent();

	virtual void Init() override;


	void SetMappingContext(UInputMappingContext* Context) { CurrentContext = Context; }

	void BindAction(UInputAction* Action, ETriggerEvent Event, UInputTrigger* Trigger, FActionDelegate Callback);
	void BindAction(UInputAction* Action, FActionDelegate Callback);

	void ProcessInput(const FRawInputState& RawInput);

private:
	struct FActionBinding
	{
		UInputAction* Action;
		ETriggerEvent FilterEvent;
		UInputTrigger* Trigger;
		FActionDelegate Callback;
		ETriggerState LastState = ETriggerState::None;
	};

	static ETriggerEvent ComputeTriggerEvent(ETriggerState LastState, ETriggerState NewState);

	UInputMappingContext* CurrentContext = nullptr;
	TMap<UInputAction*, FInputActionValue> ActionValues;
	TArray<FActionBinding> Bindings;
};