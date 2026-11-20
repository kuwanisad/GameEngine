#include "../../Public/Component/BoxComponent.h"
#include "../../Public/Actor/Actor.h"
#include "../../Public/Core/Engine.h"
#include "../../Public/Physics/PhysScene.h"

UBoxComponent::UBoxComponent(AActor* owner) :
	UShapeComponent(owner),
	ShouldRotate(false)
{

}

UBoxComponent::~UBoxComponent()
{
	UEngine::GetEngine()->GetPhysScene()->RemoveCollision(GetCollision());
}

void UBoxComponent::Init()
{
	SetCollision(new FOBB(FVector3::Zero, FQuaternion::Identity, FVector3::Zero));
	UEngine::GetEngine()->GetPhysScene()->AddCollision(GetCollision());
}

void UBoxComponent::OnUpdateWorldTransform()
{
	// [부착 계층 반영] 예전엔 Owner->GetActorScale()/GetActorRotation()/GetActorLocation()을
	// 직접 읽었는데, 이러면 이 컴포넌트가 (RootComponent가 아닌) 다른 컴포넌트에 부착된
	// 경우 부모 트랜스폼이 전혀 반영이 안 됨 - 항상 "액터 전체의" 트랜스폼만 보는 셈이라,
	// 부착 계층 자체가 있으나 마나였음. 이제 이 컴포넌트 자신의 해석적으로 합성된 월드
	// 값(GetComponentScale/Rotation/Location - 부모가 있으면 이미 그 부모 트랜스폼까지
	// 반영해서 계산됨, 부모가 없으면 예전과 동일하게 액터 트랜스폼과 같음)을 씀.
	WorldBox = ObjectBox;

	WorldBox.SetMin(WorldBox.GetMin() * GetComponentScale3D());
	WorldBox.SetMax(WorldBox.GetMax() * GetComponentScale3D());

	// 회전 전(로컬 축 기준) 반너비. WorldBox.Rotate()는 회전된 박스를 다시
	// 축 정렬로 감싸느라 이보다 넓게 만들어버리므로, OBB의 Extent로는
	// 회전 직전의 이 값을 그대로 써야 함.
	FVector3 localExtent = WorldBox.GetExtent();

	if (ShouldRotate)
	{
		WorldBox.Rotate(GetComponentRotation());
	}

	WorldBox.SetMin(WorldBox.GetMin() += GetComponentLocation());
	WorldBox.SetMax(WorldBox.GetMax() += GetComponentLocation());

	// BVH에 들어가는 실제 충돌 도형(OBB)도 최신 상태로 갱신
	FOBB* box = static_cast<FOBB*>(GetCollision());
	box->SetCenter(WorldBox.GetCenter());
	box->SetExtent(localExtent);
	box->SetRotation(ShouldRotate ? GetComponentRotation() : FQuaternion::Identity);
}
