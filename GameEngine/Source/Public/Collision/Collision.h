#pragma once
//Referenced Game Programming in C++

#include "../Core/Math.h"
#include "../Core/CoreMinimal.h"
#include "HitResult.h"

class FAABB;

enum class ECollisionType
{
	None,
	Sphere,
	Box,
	OBB,
	LineSegment,
	Capsule,
	ConvexPolygon,
};

class FCollision
{
protected:
	ECollisionType Type;

public:
	FCollision();
	virtual ~FCollision();
	ECollisionType GetType() const { return Type; }
	virtual FAABB GetBoundingBox() const = 0;
	virtual FVector3 GetCenter() const = 0;

	/*
	GJK support function
	Return the point within this shape that is farthest in Direction.
	*/
	virtual FVector3 Support(const FVector3& Direction) const = 0;
};

class FLineSegment : public FCollision
{
protected:
	FVector3 Start;
	FVector3 End;

public:
	FLineSegment(const FVector3& _Start, const FVector3 _End);
	FVector3 PointOnSegment(float t) const;
	float MinDistSq(const FVector3& _Point);
	static float MinDistSq(const FLineSegment& S1, const FLineSegment& S2);
	static float MinDistSq(const FLineSegment& S1, const FLineSegment& S2, FVector3& outPointOnS1, FVector3& outPointOnS2);
	FAABB GetBoundingBox() const override;
	FVector3 GetCenter() const override;
	FVector3 Support(const FVector3& Direction) const override;

	FVector3 GetStart() const;
	FVector3 GetEnd() const;
};