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
}

USkeletalMeshComponent::~USkeletalMeshComponent()
{
}

void USkeletalMeshComponent::SetAnimInstance(UAnimInstance* InAnimInstance)
{
	auto* RenderingManager = UEngine::GetEngine()->GetRenderingManager();

	if (InAnimInstance != nullptr)
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
