#pragma once
#include "../Core/Math.h"

struct FCollision;

//UE의 FHitResult에 대응
struct FHitResult
{
	FVector3 ImpactPoint;
	FVector3 Normal;
	bool BlockingHit;
	FCollision* CollidedTarget = nullptr;

	// 이만큼 파고들었다면, normal 방향으로 이 값만큼 밀어내야 함.
	float PenetrationDepth = 0.0f;
};