#include "../../Public/Physics/PhysScene.h"
#include "../../Public/Collision/Collision.h"
#include "../../Public/Collision/BVH.h"
#include "../../Public/Actor/Actor.h"


const int FPhysScene::MaxPenetrationIterations = 16;
const float FPhysScene::SleepLinearVelocityThreshold = 5.0f;
const int FPhysScene::SleepCounterThreshold = 30;
const float FPhysScene::PenetrationPullBackDistance = 0.125f;
const float FPhysScene::PenetrationConvergenceThreshold = 0.0001f;
const float FPhysScene::MinPenetrationDepth = 0.125f;
const float FPhysScene::PenetrationRelaxation = 0.2f;
const float FPhysScene::MaxFixupMagnitude = 200.0f;
const int FPhysScene::MaxSlideIterations = 4;
const FVector3 FPhysScene::Gravity = FVector3(0.0f, 0.0f, -980.0f);

template<>
FSphere FPhysScene::TranslateShape<FSphere>(const FSphere& s, const FVector3& offset)
{
	FSphere s2 = s;
	s2.SetCenter(s.GetCenter() + offset);
	return s2;
}

template<>
FCapsule FPhysScene::TranslateShape<FCapsule>(const FCapsule& c, const FVector3& offset)
{
	FCapsule c2 = c;
	c2.SetCenter(c.GetCenter() + offset);
	return c2;
}

template<>
FOBB FPhysScene::TranslateShape<FOBB>(const FOBB& o, const FVector3& offset)
{
	FOBB o2 = o;
	o2.SetCenter(o.GetCenter() + offset);
	return o2;
}

FPhysScene::FPhysScene()
{
	WorldBVH = new UBVH();
}

FPhysScene::~FPhysScene()
{
	delete WorldBVH;
}

bool FPhysScene::RegisterBody(AActor* _actor)
{
	if (ActorToIndex.find(_actor) != ActorToIndex.end())
	{
		return false;
	}

	size_t index;

	if (FreeIndices.empty())
	{
		index = RegisteredBodies.size();
		RegisteredBodies.push_back({ _actor });
	}
	else
	{
		auto it = FreeIndices.begin();
		index = *it;
		FreeIndices.erase(it);
		RegisteredBodies[index].OwningActor = _actor;
	}

	return true;
}

void FPhysScene::UnregisterBody(AActor* _actor)
{
	auto it = ActorToIndex.find(_actor);
	if (it == ActorToIndex.end())
	{
		return;
	}

	size_t index = it->second;
	RegisteredBodies[index].OwningActor = nullptr;
	FreeIndices.insert(index);
	ActorToIndex.erase(it);
}

void FPhysScene::AddCollision(FCollision* InCollision)
{
	Collisions.insert(InCollision);
}

void FPhysScene::RemoveCollision(FCollision* InCollision)
{
	Collisions.erase(InCollision);
}

void FPhysScene::ForEachRegisteredBody(std::function<void(AActor*)> f)
{
	for (const FPhysicsBodyEntry& entry : RegisteredBodies)
	{
		if (entry.OwningActor != nullptr)
		{
			f(entry.OwningActor);
		}
	}
}

void FPhysScene::QueryOverlaps(AActor* mover, const FVector3& testPos, std::vector<FHitResult>& outHits, float margin)
{
	outHits.clear();

	TArray<FCollision*> mover_collisions = mover->GetCollisions();

	if (mover_collisions.empty())
	{
		return;
	}

	FVector3 offset = testPos - mover->GetActorLocation();

	for (FCollision* current_collision : mover_collisions)
	{
		switch (current_collision->GetType())
		{
		case ECollisionType::Sphere:
		{
			FSphere* mover_sphere = static_cast<FSphere*>(current_collision);
			FSphere test_sphere(mover_sphere->GetCenter() + offset, mover_sphere->GetRadius() + margin);
			ProcessOverlapCandidates(test_sphere, mover, outHits);
		}
			break;
		case ECollisionType::Capsule:
		{
			FCapsule* mover_capsule = static_cast<FCapsule*>(current_collision);
			FLineSegment mover_seg = mover_capsule->GetSegment();
			FCapsule test_capsule(mover_seg.GetStart() + offset, mover_seg.GetEnd() + offset, mover_capsule->GetRadius() + margin);
			ProcessOverlapCandidates(test_capsule, mover, outHits);
		}
			break;
		case ECollisionType::OBB:
		{
			FOBB* mover_OBB = static_cast<FOBB*>(current_collision);
			FVector3 margin_vec(margin, margin, margin);
			FOBB test_OBB(mover_OBB->GetCenter() + offset, mover_OBB->GetRotation(), mover_OBB->GetExtent() + margin_vec);
			ProcessOverlapCandidates(test_OBB, mover, outHits);
		}
			break;
		default:
			break;
		}
	}
}

template<>
inline void FPhysScene::ProcessOverlapCandidates<FSphere>(const FSphere& test_collision, AActor* mover, TArray<FHitResult>& outHits)
{
	TArray<FCollision*> candidates;

	WorldBVH->Query(test_collision.GetBoundingBox(), candidates);

	for (FCollision* candidate : candidates)
	{
		if (candidate->GetOwner() == mover)
		{
			continue;
		}

		bool hit_flag = false;

		FHitResult hit;

		switch (candidate->GetType())
		{
		case ECollisionType::Sphere:
			hit_flag = CollisionStatics::Intersect(test_collision, *static_cast<FSphere*>(candidate), hit);
			break;
		case ECollisionType::Capsule:
			hit_flag = CollisionStatics::Intersect(test_collision, *static_cast<FCapsule*>(candidate), hit);
			break;
		case ECollisionType::OBB:
			hit_flag = CollisionStatics::Intersect(test_collision, *static_cast<FSphere*>(candidate), hit);
			break;
		}
	}
}

template<>
inline void FPhysScene::ProcessOverlapCandidates<FCapsule>(const FCapsule& test_collision, AActor* mover, TArray<FHitResult>& outHits)
{
	TArray<FCollision*> candidates;

	WorldBVH->Query(test_collision.GetBoundingBox(), candidates);

	for (FCollision* candidate : candidates)
	{
		if (candidate->GetOwner() == mover)
		{
			continue;
		}

		bool hit_flag = false;

		FHitResult hit;

		switch (candidate->GetType())
		{
		case ECollisionType::Sphere:
			hit_flag = CollisionStatics::Intersect(test_collision, *static_cast<FCapsule*>(candidate), hit);
			break;
		case ECollisionType::Capsule:
			hit_flag = CollisionStatics::Intersect(test_collision, *static_cast<FCapsule*>(candidate), hit);
			break;
		case ECollisionType::OBB:
			hit_flag = CollisionStatics::Intersect(test_collision, *static_cast<FCapsule*>(candidate), hit);
			break;
		}
	}
}

template<>
inline void FPhysScene::ProcessOverlapCandidates<FOBB>(const FOBB& test_collision, AActor* mover, TArray<FHitResult>& outHits)
{
	TArray<FCollision*> candidates;

	WorldBVH->Query(test_collision.GetBoundingBox(), candidates);

	for (FCollision* candidate : candidates)
	{
		if (candidate->GetOwner() == mover)
		{
			continue;
		}

		bool hit_flag = false;

		FHitResult hit;

		switch (candidate->GetType())
		{
		case ECollisionType::Sphere:
			hit_flag = CollisionStatics::Intersect(test_collision, *static_cast<FOBB*>(candidate), hit);
			break;
		case ECollisionType::Capsule:
			hit_flag = CollisionStatics::Intersect(test_collision, *static_cast<FOBB*>(candidate), hit);
			break;
		case ECollisionType::OBB:
			hit_flag = CollisionStatics::Intersect(test_collision, *static_cast<FOBB*>(candidate), hit);
			break;
		}
	}
}

bool FPhysScene::SweepQuery(AActor* mover, const FVector3& fromPos, const FVector3& toPos, float& outT, FHitResult& outHit)
{
	TArray<FCollision*> mover_collisions = mover->GetCollisions();

	if (mover_collisions.empty())
	{
		return false;
	}

	//fromPos may differ from mover's actual (stored) position
	//(when sweeping again from a "virtual intermediate position" duringrepeated sliding). 
	//Move the shape by that offset first, then start the sweep.

	FVector3 base_offset = fromPos - mover->GetActorLocation();
	FVector3 displacement = toPos - fromPos;

	bool found = false;
	float bestT = 1.f;
	FHitResult bestHit;

	for (FCollision* current_collision : mover_collisions)
	{
		switch (current_collision->GetType())
		{
		case ECollisionType::Sphere:
		{
			FSphere translated = TranslateShape(*static_cast<FSphere*>(current_collision), base_offset);
			SweepQueryCandidates(translated, displacement, mover, bestT, bestHit, found);
		}
			break;
			case ECollisionType::Capsule:
		{
				FCapsule translated = TranslateShape(*static_cast<FCapsule*>(current_collision), base_offset);
				SweepQueryCandidates(translated, displacement, mover, bestT, bestHit, found);
		}
			break;
		case ECollisionType::OBB:
		{
			FOBB translated = TranslateShape(*static_cast<FOBB*>(current_collision), base_offset);
			SweepQueryCandidates(translated, displacement, mover, bestT, bestHit, found);
		}
			break;
		}
	}

	return false;
}

template<>
inline void FPhysScene::SweepQueryCandidates<FSphere>(const FSphere& startShape, const FVector3& displacement, AActor* mover, float& bestT, FHitResult& bestHit, bool& found)
{
}

template<>
inline void FPhysScene::SweepQueryCandidates<FCapsule>(const FCapsule& startShape, const FVector3& displacement, AActor* mover, float& bestT, FHitResult& bestHit, bool& found)
{
}

template<>
inline void FPhysScene::SweepQueryCandidates<FOBB>(const FOBB& startShape, const FVector3& displacement, AActor* mover, float& bestT, FHitResult& bestHit, bool& found)
{
}

void FPhysScene::Step(float DeltaTime)
{
}

void FPhysScene::Update()
{
}

void FPhysScene::MoveActor(AActor* _Actor, const FVector3& DesiredPos, float margin)
{
	//TOOD
}

bool FPhysScene::SegmentCast(const FLineSegment& l, FHitResult& OutResult)
{
	return WorldBVH->Raycast(l, OutResult);
}

FPhysScene* FPhysScene::Get()
{
	if (Instance == nullptr)
	{
		Instance = new FPhysScene();
	}

	return Instance;
}
