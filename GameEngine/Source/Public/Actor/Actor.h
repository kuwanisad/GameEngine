#pragma once
#include "../Object/Object.h"
#include "../Tick/ActorTickFunction.h"

class UActorComponent;
class UPrimitiveComponent;
class USceneComponent;
struct FCollision;

class AActor : public UObject
{
private:
	USceneComponent* RootComponent;

	TArray<UActorComponent*> Components;

	//Physics 레이어 등록은 한 번만 일어남.
	bool IsRegisteredInPhysicsLayer = false;

public:
	AActor();
	~AActor();

	virtual void Init();
	virtual void Tick(float DeltaTime);

	void SetIsPendingKill(bool _IsPendingKill) { IsPendingKill = _IsPendingKill; }
	bool GetIsPendingKill() const { return IsPendingKill; }

	FVector3 GetActorLocation() const;
	void SetActorLocation(const FVector3& pos, bool Sweep = false, float margin = 0.0f);

	const FVector3& GetActorScale() const;
	void SetActorScale(const FVector3& scale);

	FQuaternion GetActorRotation() const;
	void SetActorRotation(FQuaternion rotation);

	//RootComponent의 ComputeWorldTransform 함수를 호출함;
	void ComputeWorldTransform();

	FMatrix4x4 GetActorTransform() const;

	void AddComponent(UActorComponent* component);
	void RemoveComponent(UActorComponent* component);

	TArray<FCollision*> GetCollisions() const;
	std::vector<UPrimitiveComponent*> GetPrimitiveComponents() const;

	FVector3 GetActorForwardVector() const;
	FVector3 GetActorRightVector() const;

	USceneComponent* GetRootComponent() const { return RootComponent; }
	bool SetRootComponent(USceneComponent* NewRootComponent);

	//Actor가 Collision을 가지게 되면 호출됨.
	//TODO: 콜리전 삭제도 고려할 것
	void RegisterPhysicsBodyIfNeeded();

	FActorTickFunction PrimaryActorTick;
};