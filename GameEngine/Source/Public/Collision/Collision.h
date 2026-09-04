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

class Plane
{
protected:
	FVector3 Normal;
	float D;

public:
	Plane(const FVector3& _Normal, float _D);
	Plane(const FVector3& a, const FVector3& b, const FVector3& c);
	float SignedDist(const FVector3& point) const;

	FVector3 GetNormal() const;
};

class FSphere : public FCollision
{
protected:
	FVector3 Center;
	float Radius;

public:
	FSphere(const FVector3& _center, float _radius);
	bool Contains(const FVector3& _point) const;
	FAABB GetBoundingBox() const override;
	FVector3 GetCenter() const override;
	FVector3 Support(const FVector3& Direction) const override;
	float GetRadius() const;
};

class FAABB
{
protected:
	FVector3  Center;
	FVector3  Extent; // 중심에서 각 축 방향으로의 반너비(half-extent). 항상 0 이상이어야 함.

public:
	FAABB();
	FAABB(const FVector3& _Min, const FVector3& _Max);

	//Contains the vector.
	void UpdateMinMax(const FVector3& _point);
	void Rotate(const FQuaternion& q);
	bool Contains(const FVector3& _point) const;
	float MinDistSq(const FVector3& point) const;

	float GetSurfaceArea() const;

	//based on the local coordinate system
	FVector3  GetMin() const;
	//based on the local coordinate system
	FVector3  GetMax() const;
	//based on the local coordinate system
	void SetMin(const FVector3& Value);
	//based on the local coordinate system
	void SetMax(const FVector3& Value);
	FVector3  GetCenter() const;
	FVector3  GetExtent() const;
};