#pragma once
#include "TickFunction.h"

class AActor;

class FActorTickFunction : public FTickFunction
{
public:
	AActor* Target = nullptr;

	// FTickFunction을(를) 통해 상속됨
	void ExecuteTick(float DeltaTime) override;
};