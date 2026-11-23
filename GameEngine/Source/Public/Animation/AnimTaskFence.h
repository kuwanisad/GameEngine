#pragma once
#include<atomic>
#include<thread>

#include "../Core/CoreMinimal.h"

//오버플로우 나도 됨. 그냥 다르다는 것만 알아도 되므로..
using FCompleteCounter = uint32;

struct FAnimTaskFence
{
private:
	std::atomic<FCompleteCounter>* Counter = nullptr;
	FCompleteCounter TargetValue = 0;

public:
	void BeginFance(std::atomic<FCompleteCounter>* InCounter)
	{
		Counter = InCounter;
		TargetValue = InCounter->load();
	}

	bool IsFenceComplete() const
	{
		return Counter == nullptr || Counter->load() != TargetValue;
	}

	void Wait() const
	{
		while (IsFenceComplete() == false)
		{
			std::this_thread::yield();
		}
	}
};