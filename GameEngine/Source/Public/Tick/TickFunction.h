#pragma once
#include <vector>
#include "TickGroup.h"

class FTickFunction
{
public:
	virtual ~FTickFunction() = default;

	virtual void ExecuteTick(float DeltaTime) = 0;

	//AI로 확인함. 틱이 트리거되는 조건이 따로 있었음.
	void AddPrerequisite(FTickFunction* Other)
	{
		Prerequisites.push_back(Other);
	}

	ETickingGroup TickGroup = ETickingGroup::TG_PrePhysics;
	std::vector<FTickFunction*> Prerequisites;

	bool CanEverTick = false;
	bool HasTickedThisFrame = false;
};