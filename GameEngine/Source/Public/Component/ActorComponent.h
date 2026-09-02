#pragma once
#include "../Object/Object.h"
#include "../Tick/TickGroup.h"
#include "../Tick/ActorComponentTickFunction.h"
#include "ComponentType.h"

class AActor;

class UActorComponent : public UObject
{
protected:
	AActor* Owner;
	EComponentType Type = EComponentType::None;

public:
	UActorComponent(AActor* _Owner);
	virtual ~UActorComponent();

	virtual void Init() override;

	virtual void Tick(float DeltaTime);
	virtual void OnUpdateWorldTransform() {}

	EComponentType GetComponentType() const { return Type; }
	ETickingGroup GetTickGroup() const { return PrimaryComponentTick.TickGroup; }

	void SetOwner(AActor* InActor) { Owner = InActor; }
	AActor* GetOwner() const { return Owner; }

	FActorComponentTickFunction PrimaryComponentTick;
};