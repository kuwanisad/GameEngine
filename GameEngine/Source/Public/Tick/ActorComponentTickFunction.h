#pragma once
#include "TickFunction.h"

class UActorComponent;

class FActorComponentTickFunction : public FTickFunction
{
public:
	UActorComponent* Target = nullptr;

	void ExecuteTick(float DeltaTime) override;
};