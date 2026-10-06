#pragma once
#include "../Core/CoreMinimal.h"
#include "Keys.h"
#include "InputAction.h"
#include "InputModifier.h"

//UE에서 GUI로 IMC에서 InputAction 등록하고 한 입력에 설정을 적용하면 그걸 하나의 FEnhancedActionKeyMapping 객체가 담당
struct FEnhancedActionKeyMapping
{
	EKeys Key;
	UInputAction* Action;
	TArray<UInputModifier*> Modifiers;

	//개인적으로 Modifiers가 빈 경우가 적지 않았음...
	FEnhancedActionKeyMapping(EKeys InKey, UInputAction* InAction, TArray<UInputModifier*> InModifiers = {})
		: Key(InKey)
		, Action(InAction)
		, Modifiers(InModifiers)
	{
	}
};