#pragma once
#include "ActorComponent.h"
#include "../Core/Math.h"

class AActor;

class UMovementComponent : public UActorComponent
{
public:
	UMovementComponent(AActor* _Owner);
	void Tick(float DeltaTime) override;

	float GetAngularSpeed() const { return AngularSpeed; }
	float GetForwardSpeed() const { return ForwardSpeed; }
	float GetStrafeSpeed() const { return StrafeSpeed; }
	void SetAngularSpeed(float speed) { AngularSpeed = speed; }
	void SetForwardSpeed(float speed) { ForwardSpeed = speed; }
	void SetStrafeSpeed(float speed) { StrafeSpeed = speed; }

	bool GetUseCCD() const { return bUseCCD; }
	void SetUseCCD(bool value) { bUseCCD = value; }

	void AddForce(const FVector3& Force) { AccumulatedForce += Force; }
	void AddImpulse(const FVector3& Impulse) { Velocity += Impulse; }

	void Launch(const FVector3& LaunchVelocity) { PendingLaunchVelocity = LaunchVelocity; }

	FVector3 GetVelocity() const { return Velocity; }

	bool IsOnGround() const { return bIsOnGround; }

private:
	float AngularSpeed;
	float ForwardSpeed;
	float StrafeSpeed;
	bool bUseCCD = false;

	FVector3 Velocity = FVector3::Zero;
	FVector3 AccumulatedForce = FVector3::Zero;
	FVector3 PendingLaunchVelocity = FVector3::Zero;

	bool bIsOnGround = false;

	bool bEnableGravity = true;
};
