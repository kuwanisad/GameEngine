#pragma once
#include "CoreMinimal.h"
#include "../../Public/Actor/FActorSpawnParameters.h"

class AActor;
class SpriteComponent;
class UInputSubsystem;
class UTickManager;
struct FPhysScene;

template <typename T>
concept ActorChildClass = std::derived_from<T, AActor>;

class UEngine
{
private:
	static UEngine* Instance;
	FPhysScene* PhysScene;
	UTickManager* TickManager;
	UInputSubsystem* InputSubsystem;

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

	void RemoveActor(class AActor* _Actor);

	static UEngine* GetEngine();

	void AddActor(AActor* InActor);
	void RemoveActor(AActor* _Actor);

	void AddSprite(SpriteComponent* _Sprite);
	void RemoveSprite(SpriteComponent* _Sprite);

	FPhysScene* GetPhysScene();
	UTickManager* GetTickManager();
	UInputSubsystem* GetInputSubsystem();

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
