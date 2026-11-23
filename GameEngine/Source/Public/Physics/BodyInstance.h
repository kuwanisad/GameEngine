#pragma once
#include "../Core/CoreMinimal.h"
#include "../Core/Math.h"
#include "PhysicalMaterial.h"

/*
UE FBodyInstance
All UPrimitiveComponent instances have one.
This struct handles mass, gravity, velocity, ...
*/
struct FBodyInstance
{
	bool SimulatePhysics = false;
	bool EnableGravity = true;

	/*For preventing vibration due to extremely low acceleration*/
	bool IsSleeping = false;
	int32 SleepCounter = 0;

	void WakeUp()
	{
		IsSleeping = false;
		SleepCounter = 0;
	}

	float MassInKg = 1.0f;

	UPhysicalMaterial PhysMaterial;

	FVector3 LinearVelocity = FVector3::Zero;

	//이번 프레임 동안 누적된 힘.
	FVector3 AccumulatedForce = FVector3::Zero;

	float GetMass() const { return MassInKg; }

	FVector3 GetPhysicsLinearVelocity() const { return LinearVelocity; }
	void SetPhysicsLinearVelocity(const FVector3& NewVel, bool AddToCurrent)
	{
		LinearVelocity = AddToCurrent ? (LinearVelocity + NewVel) : NewVel;
	}

	/*Add force during this frame.*/
	void AddForce(const FVector3& Force)
	{
		if (SimulatePhysics)
		{
			AccumulatedForce += Force;
		}
	}


	void AddImpulse(const FVector3& Impulse)
	{
		if (SimulatePhysics && MassInKg > 0.0f)
		{
			if (!Math::NearZero(Impulse.LengthSq()))
			{
				WakeUp();
			}
			LinearVelocity += Impulse / MassInKg;
		}
	}
};