#pragma once
#include "../Core/PCH.h"
#include "../Core/CoreMinimal.h"
#include "../Core/Math.h"

class UObject
{
protected:
	bool IsPendingKill = false;

public:
	UObject() = default;
	virtual ~UObject() = default;
	virtual void Init() = 0;
	virtual void OnDestroy();
};