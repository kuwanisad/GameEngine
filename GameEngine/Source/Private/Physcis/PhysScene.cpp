#include "../../Public/Physics/PhysScene.h"
#include "../../Public/Collision/Collision.h"
#include "../../Public/Collision/BVH.h"
#include "../../Public/Collision/GJKStatics.h"
#include "../../Public/Actor/Actor.h"
#include "../../Public/Component/PrimitiveComponent.h"
#include "../../Public/Component/ShapeComponent.h"
#include "../../Public/Core/Cast.h"


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

FSphere FPhysScene::TranslateShape(const FSphere& s, const FVector3& offset)
{
	FSphere s2 = s;
	s2.SetCenter(s.GetCenter() + offset);
	return s2;
}

FCapsule FPhysScene::TranslateShape(const FCapsule& c, const FVector3& offset)
{
	FCapsule c2 = c;
	c2.SetCenter(c.GetCenter() + offset);
	return c2;
}

FOBB FPhysScene::TranslateShape(const FOBB& o, const FVector3& offset)
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

void FPhysScene::Step(float DeltaTime)
{
	//각 컴포넌트가 물리 영향을 받을 때 부모가 이동 시 자식도 이동하지만 자식이 충돌 시 부모가 멈추지 않기 때문에
	//부모의 이동에 자식이 이동되는 경우 좌표계 때문이고 부모 자식이 물리 작용은 각각 적용된다고 봐야 함

	ForEachRegisteredBody([&](AActor* actor)
		{
			if (actor->GetPrimitiveComponents().empty())
			{
				return;
			}

			for (auto prim : actor->GetPrimitiveComponents())
			{
				if (prim->BodyInstance.SimulatePhysics)
				{
					FBodyInstance& body = prim->BodyInstance;
					
					if (body.IsSleeping)
					{
						break;
					}

					FVector3 force = body.AccumulatedForce;

					if (body.MassInKg)
					{
						if (body.EnableGravity)
						{
							force += FPhysScene::Gravity * body.MassInKg;
						}

						//f = ma, f / m = a
						FVector3 acceleration = force / body.MassInKg;
						body.LinearVelocity += acceleration * DeltaTime;
					}

					FVector3 old_pos = prim->GetComponentLocation();
					FVector3 predicted_velocity = body.LinearVelocity;
					FVector3 desired_pos = old_pos + predicted_velocity * DeltaTime;

					UShapeComponent* shape_comp = Cast<UShapeComponent>(prim);

					if (shape_comp != nullptr && shape_comp->GetCollision())
					{
						FVector3 movement = desired_pos - old_pos;

						if (Math::NearZero(movement.LengthSq()) == false)
						{
							float t = 1.f;
							FHitResult ccd_result;
							bool is_blocked = SweepQuery(actor, old_pos, desired_pos, t, ccd_result);

							desired_pos = old_pos + movement * t;
							FVector3 desired_pos_delta = desired_pos - old_pos;

							FVector3 desired_local_pos = desired_pos - shape_comp->GetComponentLocation();
							FMatrix4x4 shape_comp_world_mat_inv = shape_comp->GetComponentTransform();
							shape_comp_world_mat_inv.Invert();

							FVector3 desired_pos_delta_local = FVector3::Transform(desired_pos_delta, shape_comp_world_mat_inv);

							shape_comp->SetRelativeLocation(desired_pos_delta_local);

							if (Math::NearZero(DeltaTime) == false)
							{
								FVector3 corrected_velocity = (actor->GetActorLocation() - old_pos) / DeltaTime;
								FVector3 velocity_removed = predicted_velocity - corrected_velocity;
								body.LinearVelocity = corrected_velocity - velocity_removed * body.PhysMaterial.Restitution;
							}

							if (is_blocked && body.LinearVelocity.LengthSq() < FPhysScene::SleepLinearVelocityThreshold * FPhysScene::SleepLinearVelocityThreshold)
							{
								++body.SleepCounter;
								if (body.SleepCounter >= FPhysScene::SleepCounterThreshold)
								{
									body.IsSleeping = true;
									body.LinearVelocity = FVector3::Zero;
								}
								else
								{
									body.SleepCounter;
								}
							}
						}
					}
				}
			}
		});
}

void FPhysScene::Update()
{
	TArray<FCollision*> objs(Collisions.begin(), Collisions.end());
	WorldBVH->Build(objs);
}

void FPhysScene::MoveActor(AActor* _Actor, const FVector3& DesiredPos, float margin)
{
	//이동했는데 해당 위치가 다른 액터들 사이에 낀 경우 여러 번 조정해도 제대로 된 위치로 옮겨진다는 보장이 없어
	//최대 횟수를 정함
	FVector3 fixup = FVector3::Zero;
	std::vector<FHitResult> hits;

	for (int iter = 0; iter < FPhysScene::MaxPenetrationIterations; iter++)
	{
		QueryOverlaps(_Actor, DesiredPos + fixup, hits, margin);

		if (hits.empty())
		{
			break;
		}

		float error_sum = 0.0f;

		for (const FHitResult& hit : hits)
		{
			FVector3 escape_dir = hit.Normal;

			float depth = (hit.PenetrationDepth > 0.0f) ? hit.PenetrationDepth : FPhysScene::MinPenetrationDepth;
			float required = depth + FPhysScene::PenetrationPullBackDistance;

			//튕길 방향으로 얼마나 갔는지 계산하고 부족분을 더함
			float current = FVector3::Dot(fixup, escape_dir);
			float gap = Math::Max(0.0f, required - current);

			error_sum += gap;
			fixup += escape_dir * gap * FPhysScene::PenetrationRelaxation;
		}

		if (error_sum < FPhysScene::PenetrationConvergenceThreshold)
		{
			break;
		}
	}

	// [안전판] 완화 계수로도 못 막을 만큼 극단적인(또는 기하학적으로 애초에 풀 수 없는)
	// 경우를 대비해서, 최종 fixup 크기 자체를 강제로 제한함 - 이 상황까지 왔다는 건 이미
	// 뭔가 비정상(예: 물체가 좁은 틈에 낌)이라는 뜻이라, "완벽히 안 풀리더라도 최소한
	// 폭주는 안 한다"는 최후 방어선.
	if (fixup.Length() > FPhysScene::MaxFixupMagnitude)
	{
		fixup = FVector3::Normalize(fixup) * FPhysScene::MaxFixupMagnitude;
	}

	_Actor->SetActorLocation(DesiredPos + fixup);
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
