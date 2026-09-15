#pragma once
#include "PrimitiveComponent.h"

struct FCollision;

//unlike UE, this component has only collision.
class UShapeComponent : public UPrimitiveComponent
{
protected:
	FCollision* _Collision;

	void SetCollision(FCollision* InCollision);

public:
	UShapeComponent(AActor* _Owner);
	virtual ~UShapeComponent();

	FCollision* GetCollision() const;
};