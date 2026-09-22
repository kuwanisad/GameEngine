#pragma once
#include "../Core/Math.h"
#include "Collision.h"

#include <vector>
#include <algorithm>
#include <cfloat>

struct FBVHNode
{
	FAABB box;

	FBVHNode* left = nullptr;
	FBVHNode* right = nullptr;

	FCollision* object = nullptr;

	bool IsLeaf() const { return object != nullptr; }
};

class UBVH
{
protected:
	FBVHNode* root = nullptr;

	TArray<FCollision*> Collisions;
	bool IntersectSegmentWithCollision(const FLineSegment& l, FCollision* target, FHitResult& outHit) const;

	void Free();
	void FreeNode(FBVHNode* node);
	void Query_Internal(FBVHNode* node, const FAABB& box, TArray<FCollision*>& out) const;
	void QueryCapsule_Internal(FBVHNode* node, const FCapsule& capsule, TArray<FCollision*>& out) const;
	void QuerySphere_Internal(FBVHNode* node, const FSphere& Sphere, TArray<FCollision*>& out) const;
	void QueryOBB_Internal(FBVHNode* node, const FOBB& OBB, TArray<FCollision*>& out) const;
	void Raycast_Internal(FBVHNode* node, const FLineSegment& segment, FHitResult& OutResult, float& bestDistSq, bool& found) const;

	static FAABB Union(const FAABB& a, const FAABB& b);
	FAABB ComputeBounds(const TArray<FCollision*>& objs);

public:
	UBVH();
	~UBVH();

	void Build(TArray<FCollision*> objs);
	FBVHNode* BuildTree(TArray<FCollision*> objs);
	void Query(const FAABB& box, TArray<FCollision*>& out) const;
	void QueryCapsule(const FCapsule& shape, TArray<FCollision*>& out) const;
	void QuerySphere(const FSphere& shape, TArray<FCollision*>& out) const;
	void QueryOBB(const FOBB& shape, TArray<FCollision*>& out) const;
	bool Raycast(const FLineSegment& segment, FHitResult& OutResult) const;

	//오브젝트 목록을 갱신하고 트리를 다시 빌드
	void AddCollision(FCollision* InCollision);
	void RemoveCollision(FCollision* InCollision);
};