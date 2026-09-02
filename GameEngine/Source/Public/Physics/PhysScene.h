#pragma once
#include "../Core/CoreMinimal.h"
#include "../Core/Math.h"

class AActor;

struct FPhysScene
{
private:
	static FPhysScene* Instance;

	TArray<AActor*> Actors;
	TArray<AActor*> PendingActors;

	bool IsUpdatingActors = false;

public:
	FPhysScene();
	~FPhysScene();

	bool RegisterBody(AActor* _actor);
	void UnregisterBody(AActor* _actor);

	void MoveActor(AActor* _Actor, const FVector3& DesiredPos, float margin);


	static FPhysScene* Get();
};