#include "../../Public/Input/EnhancedInputComponent.h"
#include "../../Public/Input/InputMappingContext.h"
#include "../../Public/Input/InputTriggers.h"
#include "../../Public/Actor/Actor.h"
#include "../../Public/Core/Engine.h"
#include "../../Public/Input/InputSubsystem.h"

UEnhancedInputComponent::UEnhancedInputComponent(AActor* _Owner) : UActorComponent(_Owner)
{
	// [중복 등록 제거] 생성자에서도 RegisterInputComponent를 부르고 Init()에서도
	// 또 불러서, CreateComponent<T>()로 만들면(new 직후 바로 Init() 호출) 같은 컴포넌트가
	// UInputSubsystem::RegisteredComponents에 두 번 들어갔음 - RegisterInputComponent가
	// 중복 검사 없이 그냥 push_back이라 실제로 리스트에 두 개가 쌓임. 그러면 입력
	// 콜백(BindAction)이 매 프레임 두 번씩 불려서(예: 점프 임펄스가 두 번 적용) 게임
	// 로직이 미묘하게 틀어지고, UnregisterInputComponent는 처음 찾은 것 하나만 지우므로
	// 액터가 죽은 뒤에도 나머지 하나가 댕글링 포인터로 리스트에 남아 다음 프레임
	// 입력 처리에서 해제된 메모리를 참조하는 크래시로 이어질 수 있음. 등록은 Init()
	// 한 곳에서만 하도록 여기선 제거.
}

UEnhancedInputComponent::~UEnhancedInputComponent()
{
	UEngine::GetEngine()->GetInputSubsystem()->UnregisterInputComponent(this);

	for (auto& Binding : Bindings)
	{
		//Action은 돌려쓸 가능성이 있어 여기서 하지 않음
		delete Binding.Trigger;
	}
}

void UEnhancedInputComponent::Init()
{
	// [체이닝 필수 - 다른 컴포넌트들과 동일한 버그] 베이스를 안 부르면 Owner->AddComponent
	// (this)가 빠져서 AActor 소멸자가 이 컴포넌트를 못 지움 - 그러면 소멸자의
	// UnregisterInputComponent 호출도 영원히 안 일어나 위에서 설명한 댕글링 포인터
	// 문제가 그대로 남음.
	UActorComponent::Init();

	UEngine::GetEngine()->GetInputSubsystem()->RegisterInputComponent(this);
}

void UEnhancedInputComponent::BindAction(UInputAction* Action, ETriggerEvent Event, UInputTrigger* Trigger, FActionDelegate Callback)
{
	FActionBinding Binding;
	Binding.Action = Action;
	Binding.FilterEvent = Event;
	Binding.Trigger = Trigger;
	Binding.Callback = Callback;
	Bindings.push_back(Binding);
}

void UEnhancedInputComponent::BindAction(UInputAction* Action, FActionDelegate Callback)
{
	BindAction(Action, ETriggerEvent::Triggered, new UInputTriggerDown(), Callback);
}

void UEnhancedInputComponent::ProcessInput(const FRawInputState& RawInput)
{
	ActionValues.clear();

	if (CurrentContext == nullptr)
	{
		return;
	}

	for (const auto& Mapping : CurrentContext->GetMappings())
	{
		float RawValue = 0.f;

		switch (Mapping.Key)
		{
		case EKeys::MouseX:
		case EKeys::MouseY:
			RawValue = RawInput.GetAxis(Mapping.Key);
			break;
		default:
			RawValue = RawInput.IsPressed(Mapping.Key) ? 1.f : 0.f;
		}

		FInputActionValue Value{ RawValue };
		for (auto Modifier : Mapping.Modifiers)
		{
			Value = Modifier->ModifyRaw(Value);
		}

		//wasd 이동의 경우 대입은 깨질 수 있어 더해야 함
		ActionValues[Mapping.Action] += Value;
	}

	for (auto& Pair : ActionValues)
	{
		auto& value = Pair.second;
		value.Value.x = Math::Clamp(value.Value.x, -1.f, 1.f);
		value.Value.y = Math::Clamp(value.Value.y, -1.f, 1.f);
		value.Value.z = Math::Clamp(value.Value.z, -1.f, 1.f);
	}

	// 이제 호출해야 함

	for (auto& Binding : Bindings)
	{
		auto it = ActionValues.find(Binding.Action);
		FInputActionValue ActionValue = (it != ActionValues.end()) ? it->second : FInputActionValue{};

		ETriggerState NewState = Binding.Trigger->UpdateState(ActionValue);
		ETriggerEvent Event = ComputeTriggerEvent(Binding.LastState, NewState);
		Binding.LastState = NewState;

		//Triggered로 필터링된 바인딩은 발동 중인 프레임과 방금
		//멈춘 그 프레임(Completed)에도 한 번은 불려야 함.
		//안 그러면 SetForwardSpeed/SetAngularSpeed처럼 마지막 값을 계속 들고 있는 상태형 세터들이 
		//0을 받을 기회가 없어서 키를 뗀 뒤에도 마지막 입력값이 영원히 유지되는 버그가 생김
		bool bShouldFire = (Binding.FilterEvent == ETriggerEvent::Triggered)
			? (NewState == ETriggerState::Triggered || Event == ETriggerEvent::Completed)
			: (Event == Binding.FilterEvent);

		if (bShouldFire)
		{
			Binding.Callback(ActionValue);
		}
	}
}

ETriggerEvent UEnhancedInputComponent::ComputeTriggerEvent(ETriggerState LastState, ETriggerState NewState)
{
	switch (NewState)
	{
	case ETriggerState::Triggered:
		if (LastState != ETriggerState::Triggered)
		{
			return ETriggerEvent::Started;
		}
		else
		{
			return ETriggerEvent::Triggered;
		}
		break;
	case ETriggerState::None:
		if (LastState == ETriggerState::Triggered)
		{
			return ETriggerEvent::Completed;
		}
		break;
	}

	return ETriggerEvent::None;
}
