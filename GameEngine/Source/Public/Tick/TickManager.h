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

	//매 프레임 한 번 호출됨. 프레임 시작 지점.
	void BeginFrame();

	void RunTickGroup(ETickingGroup Group, float DeltaTime);

private:

	bool TryExecute(FTickFunction* TickFunction, float DeltaTime);

	std::vector<FTickFunction*> TickFunctionsByGroup[NumTickingGroups];

	// 틱 선행 조건(Prerequisite)
	std::vector<FTickFunction*> Deferred;
};