#pragma once
//Referenced Game Programming in C++

#include "../Core/Math.h"
#include "../Core/CoreMinimal.h"
#include "HitResult.h"

class AActor;

enum class ECollisionType
{
	None,
	Sphere,
	OBB,
	LineSegment,
	Capsule,
	ConvexPolygon,
};

struct FAABB
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

struct FCollision
{
protected:
	ECollisionType Type;
	AActor* Owner;

public:
	FCollision();
	virtual ~FCollision();
	void SetOwner(AActor* InOwner);
	AActor* GetOwner() const;
	ECollisionType GetType() const { return Type; }
	virtual FAABB GetBoundingBox() const = 0;

	//Global coordinate system
	virtual FVector3 GetCenter() const = 0;
	virtual void SetCenter(const FVector3& InCenter) = 0;

	/*
	GJK support function
	Return the point within this shape that is farthest in Direction.
	*/
	virtual FVector3 Support(const FVector3& Direction) const = 0;
};

struct FLineSegment : public FCollision
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
	void SetCenter(const FVector3& InCenter) override;
	FVector3 Support(const FVector3& Direction) const override;

	FVector3 GetStart() const;
	FVector3 GetEnd() const;

};

struct FPlane
{
protected:
	FVector3 Normal;
	float D;

public:
	FPlane(const FVector3& _Normal, float _D);
	FPlane(const FVector3& a, const FVector3& b, const FVector3& c);
	float SignedDist(const FVector3& point) const;

	FVector3 GetNormal() const;
	float GetD() const;
};

struct FSphere : public FCollision
{
protected:
	FVector3 Center;
	float Radius;

public:
	FSphere(const FVector3& _center, float _radius);
	bool Contains(const FVector3& _point) const;
	FAABB GetBoundingBox() const override;
	FVector3 GetCenter() const override;
	void SetCenter(const FVector3& InCenter) override;
	FVector3 Support(const FVector3& Direction) const override;
	float GetRadius() const;
};

struct FCapsule : public FCollision
{
protected:
	FLineSegment Segment;
	float Radius;

public:
	FCapsule(const FVector3& _Start, const FVector3& _End, float _Radius);
	~FCapsule();
	FVector3 PointOnSegment(float t);
	bool Contains(const FVector3& point);
	FLineSegment GetSegment() const;
	float GetRadius() const;
	FAABB GetBoundingBox() const override;
	FVector3 GetCenter() const override;
	void SetCenter(const FVector3& InCenter) override;
	FVector3 Support(const FVector3& Direction) const override;
};

struct FOBB : public FCollision
{
protected:
	FVector3 Center;
	FQuaternion Rotation;
	FVector3 Extents; //Local

public:
	FOBB(const FVector3& _Center, const FQuaternion& _Rotation, const FVector3& _Extents);
	~FOBB();

	FAABB GetBoundingBox() const override;
	FVector3 GetCenter() const override;
	void SetCenter(const FVector3& InCenter) override;
	FQuaternion GetRotation() const;
	void Rotate(const FQuaternion& InRotation);
	FVector3 GetExtent() const;
	FVector3 Support(const FVector3& Direction) const override;

	void SetExtent(const FVector3& InExtent);
	void SetRotation(const FQuaternion& InRotation);
};

struct CollisionStatics
{
private:
	static const int32 IterationCount;

public:
	//NOTE!!!
	//The first parameter is the caller!
	//Every normal lies on the second parameter's surface and is oriented toward the first parameter!
	//Do not "normal = second - first", use GetNormalDirection.
	static FVector3 GetNormalDirection(const FVector3& Caller, const FVector3& Target);

	static bool Intersect(const FAABB& a, const FAABB& b, FHitResult& OutResult);

	static bool Intersect(const FOBB& O1, const FOBB& O2, FHitResult& OutResult);

	static bool Intersect(const FCapsule& a, const FCapsule& b, FHitResult& OutResult);
	static bool Intersect(const FCapsule& c, const FOBB& o, FHitResult& OutResult);
	static bool Intersect(const FCapsule& c, const FAABB& box, FHitResult& OutResult);

	static bool Intersect(const FSphere& s, const FAABB& box, FHitResult& OutResult);
	static bool Intersect(const FSphere& a, const FSphere& b, FHitResult& OutResult);
	static bool Intersect(const FSphere& s, const FOBB& o, FHitResult& OutResult);
	static bool Intersect(const FSphere& s, const FCapsule& c, FHitResult& OutResult);

	static bool Intersect(const FLineSegment& l, const FSphere& s, FHitResult& OutResult);
	static bool Intersect(const FLineSegment& l, const FPlane& p, FHitResult& OutResult);
	static bool Intersect(const FLineSegment& l, const FAABB& b, FHitResult& OutResult);
	static bool Intersect(const FLineSegment& l, const FCapsule& C, FHitResult& OutResult);
	static bool Intersect(const FLineSegment& l, const FOBB& O, FHitResult& OutResult);
};