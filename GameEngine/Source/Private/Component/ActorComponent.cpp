#include "../../Public/Component/ActorComponent.h"
#include "../../Public/Actor/Actor.h"
#include "../../Public/Tick/TickManager.h"
#include "../../Public/Core/Engine.h"


UActorComponent::UActorComponent(AActor* _Owner) : Owner(_Owner)
{
	PrimaryComponentTick.Target = this;

	//This is Child's responsibility.
	//PrimaryComponentTick.TickGroup = 
}

UActorComponent::~UActorComponent()
{
	UEngine::GetEngine()->GetTickManager()->UnregisterTickFunction(&PrimaryComponentTick);
}

void UActorComponent::Init()
{
	UEngine::GetEngine()->GetTickManager()->RegisterTickFunction(&PrimaryComponentTick);
}

void UActorComponent::Tick(float DeltaTime)
{
}
