#pragma once
#include "../Object/Object.h"
#include "../Tick/ActorTickFunction.h"

class UActorComponent;
class FCollision;
class UPrimitiveComponent;
class USceneComponent;

class AActor : public UObject
{
private:
	USceneComponent* RootComponent;

	TArray<UActorComponent*> Components;

	//Registration with the Physics layer only happens once.
	bool IsRegisteredInPhysicsLayer = false;

public:
	AActor();
	~AActor();

	virtual void Init();
	virtual void Tick(float DeltaTime);

	bool SetIsPendingKill(bool _IsPendingKill) { IsPendingKill = _IsPendingKill; }
	bool GetIsPendingKill() const { return IsPendingKill; }

	FVector3 GetActorLocation() const;
	void SetActorLocation(const FVector3& pos, bool Sweep = false, float margin = 0.0f);

	const FVector3& GetActorScale() const;
	void SetActorScale(const FVector3& scale);

	FQuaternion GetActorRotation() const;
	void SetActorRotation(FQuaternion rotation);

	//Call ComputeWorldTransform Function of RootComponent;
	void ComputeWorldTransform();

	void AddComponent(UActorComponent* component);
	void RemoveComponent(UActorComponent* component);

	TArray<FCollision*> GetCollisions() const;
	std::vector<UPrimitiveComponent*> GetPrimitiveComponents() const;

	FVector3 GetActorForwardVector() const;
	FVector3 GetActorRightVector() const;

	USceneComponent* GetRootComponent() const { return RootComponent; }
	bool SetRootComponent(USceneComponent* NewRootComponent);

	//Calls when Actor have any Collision.
	//TODO: Consider collision deletion
	void RegisterPhysicsBodyIfNeeded();

	FActorTickFunction PrimaryActorTick;
};