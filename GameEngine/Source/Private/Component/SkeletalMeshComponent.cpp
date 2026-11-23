#include "../../Public/Component/SkeletalMeshComponent.h"
#include "../../Public/Animation/AnimInstance.h"
#include "../../Public/Rendering/RenderingManager.h"
#include "../../Public/Core/Engine.h"

USkeletalMeshComponent::USkeletalMeshComponent(AActor* _Owner)
	: UPrimitiveComponent(_Owner),
	AnimInstance(nullptr),
	Visible(true)
{
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;

	// PrimaryComponentTick.CanEverTick 기본값은 false(실제 UE와 동일) - 물리로 트랜스폼이
	// 확정된 뒤 RenderingManager에 넘기는 작업이 Tick() 안에 있으므로 켜줘야 함.
	PrimaryComponentTick.CanEverTick = true;
}

USkeletalMeshComponent::~USkeletalMeshComponent()
{
}

void USkeletalMeshComponent::SetAnimInstance(UAnimInstance* InAnimInstance)
{
	auto* RenderingManager = UEngine::GetEngine()->GetRenderingManager();

	// [조건 반전 버그] 여기서 정리하려는 대상은 "기존에 들고 있던" AnimInstance(멤버)인데,
	// 조건은 "새로 들어온" InAnimInstance(파라미터)의 null 여부를 보고 있었음 - 처음
	// SetAnimInstance를 호출하는 경우(ThirdPersonCharacter 생성자처럼) 멤버 AnimInstance는
	// 아직 nullptr인데 InAnimInstance는 유효한 값이라 이 블록이 실행돼버려서, null인
	// AnimInstance->RenderInstanceHandle을 그대로 역참조해 죽었음. "지울 기존 값이
	// 있는지"를 봐야 하므로 멤버 쪽을 검사해야 함.
	if (AnimInstance != nullptr)
	{
		RenderingManager->UnregisterInstance(AnimInstance->RenderInstanceHandle);
		delete AnimInstance;
	}

	AnimInstance = InAnimInstance;

	if (AnimInstance != nullptr)
	{
		AnimInstance->RenderInstanceHandle = RenderingManager->RegisterInstance(
			AnimInstance, AnimInstance->SkeletonHandle, AnimInstance->SkeletalMeshHandle, 
			AnimInstance->AnimGraphHandle);

		AnimInstance->OwningActor = Owner;
	}
}

void USkeletalMeshComponent::Tick(float DeltaSeconds)
{
	if (AnimInstance == nullptr || Visible == false)
	{
		return;
	}

	AnimInstance->NativeUpdateAnimation(DeltaSeconds);

	UEngine::GetEngine()->GetRenderingManager()->UpdateSkeletalMeshTransform(
		AnimInstance->RenderInstanceHandle,
		GetComponentLocation(),
		GetComponentRotation(),
		GetComponentScale3D());
}

void USkeletalMeshComponent::OnDestroy()
{
	if (AnimInstance != nullptr)
	{
		UEngine::GetEngine()->GetRenderingManager()->UnregisterInstance(AnimInstance->RenderInstanceHandle);
		delete AnimInstance;
		AnimInstance = nullptr;
	}
}
