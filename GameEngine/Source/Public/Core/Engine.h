#pragma once
#include "CoreMinimal.h"

class AActor;

class UEngine
{
private:
	static UEngine* Instance;

	TArray<AActor*> Actors;
	TArray<AActor*> PendingActors;

	bool IsUpdatingActors = false;

public:
	UEngine();
	~UEngine();

	static UEngine* Get();

	void AddActor(AActor* InActor);
	void RemoveActor(AActor* _Actor);
};