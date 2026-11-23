#include "../../Public/Component/ActorComponent.h"
#include "../../Public/Actor/Actor.h"
#include "../../Public/Tick/TickManager.h"
#include "../../Public/Core/Engine.h"


UActorComponent::UActorComponent(AActor* _Owner, ETickingGroup _TickGroup) : Owner(_Owner)
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
	// [등록 지점] AttachToComponent/SetupAttachment/SetRootComponent 같은 "부착" 시점에
	// 등록하면, USceneComponent가 아닌(부착 개념 자체가 없는) UMovementComponent/
	// UEnhancedInputComponent 같은 컴포넌트는 영원히 Owner->Components에 안 들어가서
	// 소멸자에서 못 지워지고 누수됨 - 모든 컴포넌트 종류를 빠짐없이 포괄하는 지점은
	// 생성/초기화(Init) 시점뿐임. AActor::Init()이 UEngine::AddActor를 부르는 것과
	// 정확히 같은 패턴.
	Owner->AddComponent(this);

	UEngine::GetEngine()->GetTickManager()->RegisterTickFunction(&PrimaryComponentTick);
}

void UActorComponent::Tick(float DeltaTime)
{
}
