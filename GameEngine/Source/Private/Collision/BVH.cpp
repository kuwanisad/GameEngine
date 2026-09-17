#include "../../Public/Collision/BVH.h"

UBVH::UBVH()
{
}

UBVH::~UBVH()
{
	Free();
}

bool UBVH::Intersect(const FAABB& a, const FAABB& b) const
{
	return (a.GetMin().x <= b.GetMax().x && a.GetMax().x >= b.GetMin().x) &&
		(a.GetMin().y <= b.GetMax().y && a.GetMax().y >= b.GetMin().y) &&
		(a.GetMin().z <= b.GetMax().z && a.GetMax().z >= b.GetMin().z);
}

bool UBVH::Intersect(const FCapsule& C, const FAABB& box) const
{
	// AABB는 이미 축정렬이라 OBB 버전과 달리 로컬 공간 변환(회전) 자체가 필요 없음 -
	// center 기준 상대좌표로만 옮기면 그게 곧 "로컬 공간"임.
	FVector3 center = box.GetCenter();
	FVector3 ext = box.GetExtent();

	FLineSegment seg = C.GetSegment();

	FVector3 a = seg.GetStart() - center;
	FVector3 b = seg.GetEnd() - center;

	FVector3 d = b - a;
	float lenSq = d.LengthSq();

	// =========================================================
	// 캡슐 중심선과 박스 사이의 실제 최근접점 쌍(p, q) 찾기 (Capsule-OBB와 동일한
	// alternating projection - 증명도 그대로 적용됨: 둘 다 볼록도형이라 수렴 보장됨)
	// =========================================================
	FVector3 p = a;
	FVector3 q = FVector3::Zero;

	const int IterationCount = 8;

	for (int i = 0; i < IterationCount; i++)
	{
		q.x = Math::Clamp(p.x, -ext.x, ext.x);
		q.y = Math::Clamp(p.y, -ext.y, ext.y);
		q.z = Math::Clamp(p.z, -ext.z, ext.z);

		if (lenSq <= 0.0f)
		{
			// 캡슐 길이가 0인 퇴화 케이스(사실상 구): 선분 위의 점은 항상 a로 고정
			break;
		}

		float t = Math::Clamp(FVector3::Dot(q - a, d) / lenSq, 0.0f, 1.0f);
		p = a + d * t;
	}

	FVector3 diff = p - q;
	float distSq = diff.LengthSq();

	float r = C.GetRadius();

	return distSq > r * r;
}

bool UBVH::Intersect(const FLineSegment& l, const FAABB& b) const
{
	FVector3 l_start = l.GetStart();
	FVector3 l_end = l.GetEnd();
	FVector3 l_direction = l_end - l_start;

	FVector3 b_min = b.GetMin();
	FVector3 b_max = b.GetMax();

	float min_t = 0.f;
	float max_t = 1.f;

	int32 min_axis = -1;
	float min_axis_t = 0.f;

	for (int32 i = 0; i < 3; ++i)
	{
		// The segment is parallel to this axis.
		if (Math::NearZero(l_direction[i]))
		{
			// If the entire segment lies outside the AABB
			// along this axis, it cannot intersect the AABB.
			if (l_start[i] < b_min[i] || l_start[i] > b_max[i])
			{
				return false;
			}

			// Since there is no movement along this axis,
			// this axis does not affect the t range.
			continue;
		}

		// s + d * t = v
		// t = (v - s) / d
		// Calculate the t values where the segment intersects
		// the two AABB planes along this axis.
		const float inv_direction = 1.f / l_direction[i];

		float t1 = (b_min[i] - l_start[i]) * inv_direction;
		float t2 = (b_max[i] - l_start[i]) * inv_direction;

		// The order of t1 and t2 depends on the direction,
		// so make sure they are sorted.
		const float axis_min_t = Math::Min(t1, t2);
		const float axis_max_t = Math::Max(t1, t2);

		// Intersect the current t range with the t range
		// of the current axis.
		if (axis_min_t > min_t)
		{
			min_t = axis_min_t;
			min_axis = i;
			min_axis_t = axis_min_t;
		}

		max_t = Math::Min(max_t, axis_max_t);

		// The intersection of the t ranges is empty.
		if (min_t > max_t)
		{
			return false;
		}
	}

	// The segment intersects the AABB along all three axes.
	// If min_t is less than 0, the segment starts inside the AABB.
	if (min_t < 0.f)
	{
		min_t = 0.f;
	}

	// The intersection lies outside the segment.
	if (min_t > 1.f)
	{
		return false;
	}

	return true;
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

	if (Intersect(node->box, box) == false)
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

void UBVH::QueryCapsule(const FCapsule& capsule, TArray<FCollision*>& out) const
{
	QueryCapsule_Internal(root, capsule, out);
}

void UBVH::QueryCapsule_Internal(FBVHNode* node, const FCapsule& capsule, TArray<FCollision*>& out) const
{
	if (node == nullptr)
	{
		return;
	}

	if (Intersect(capsule, node->box) == false)
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

	if (Intersect(segment, node->box) == false)
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
