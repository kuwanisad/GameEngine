#pragma once
#include "CoreMinimal.h"
#include "../../Public/Actor/FActorSpawnParameters.h"

class AActor;
class UActorComponent;
class SpriteComponent;
class UInputSubsystem;
class UTickManager;
class URenderingManager;
struct FPhysScene;

template <typename T>
concept ActorChildClass = std::derived_from<T, AActor>;

template <typename T>
concept ActorComponentChildClass = std::derived_from<T, UActorComponent>;

class UEngine
{
private:
	static UEngine* Instance;
	FPhysScene* PhysScene;
	UTickManager* TickManager;
	UInputSubsystem* InputSubsystem;
	URenderingManager* RenderingManager;

	TArray<AActor*> Actors;
	TArray<AActor*> PendingActors;

	bool IsRunning = false;
	bool IsUpdatingActors = false;
	uint32 TickCount;

	float CurrentDeltaSeconds = 0.f;

	void ProcessInput();
	void UpdateGame();
	void GenerateOutput();
	void LoadData();
	void UnloadData();

	void ResetAccumulatedForceThisTick();

public:
	UEngine();
	~UEngine();

	bool Initialize();
	void RunLoop();
	void Shutdown();

	template<ActorChildClass T>
	T* SpawnActor(const FActorSpawnParameters& Params);

	template<ActorChildClass T>
	T* SpawnActor(const FVector3& Location, const FQuaternion& Rotation, const FVector3& Scale);

	template<typename T, typename... Args>
	T* SpawnActor(Args&&... args)
	{
		T* actor = new T(std::forward<Args>(args)...);
		actor->Init();
		return actor;
	}

	// SpawnActor와 같은 모양 - 컴포넌트를 생성(new)과 동시에 Init()까지 호출해줘서,
	// Init() 안에 있는 "Owner->AddComponent(this) + 틱 등록"이 실제로 일어나게 함.
	// 지금까지 흩어져 있던 new XComponent(owner) 호출들을 이걸로 바꿔줘야 함.
	template<ActorComponentChildClass T, typename... Args>
	T* CreateComponent(AActor* Owner, Args&&... args)
	{
		T* Comp = new T(Owner, std::forward<Args>(args)...);
		Comp->Init();
		return Comp;
	}

	static UEngine* GetEngine();

	void AddActor(AActor* InActor);
	void RemoveActor(AActor* InActor);

	FPhysScene* GetPhysScene();
	UTickManager* GetTickManager();
	UInputSubsystem* GetInputSubsystem();
	URenderingManager* GetRenderingManager();

	float GetDeltaSeconds() const { return CurrentDeltaSeconds; }
};

template<ActorChildClass T>
T* UEngine::SpawnActor(const FActorSpawnParameters& Params)
{
	T* actor = new T();
	actor->Init();

	actor->SetActorLocation(Params.Location);
	actor->SetActorRotation(Params.Rotation);
	actor->SetActorScale(Params.Scale);

	return actor;
}

template<ActorChildClass T>
inline T* UEngine::SpawnActor(const FVector3& Location, const FQuaternion& Rotation, const FVector3& Scale)
{
	FActorSpawnParameters Params;
	Params.Location = Location;
	Params.Rotation = Rotation;
	Params.Scale = Scale;

	return SpawnActor(Params);
}
