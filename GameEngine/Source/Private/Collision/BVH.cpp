#include "../../Public/Collision/BVH.h"

UBVH::UBVH()
{
}

UBVH::~UBVH()
{
	Free();
}

bool UBVH::IntersectSegmentWithCollision(const FLineSegment& l, FCollision* target, FHitResult& outHit) const
{
	bool hit_flag = false;
	outHit.BlockingHit = false;

	if (target == nullptr)
	{
		return false;
	}

	switch (target->GetType())
	{
	case ECollisionType::Sphere:
		hit_flag = CollisionStatics::Intersect(l, (const FSphere&)(*target), outHit);
		break;
	case ECollisionType::OBB:
		hit_flag = CollisionStatics::Intersect(l, (const FOBB&)(*target), outHit);
		break;
	case ECollisionType::Capsule:
		hit_flag = CollisionStatics::Intersect(l, (const FCapsule&)(*target), outHit);
		break;
	case ECollisionType::LineSegment:
	{
		FVector3 p1, p2;
		float distSq = Math::NearZero(FLineSegment::MinDistSq(l, (const FLineSegment&)(*target), p1, p2));

		if (Math::NearZero(distSq))
		{
			outHit.BlockingHit = true;
			outHit.ImpactPoint = p1;

			//Lines don't have surfaces...
			outHit.Normal = FVector3::Zero;
			hit_flag = true;
		}
	}
		break;
	}

	outHit.CollidedTarget = hit_flag ? target : nullptr;

	return hit_flag;
}

void UBVH::Free()
{
	if (root != nullptr)
	{
		FreeNode(root->left);
		FreeNode(root->right);
		delete root;
		root = nullptr;
	}
}

void UBVH::FreeNode(FBVHNode* node)
{
	if (node == nullptr)
	{
		return;
	}

	FreeNode(node->left);
	FreeNode(node->right);
	delete node;
}

FAABB UBVH::ComputeBounds(const TArray<FCollision*>& objs)
{
	if (objs.empty())
	{
		return FAABB();
	}

	FAABB box = objs.front()->GetBoundingBox();

	for (size_t i = 1; i < objs.size(); ++i)
	{
		box = Union(box, objs[i]->GetBoundingBox());
	}

	return box;
}

FAABB UBVH::Union(const FAABB& a, const FAABB& b)
{
	FVector3 minVec(
		Math::Min(a.GetMin().x, b.GetMin().x),
		Math::Min(a.GetMin().y, b.GetMin().y),
		Math::Min(a.GetMin().z, b.GetMin().z));

	FVector3 maxVec(
		Math::Max(a.GetMax().x, b.GetMax().x),
		Math::Max(a.GetMax().y, b.GetMax().y),
		Math::Max(a.GetMax().z, b.GetMax().z));

	return FAABB(minVec, maxVec);
}

void UBVH::Build(TArray<FCollision*> objs)
{
	Free();

	Collisions = objs;

	if (objs.empty())
	{
		return;
	}

	root = BuildTree(objs);
}

FBVHNode* UBVH::BuildTree(TArray<FCollision*> objs)
{
	if (objs.empty())
	{
		return nullptr;
	}

	FBVHNode* node = new FBVHNode();
	node->box = ComputeBounds(objs);

	if (objs.size() <= 1)
	{
		node->object = objs.front();
		return node;
	}

	// =========================
	// SAH (Surface Area Heuristic) Search
	//
	// For each of the three axes (x, y, z), sort the objects and evaluate
	// the cost at every possible split position.
	// By precomputing the prefix AABBs, where prefix[i] is the AABB enclosing
	// sorted objs[0..i], and the suffix AABBs, where suffix[i] is the AABB
	// enclosing sorted objs[i..n-1], we can retrieve the left and right AABBs
	// of each split point in O(1) instead of recomputing them from scratch.
	// =========================
	float bestCost = FLT_MAX;
	int32 bestAxis = 0;

	const size_t n = objs.size();
	int32 bestIndex = (int32)n >> 1;

	TArray<FAABB> leftBound(n);
	TArray<FAABB> rightBound(n);

	for (int32 axis = 0; axis < 3; ++axis)
	{
		std::sort(objs.begin(), objs.end(), 
			[axis](FCollision* a, FCollision* b)
			{
				return a->GetCenter()[axis] < b->GetCenter()[axis];
			});

		leftBound[0] = objs[0]->GetBoundingBox();
		rightBound[n - 1] = objs[n - 1]->GetBoundingBox();

		for (size_t i = 1; i < n; ++i)
		{
			leftBound[i] = Union(leftBound[i - 1], objs[i]->GetBoundingBox());
			rightBound[n - 1 - i] = Union(rightBound[n - i], objs[n - 1 - i]->GetBoundingBox());
		}

		//Split point i: left = objs[0..i-1] (count i), right = objs[i..n-1] (count n-i)
		//if n = 6, i = 3
		//0, 1, 2, 3(i), 4, 5
		//frontBound -> 0~2 -> i - 1
		//backBound -> 3~5 -> i
		for (size_t i = 1; i < n; ++i)
		{
			float leftSA = leftBound[i - 1].GetSurfaceArea();
			float rightSA = rightBound[i].GetSurfaceArea();

			/*
			SAH cost calculation.
			The parent node's surface area is omitted because
			we only need to compare relative costs.
			*/
			float cost = leftSA * (float)i + rightSA * (float)(n - i);

			if (cost < bestCost)
			{
				bestCost = cost;
				bestAxis = axis;
				bestIndex = (int32)i;
			}
		}
	}

	//Re-sort based on the optimal axis, then split.
	std::sort(objs.begin(), objs.end(),
		[bestAxis](FCollision* a, FCollision* b)
		{
			return a->GetCenter()[bestAxis] < b->GetCenter()[bestAxis];
		});

	TArray<FCollision*> nextLeft(objs.begin(), objs.begin() + bestIndex);
	TArray<FCollision*> nextRight(objs.begin() + bestIndex, objs.end());

	node->left = BuildTree(std::move(nextLeft));
	node->right = BuildTree(std::move(nextRight));

	return node;
}

void UBVH::Query(const FAABB& box, TArray<FCollision*>& out) const
{
	Query_Internal(root, box, out);
}

void UBVH::Query_Internal(FBVHNode* node, const FAABB& box, TArray<FCollision*>& out) const
{
	if (node == nullptr)
	{
		return;
	}

	FHitResult hit;

	if (CollisionStatics::Intersect(node->box, box, hit) == false)
	{
		return;
	}

	if (node->IsLeaf())
	{
		out.push_back(node->object);
		return;
	}

	Query_Internal(node->left, box, out);
	Query_Internal(node->right, box, out);
}

void UBVH::QueryCapsule(const FCapsule& shape, TArray<FCollision*>& out) const
{
	QueryCapsule_Internal(root, shape, out);
}

void UBVH::QuerySphere(const FSphere& shape, TArray<FCollision*>& out) const
{
	QuerySphere_Internal(root, shape, out);
}

void UBVH::QueryOBB(const FOBB& shape, TArray<FCollision*>& out) const
{
	QueryOBB_Internal(root, shape, out);
}

void UBVH::QueryCapsule_Internal(FBVHNode* node, const FCapsule& capsule, TArray<FCollision*>& out) const
{
	if (node == nullptr)
	{
		return;
	}

	FHitResult hit;

	if (CollisionStatics::Intersect(capsule, node->box, hit) == false)
	{
		return;
	}

	if (node->IsLeaf())
	{
		out.push_back(node->object);
		return;
	}

	QueryCapsule_Internal(node->left, capsule, out);
	QueryCapsule_Internal(node->right, capsule, out);
}

void UBVH::QuerySphere_Internal(FBVHNode* node, const FSphere& sphere, TArray<FCollision*>& out) const
{
	if (node == nullptr)
	{
		return;
	}

	FHitResult hit;

	if (CollisionStatics::Intersect(sphere, node->box, hit) == false)
	{
		return;
	}

	if (node->IsLeaf())
	{
		out.push_back(node->object);
		return;
	}

	QuerySphere_Internal(node->left, sphere, out);
	QuerySphere_Internal(node->right, sphere, out);
}

void UBVH::QueryOBB_Internal(FBVHNode* node, const FOBB& OBB, TArray<FCollision*>& out) const
{
	if (node == nullptr)
	{
		return;
	}

	FHitResult hit;

	if (CollisionStatics::Intersect(OBB, node->box, hit) == false)
	{
		return;
	}

	if (node->IsLeaf())
	{
		out.push_back(node->object);
		return;
	}

	QueryOBB_Internal(node->left, OBB, out);
	QueryOBB_Internal(node->right, OBB, out);
}

bool UBVH::Raycast(const FLineSegment& segment, FHitResult& OutResult) const
{
	OutResult.BlockingHit = false;

	float bestDisSq = Math::Infinity;
	bool result = false;

	Raycast_Internal(root, segment, OutResult, bestDisSq, result);

	return result;
}

void UBVH::Raycast_Internal(FBVHNode* node, const FLineSegment& segment, FHitResult& OutResult, float& bestDistSq, bool& found) const
{
	if (node == nullptr)
	{
		return;
	}
	FHitResult hit;

	if (CollisionStatics::Intersect(segment, node->box, hit) == false)
	{
		return;
	}

	if (node->IsLeaf())
	{
		FHitResult hit;
		if (IntersectSegmentWithCollision(segment, node->object, hit))
		{
			float distSq = (hit.ImpactPoint - segment.GetStart()).LengthSq();

			if (distSq < bestDistSq)
			{
				bestDistSq = distSq;
				OutResult = hit;
				found = this;
			}
			return;
		}
	}

	Raycast_Internal(node->left, segment, OutResult, bestDistSq, found);
	Raycast_Internal(node->right, segment, OutResult, bestDistSq, found);
}

void UBVH::AddCollision(FCollision* InCollision)
{
	Collisions.push_back(InCollision);
	Build(Collisions);
}

void UBVH::RemoveCollision(FCollision* InCollision)
{
	for (int32 i = Collisions.size() - 1; i >= 0; --i)
	{
		if (Collisions[i] == InCollision)
		{
			Collisions.erase(Collisions.begin() + i);
			break;
		}
	}

	Build(Collisions);
}
