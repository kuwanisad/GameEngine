#include "../../Public/Component/ActorComponent.h"
#include "../../Public/Actor/Actor.h"
#include "../../Public/Tick/TickManager.h"


UActorComponent::UActorComponent(AActor* _Owner) : Owner(_Owner)
{
	PrimaryComponentTick.Target = this;

	//This is Child's responsibility.
	//PrimaryComponentTick.TickGroup = 
}

UActorComponent::~UActorComponent()
{
	UTickManager::Get()->UnregisterTickFunction(&PrimaryComponentTick);
}

void UActorComponent::Init()
{
	UTickManager::Get()->RegisterTickFunction(&PrimaryComponentTick);
}

void UActorComponent::Tick(float DeltaTime)
{
}
