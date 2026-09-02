#pragma once
#include "../Core/Math.h"

class FCollision;

//UE FHitResult
struct FHitResult
{
	FVector3 ImpactPoint;
	FVector3 Normal;
	bool BlockingHit;
	FCollision* CollidedTarget = nullptr;

	// If it penetrates this far, it should protrude by this amount in the normal direction.
	float PenetrationDepth = 0.0f;
};