#pragma once
#include "../Core/CoreMinimal.h"
#include "../Core/Math.h"

class AActor;

struct FPhysScene
{
private:
	static FPhysScene* Instance;

	TArray<AActor*> Actors;
	TArray<AActor*> PendingActors;

	bool IsUpdatingActors = false;

public:
	FPhysScene();
	~FPhysScene();

	bool RegisterBody(AActor* _actor);
	void UnregisterBody(AActor* _actor);

	void MoveActor(AActor* _Actor, const FVector3& DesiredPos, float margin);


	static FPhysScene* Get();

public:
	//Investigation of variables was aided by Claude

	// Tuning constants used for iterative depenetration. Although they are used
	// by the code that actually performs movement, such as Game::MoveActor,
	// the policy for "how much clearance to allow / how many iterations to run"
	// belongs to FPhysScene (the physics domain), so they are kept here.
	// (Similar in purpose to values such as
	// UE's MovementComponentCVars::PenetrationPullbackDistance.)
	static const int MaxPenetrationIterations;

	// [Sleep] UE also has SleepLinearVelocityThreshold /
	// SleepCounterThreshold in PhysicalMaterial (confirmed in the official docs).
	// If the velocity remains below this threshold for SleepCounterThreshold
	// consecutive steps, the object is forcibly brought to a complete stop
	// (velocity = 0) and put to sleep. Without this, due to a fundamental
	// limitation of our system—contacts are approximated using only a few
	// points—an object that has come to rest at an angle may never naturally
	// settle flat, causing tiny residual drift to continue indefinitely.
	// Sleep forcibly cuts off this residual drift.
	static const float SleepLinearVelocityThreshold;
	static const int SleepCounterThreshold;

	static const float PenetrationPullBackDistance;
	static const float PenetrationConvergenceThreshold;
	// Minimum value to use when FHitResult.PenetrationDepth is less than or
	// equal to zero due to floating-point errors or similar issues.
	static const float MinPenetrationDepth;
	// [Relaxation factor] During iterative fixup accumulation, only this fraction
	// of the error requested by the current contact is applied instead of the
	// full error (100%). When multiple contacts request movement in the same
	// direction (a common case, e.g. multiple flat floor tiles), this only slows
	// convergence slightly and the result still converges to the same solution.
	// However, when two contacts request movement in exactly opposite directions
	// (e.g. an object wedged between the floor and another object), applying the
	// full correction can cause the contacts to push against each other on every
	// iteration, making the fixup grow without bound and potentially diverge.
	// The relaxation factor reduces the response magnitude on each iteration,
	// preventing this divergence and allowing even geometrically impossible
	// situations to converge to a finite value (or at least preventing the value
	// from growing indefinitely).
	static const float PenetrationRelaxation;
	// [Safety limit] As a final safeguard against extreme cases that cannot be
	// prevented by the relaxation factor alone (e.g. failure to converge within
	// the iteration limit), the magnitude of the final fixup is clamped to this
	// value. The goal is to leave a small amount of unresolved penetration if
	// necessary, while ensuring that the correction can never blow up.
	static const float MaxFixupMagnitude;
	// Upper bound on the number of iterations for CCD sliding (velocity clipping).
	// A single wall usually takes only 1–2 iterations, but extra iterations are
	// allowed for cases where multiple walls block movement in succession, such
	// as corners. (Following the convention of Quake's ClipVelocity.)
	static const int MaxSlideIterations;

	// Gravitational acceleration, assuming a cm/s^2 scale.
	// Matches UE's default world gravity value of -980.
	// The integrator adds this value as a force/acceleration contribution each
	// step for FBodyInstance objects with bEnableGravity enabled.
	static const FVector3 Gravity;
};