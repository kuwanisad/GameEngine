#include "../../public/Actor/Actor.h"
#include "../../Public/Core/Engine.h"
#include "../../Public/Tick/TickManager.h"
#include "../../Public/Physics/PhysScene.h"
#include "../../Public/Component/SceneComponent.h"
#include "../../Public/Core/Cast.h"
#include "../../Public/Component/ShapeComponent.h"
#include "../../Public/Collision/Collision.h"

AActor::AActor() :
	RootComponent(nullptr)
{
	PrimaryActorTick.Target = this;

	//This is Child's responsibility.
	PrimaryActorTick.CanEverTick = true;
}

AActor::~AActor()
{
	if (IsRegisteredInPhysicsLayer)
	{
		UEngine::GetEngine()->GetPhysScene()->UnregisterBody(this);
	}

	UEngine::GetEngine()->GetTickManager()->UnregisterTickFunction(&PrimaryActorTick);

	UEngine::GetEngine()->RemoveActor(this);

	while (!Components.empty())
	{
		delete Components.back();
	}
}

void AActor::Init()
{
	UEngine::GetEngine()->AddActor(this);

	UEngine::GetEngine()->GetTickManager()->RegisterTickFunction(&PrimaryActorTick);
}

void AActor::Tick(float DeltaTime)
{
}

FVector3 AActor::GetActorLocation() const
{
	return RootComponent->GetRelativeLocation();;
}

void AActor::SetActorLocation(const FVector3& pos, bool Sweep, float margin)
{
	if (Sweep)
	{
		UEngine::GetEngine()->GetPhysScene()->MoveActor(this, pos, margin);
	}
	else
	{
		GetRootComponent()->SetRelativeLocation(pos);
	}
}

const FVector3& AActor::GetActorScale() const
{
	return GetRootComponent()->GetRelativeScale3D();
}

void AActor::SetActorScale(const FVector3& scale)
{
	GetRootComponent()->SetRelativeScale3D(scale);
}

FQuaternion AActor::GetActorRotation() const
{
	return GetRootComponent()->GetRelativeRotation();
}

void AActor::SetActorRotation(FQuaternion rotation)
{
	GetRootComponent()->SetRelativeRotation(rotation);
}

void AActor::ComputeWorldTransform()
{
	GetRootComponent()->ComputeWorldTransform();
	GetRootComponent()->OnUpdateWorldTransform();
}

void AActor::AddComponent(UActorComponent* component)
{
	Components.emplace_back(component);
	component->SetOwner(this);
	RegisterPhysicsBodyIfNeeded();
}

void AActor::RemoveComponent(UActorComponent* component)
{
	auto iter = std::find(Components.begin(), Components.end(), component);
	if (iter != Components.end())
	{
		Components.erase(iter);
	}
}

TArray<FCollision*> AActor::GetCollisions() const
{
	TArray<FCollision*> result;

	for (auto comp : Components)
	{
		if (comp->GetComponentType() == EComponentType::Shape)
		{
			FCollision* c = Cast<UShapeComponent>(comp)->GetCollision();
			if (c)
			{
				result.push_back(c);
			}
		}
	}

	return result;
}

std::vector<UPrimitiveComponent*> AActor::GetPrimitiveComponents() const
{
	std::vector<UPrimitiveComponent*> result;

	for (auto comp : Components)
	{
		UPrimitiveComponent* prim = Cast<UPrimitiveComponent>(comp);
		if (prim)
		{
			result.push_back(prim);
		}
	}

	return result;
}

FVector3 AActor::GetActorForwardVector() const
{
	return RootComponent->GetForwardVector();
}

FVector3 AActor::GetActorRightVector() const
{
	return RootComponent->GetRightVector();
}

bool AActor::SetRootComponent(USceneComponent* NewRootComponent)
{
	if (NewRootComponent == nullptr)
	{
		return false;
	}

	if (NewRootComponent == GetRootComponent())
	{
		return true;
	}

	if (NewRootComponent->GetOwner() == nullptr)
	{
		AddComponent(NewRootComponent);

		RootComponent = NewRootComponent;
		NewRootComponent->SetRelativeLocation(NewRootComponent->GetRelativeLocation());
		RegisterPhysicsBodyIfNeeded();
		return true;
	}

	return false;
}

void AActor::RegisterPhysicsBodyIfNeeded()
{
	if (IsRegisteredInPhysicsLayer)
	{
		return;
	}

	if (GetCollisions().size())
	{
		UEngine::GetEngine()->GetPhysScene()->RegisterBody(this);
	}
}
