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

struct FBVH
{
protected:
	FBVHNode* root = nullptr;

	TArray<FCollision*> Collisions;

	static bool Intersect(const FAABB& a, const FAABB& b);
	static bool Intersect(const FCapsule& C, const FAABB& box);

	void Free();
	void FreeNode(FBVHNode* node);
	static FAABB ComputeBounds(const TArray<FCollision*>& objs);
	void Query_Internal(FBVHNode* node, const FAABB& box, TArray<FCollision*>& out) const;
	void QueryCapsule_Internal(FBVHNode* node, const FCapsule& capsule, TArray<FCollision*>& out) const;
	void Raycast_Internal(FBVHNode* node, const FLineSegment& segment, FHitResult& OutResult, float& bestDistSq, bool& found) const;

	static FAABB Union(const FAABB& a, const FAABB& b);

public:
	FBVH();
	~FBVH();

	void Build(TArray<FCollision*> objs);
	FBVHNode* BuildTree(TArray<FCollision*> objs);
	void Query(const FAABB& box, TArray<FCollision*>& out) const;
	void QueryCapsule(const FCapsule& shape, TArray<FCollision*>& out) const;
	bool Raycast(const FLineSegment& segment, FHitResult& OutResult) const;

	//오브젝트 목록을 갱신하고 트리를 다시 빌드
	void AddCollision(FCollision* InCollision);
	void RemoveCollision(FCollision* InCollision);
};