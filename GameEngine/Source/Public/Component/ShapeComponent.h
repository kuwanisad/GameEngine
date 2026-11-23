#pragma once
#include "PrimitiveComponent.h"

struct FCollision;

//UE와 달리 이 컴포넌트는 콜리전만 가짐.
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