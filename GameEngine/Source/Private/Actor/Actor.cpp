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

	//이건 자식 클래스의 책임임.
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
	// [철회] 여기서 "생성자에서 SetRootComponent를 꼭 불러야 한다"고 assert를 걸었었는데,
	// TestManager::LoadTestLevel()을 보면 그게 틀린 전제였음 - 거기선 일부러
	// engine->SpawnActor<AActor>()로 루트 없는 빈 액터를 스폰한 뒤, 컴포넌트를 따로
	// new로 만들어서 그걸 나중에(스폰 호출이 끝난 다음 줄에서) SetRootComponent로
	// 붙이는 패턴을 씀. SpawnActor<T>가 new T(...) 직후 바로 Init()을 부르기 때문에,
	// 이 패턴에서는 Init() 시점에 RootComponent == nullptr인 게 정상 상태임.
	// 즉 "루트 없음"은 버그가 아니라 합법적인 상태(실제 언리얼도 동일) - 대신 루트가
	// 세팅되기 전에 GetActorLocation() 등을 호출하는 "사용 순서" 문제는 호출하는 쪽의
	// 책임으로 남겨둠(원본/실제 UE와 같은 계약).
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

FMatrix4x4 AActor::GetActorTransform() const
{
	return GetRootComponent()->GetComponentTransform();
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

	// [수정] 예전엔 NewRootComponent->GetOwner() == nullptr을 조건으로 썼는데,
	// UActorComponent 생성자가 Owner를 생성 시점에 바로 채우기 때문에(new XComponent(this)
	// 호출 시 항상 유효한 포인터가 들어옴) 이 조건은 절대 참이 될 수 없었음 - 결과적으로
	// SetRootComponent가 항상 false로 빠져서 RootComponent가 영원히 안 세팅되는 버그였음.
	// 이제는 "이 컴포넌트가 정말 이 액터 소유인지"를 직접 확인함(다른 액터 소유 컴포넌트를
	// 루트로 삼는 것만 막으면 됨).
	if (NewRootComponent->GetOwner() != this)
	{
		return false;
	}

	RootComponent = NewRootComponent;
	NewRootComponent->SetRelativeLocation(NewRootComponent->GetRelativeLocation());
	RegisterPhysicsBodyIfNeeded();
	return true;
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
