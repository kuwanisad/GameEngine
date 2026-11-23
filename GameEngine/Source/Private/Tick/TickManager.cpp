#include "../../Public/Tick/TickManager.h"
#include "../../Public/Tick/TickFunction.h"
#include <algorithm>

void UTickManager::RegisterTickFunction(FTickFunction* TickFunction)
{
	TickFunctionsByGroup[static_cast<int>(TickFunction->TickGroup)].push_back(TickFunction);
}

void UTickManager::UnregisterTickFunction(FTickFunction* TickFunction)
{
	TArray<FTickFunction*>& Arr = TickFunctionsByGroup[static_cast<int32>(TickFunction->TickGroup)];
	auto iter = std::find(Arr.begin(), Arr.end(), TickFunction);
	if (iter != Arr.end())
	{
		Arr.erase(iter);
	}

	auto deferredIter = std::find(Deferred.begin(), Deferred.end(), TickFunction);
	if (deferredIter != Deferred.end())
	{
		Deferred.erase(deferredIter);
	}
}

void UTickManager::BeginFrame()
{
	for (int32 i = 0; i < NumTickingGroups; ++i)
	{
		for (FTickFunction* fn : TickFunctionsByGroup[i])
		{
			fn->HasTickedThisFrame = false;
		}
	}
	Deferred.clear();
}

void UTickManager::RunTickGroup(ETickingGroup Group, float DeltaTime)
{
	TArray<FTickFunction*> Snapshot = TickFunctionsByGroup[static_cast<int32>(Group)];

	for (FTickFunction* Tf : Snapshot)
	{
		if (TryExecute(Tf, DeltaTime) == false)
		{
			Deferred.emplace_back(Tf);
		}
	}

	if (Deferred.size())
	{
		TArray<FTickFunction*> RetrySnapshot = Deferred;
		Deferred.clear();
		for (FTickFunction* fn : RetrySnapshot)
		{
			if (TryExecute(fn, DeltaTime) == false)
			{
				Deferred.emplace_back(fn);
			}
		}
	}
}

bool UTickManager::TryExecute(FTickFunction* TickFunction, float DeltaTime)
{
	if (TickFunction == nullptr)
	{
		//조건이 비어있으면... 아마도.. 통과시킴!
		return true;
	}

	for (FTickFunction* Prereq : TickFunction->Prerequisites)
	{
		if (Prereq->HasTickedThisFrame == false)
		{
			return false;
		}
	}

	if (TickFunction->CanEverTick)
	{
		TickFunction->ExecuteTick(DeltaTime);
	}

	TickFunction->HasTickedThisFrame = true;
	return true;
}

UTickManager::UTickManager()
{
}

UTickManager::~UTickManager()
{
}
