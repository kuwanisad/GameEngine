#pragma once
#include <vector>
#include "TickGroup.h"

class FTickFunction
{
public:
	virtual ~FTickFunction() = default;

	virtual void ExecuteTick(float DeltaTime) = 0;

	//Confirmed via AI. There were conditions for triggering the tick.
	void AddPrerequisite(FTickFunction* Other)
	{
		Prerequisites.push_back(Other);
	}

	ETickingGroup TickGroup = ETickingGroup::TG_PrePhysics;
	std::vector<FTickFunction*> Prerequisites;

	bool CanEverTick = false;
	bool HasTickedThisFrame = false;
};