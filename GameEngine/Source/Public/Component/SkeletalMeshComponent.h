#pragma once
#include "PrimitiveComponent.h"

class UAnimInstance;

class USkeletalMeshComponent : public UPrimitiveComponent
{
protected:
	UAnimInstance* AnimInstance;
	bool Visible;

public:
	USkeletalMeshComponent(AActor* _Owner);
	virtual ~USkeletalMeshComponent();

	//완전히 소유권이 이전되고 책임져야 함
	void SetAnimInstance(UAnimInstance* InAnimInstance);
	UAnimInstance* GetAnimInstance() const { return AnimInstance; }

	void SetVisible(bool _Visible) { Visible = _Visible; }
	bool GetVisible() const { return Visible; }

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnDestroy() override;
};