#pragma once
#include "ActorComponent.h"
#include "AttachmentTransformRules.h"
#include "../Core/CoreMinimal.h"
#include "../../Public/Core/Math.h"

class USceneComponent : public UActorComponent
{
protected:
	FMatrix4x4 ComponentToWorld;
	FVector3 RelativeLocation = FVector3::Zero;
	FVector3 RelativeScale3D = FVector3::One;
	FQuaternion RelativeRotation = FQuaternion::Identity;
	bool ShouldRecomputeWorldTransform : true;

	FVector3 WorldLocation = FVector3::Zero;
	FQuaternion WorldRotation = FQuaternion::Identity;
	FVector3 WorldScale = FVector3::One;

	USceneComponent* AttachParent = nullptr;
	TArray<USceneComponent*> AttachChildren;
	FString AttachSocketName;

public:
	USceneComponent(AActor* _Owner);

	const FVector3& GetRelativeLocation() const { return RelativeLocation; }
	void SetRelativeLocation(const FVector3& pos) { RelativeLocation = pos; ShouldRecomputeWorldTransform = true; ComputeWorldTransform(); OnUpdateWorldTransform(); }

	const FVector3& GetRelativeScale3D() const { return RelativeScale3D; }
	void SetRelativeScale3D(const FVector3& scale) { RelativeScale3D = scale; ShouldRecomputeWorldTransform = true; ComputeWorldTransform(); OnUpdateWorldTransform(); }

	FQuaternion GetRelativeRotation() const { return RelativeRotation; }
	void SetRelativeRotation(FQuaternion rotation) { RelativeRotation = rotation; ShouldRecomputeWorldTransform = true; ComputeWorldTransform(); OnUpdateWorldTransform(); }

	bool AttachToComponent(USceneComponent* InParent,
		const FAttachmentTransformRules& AttachmentRules = FAttachmentTransformRules::KeepRelativeTransform,
		const FString& InSocketName = "");
	void DetachFromComponent(const FDetachmentTransformRules& DetachmentRules = FDetachmentTransformRules::KeepRelativeTransform);

	bool IsAttachedTo(USceneComponent* TestComp) const;

	//Only Constructor
	void SetupAttachment(USceneComponent* InParent, const FString& InSocketName = "");

	USceneComponent* GetAttachParent() const { return AttachParent; }
	const TArray<USceneComponent*>& GetAttachChildren() const { return AttachChildren; }
	const FString& GetAttachSocketName() const { return AttachSocketName; }

	//Recursively calls the function on its children
	void ComputeWorldTransform();
	const FMatrix4x4& GetComponentTransform() const { return ComponentToWorld; }

	const FVector3& GetComponentLocation() const { return WorldLocation; }
	const FQuaternion& GetComponentRotation() const { return WorldRotation; }
	const FVector3& GetComponentScale3D() const { return WorldScale; }

	FVector3 GetForwardVector() const { return FVector3::Transform(FVector3::UnitX, WorldRotation); }
	FVector3 GetRightVector() const { return FVector3::Transform(FVector3::UnitY, WorldRotation); }
};