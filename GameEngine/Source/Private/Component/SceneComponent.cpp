#include "../../Public/Component/SceneComponent.h"
#include "../../Public/Actor/Actor.h"

USceneComponent::USceneComponent(AActor* _Owner, ETickingGroup _TickGroup = ETickingGroup::TG_DuringPhysics_ComponentTick) : UActorComponent(_Owner, _TickGroup),
	ShouldRecomputeWorldTransform(true)
{
}

bool USceneComponent::AttachToComponent(USceneComponent* InParent, const FAttachmentTransformRules& AttachmentRules, const FString& InSocketName)
{
	if (InParent == nullptr)
	{
		return false;
	}

	if (InParent == this)
	{
		return false;
	}

	if (GetOwner() && GetOwner() == InParent->GetOwner() && GetOwner()->GetRootComponent() == this)
	{
		return false;
	}

	if (InParent->IsAttachedTo(this))
	{
		return false;
	}

	if (AttachParent == InParent && AttachSocketName == InSocketName)
	{
		return true;
	}

	if (AttachParent != nullptr)
	{
		DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	}

	AttachParent = InParent;
	AttachSocketName = InSocketName;
	InParent->AttachChildren.push_back(this);

	ShouldRecomputeWorldTransform = true;

	return true;
}

void USceneComponent::DetachFromComponent(const FDetachmentTransformRules& DetachmentRules)
{
	if (AttachParent == nullptr)
	{
		return;
	}

	TArray<USceneComponent*>& siblings = AttachParent->AttachChildren;
	siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());

	AttachParent = nullptr;
	AttachSocketName.clear();
	ShouldRecomputeWorldTransform = true;
}

bool USceneComponent::IsAttachedTo(USceneComponent* TestComp) const
{
	if (TestComp == nullptr)
	{
		return false;
	}

	for (USceneComponent* comp = AttachParent; comp != nullptr; comp = comp->AttachParent)
	{
		if (comp == TestComp)
		{
			return true;
		}
	}

	return false;
}

void USceneComponent::SetupAttachment(USceneComponent* InParent, const FString& InSocketName)
{
	if (InParent == AttachParent && InSocketName == AttachSocketName)
	{
		// UE 그대로: 바뀌는 게 없으면 아무것도 안 함
		return;
	}

	if (InParent == this)
	{
		return;
	}

	if (InParent != nullptr && InParent->IsAttachedTo(this))
	{
		return;
	}

	if (AttachParent != nullptr)
	{
		TArray<USceneComponent*>& siblings = AttachParent->AttachChildren;
		bool bAlreadyFullyAttached = std::find(siblings.begin(), siblings.end(), this) != siblings.end();
		if (bAlreadyFullyAttached)
		{
			return;
		}
	}

	AttachParent = InParent;
	AttachSocketName = InSocketName;

	// [Unlike UE] Populate AttachChildren immediately since we have no deferred component registration step.
	if (InParent != nullptr)
	{
		InParent->AttachChildren.push_back(this);
	}

	ShouldRecomputeWorldTransform = true;
}

void USceneComponent::ComputeWorldTransform()
{
	if (ShouldRecomputeWorldTransform)
	{
		ShouldRecomputeWorldTransform = false;
		FMatrix4x4 Local = FMatrix4x4::TransformScaleToMatrix(RelativeScale3D);
		Local *= FMatrix4x4::TransformQuaternionToMatrix(RelativeRotation);
		Local *= FMatrix4x4::TransformTranslationToMatrix(RelativeLocation);

		if (AttachParent != nullptr)
		{
			Local *= AttachParent->GetComponentTransform();

			WorldScale = RelativeScale3D * AttachParent->GetComponentScale3D();
			WorldRotation = FQuaternion::Concatenate(RelativeRotation, AttachParent->GetComponentRotation());
			WorldLocation = AttachParent->GetComponentLocation() +
				FVector3::Transform(RelativeLocation * AttachParent->GetComponentScale3D(), AttachParent->GetComponentRotation());
		}
		else
		{
			WorldScale = RelativeScale3D;
			WorldRotation = RelativeRotation;
			WorldLocation = RelativeLocation;
		}

		ComponentToWorld = Local;

		for (USceneComponent* child : AttachChildren)
		{
			child->ShouldRecomputeWorldTransform = true;
		}
	}

	for (USceneComponent* child : AttachChildren)
	{
		child->ComputeWorldTransform();
		child->OnUpdateWorldTransform();
	}
}
