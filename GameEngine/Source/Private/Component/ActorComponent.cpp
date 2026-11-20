#include "../../Public/Component/ActorComponent.h"
#include "../../Public/Actor/Actor.h"
#include "../../Public/Tick/TickManager.h"
#include "../../Public/Core/Engine.h"


UActorComponent::UActorComponent(AActor* _Owner, ETickingGroup _TickGroup = ETickingGroup::TG_DuringPhysics_ComponentTick) : Owner(_Owner)
{
	PrimaryComponentTick.Target = this;
	PrimaryComponentTick.TickGroup = _TickGroup;
}

UActorComponent::~UActorComponent()
{
	UEngine::GetEngine()->GetTickManager()->UnregisterTickFunction(&PrimaryComponentTick);
	Owner->RemoveComponent(this);
}

void UActorComponent::Init()
{
	UEngine::GetEngine()->GetTickManager()->RegisterTickFunction(&PrimaryComponentTick);
}

void UActorComponent::Tick(float DeltaTime)
{
}
