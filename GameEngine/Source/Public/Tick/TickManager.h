#pragma once
#include "TickGroup.h"
#include "../Core/CoreMinimal.h"

class FTickFunction;


class UTickManager
{
public:
	UTickManager();
	~UTickManager();

	void RegisterTickFunction(FTickFunction* TickFunction);
	void UnregisterTickFunction(FTickFunction* TickFunction);

	//Call once per frame. this is start of frame.
	void BeginFrame();

	void RunTickGroup(ETickingGroup Group, float DeltaTime);

private:

	bool TryExecute(FTickFunction* TickFunction, float DeltaTime);

	std::vector<FTickFunction*> TickFunctionsByGroup[NumTickingGroups];

	// Tick Prerequisite
	std::vector<FTickFunction*> Deferred;
};