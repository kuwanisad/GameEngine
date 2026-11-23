#include "../../Public/Rendering/Renderer.h"
#include "../../Public/Rendering/VertexContainer.h"
#include "../../Public/Rendering/Shader.h"
#include "../../Public/Rendering/RenderingStatics.h"
#include "../../Public/Rendering/RenderingManager.h"
#include "../../Public/Rendering/Texture.h"
#include "../../Public/Asset/AssetManager.h"
#include "../../Public/Asset/SkeletalMesh.h"
#include "../../Public/Component/StaticMeshComponent.h"
#include "../../Public/Component/SpriteComponent.h"
#include "../../Public/Asset/Skeleton.h"

#include <GL/glew.h>
#include <SDL_log.h>

URenderer::URenderer() :
	Context(nullptr),
	VertexContainer_Sprite(nullptr),
	ShaderPipeline_Sprite(nullptr),
	ShaderPipeline_Mesh(nullptr),
	ShaderPipeline_Skin(nullptr),
	Window(nullptr),
	ScreenWidth(0),
	ScreenHeight(0)
{
}

URenderer::~URenderer()
{
}

bool URenderer::Init()
{
	if (GetScreenWidth() <= 0.f || GetScreenHeight() <= 0)
	{
		return false;
	}

	// OpenGL 속성 설정
	// 코어 OpenGL 프로파일 사용
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	// 버전 3.3 지정
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	// RGBA 채널당 8비트 컬러 버퍼 요청
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	// 더블 버퍼링 활성화
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	// 하드웨어 가속 사용 강제
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	Window = SDL_CreateWindow("Game Programming in C++ (Chapter 6)", 100, 100,
		static_cast<int>(ScreenWidth), static_cast<int>(ScreenHeight), SDL_WINDOW_OPENGL);
	if (!Window)
	{
		SDL_Log("Failed to create window: %s", SDL_GetError());
		return false;
	}

	// OpenGL 컨텍스트 생성
	Context = SDL_GL_CreateContext(Window);

	// GLEW 초기화
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		SDL_Log("Failed to initialize GLEW.");
		return false;
	}

	// 일부 플랫폼에서는 GLEW가 무해한 에러 코드를 뱉으므로,
	// 그걸 지워줌
	glGetError();

	// 셰이더를 생성/컴파일할 수 있는지 확인
	if (LoadShaders() == false)
	{
		SDL_Log("Failed to load shaders.");
		return false;
	}

	// 스프라이트 그리기용 쿼드 생성
	CreateSpriteVerts();

	WorkerThread = std::thread(&URenderer::WorkerLoop, this);

	return true;
}

void URenderer::DispatchToWorker(FSkeletalMeshInstance& Instance, FAnimSnapshotBase* Snapshot, float DeltaSeconds)
{
	Instance.DrawWorldPosition = Instance.LatestWorldPosition;
	Instance.DrawWorldRotation = Instance.LatestWorldRotation;
	Instance.DrawWorldScale = Instance.LatestWorldScale;

	const USkeleton* Skel = UAssetManager::Get()->GetSkeleton(Instance.SkeletonHandle);

	int32 WriteIndex = (Instance.ReadIndex + 1) % URenderingStatics::PaletteBufferCount;

	FSkeletalMeshInstance* InstancePtr = &Instance;

	Instance.IsHasPendingSwap = true;
	IAnimGraphBase* Graph = Instance.Graph;

	std::shared_ptr<FAnimSnapshotBase> OwnedSnapshot(Snapshot);

	PendingJobCount.fetch_add(1);

	{
		std::lock_guard<std::mutex> Lock(QueueMutex);

		PendingCommands.emplace_back(
			[this, OwnedSnapshot = std::move(OwnedSnapshot), InstancePtr, WriteIndex, Graph, Skel, DeltaSeconds]() mutable
			{
				TArray<FBoneTransform> LocalPose;

				if (Graph != nullptr)
				{
					Graph->Update(DeltaSeconds, *OwnedSnapshot);
					Graph->Evaluate(LocalPose, *OwnedSnapshot);
				}

				if (Skel != nullptr)
				{
					const UReferenceSkeleton& RefSkel = Skel->GetReferenceSkeleton();
					int32 BoneCount = RefSkel.GetBoneCount();

					TArray<FMatrix4x4> ComponentSpace(BoneCount);

					for (int32 i = 0; i < BoneCount; ++i)
					{
						FMatrix4x4 LocalMatrix = (i < static_cast<int32>(LocalPose.size())) ? LocalPose[i].ToMatrix() : FMatrix4x4::Identity;
						int32 ParentIndex = RefSkel.GetParentIndex(i);
						ComponentSpace[i] = (ParentIndex == INDEX_NONE) ? LocalMatrix : LocalMatrix * ComponentSpace[ParentIndex];
					}

					const TArray<FMatrix4x4>& InverseBindPose = Skel->GetInverseBindPoseComponentSpace();
					InstancePtr->RenderingPalette[WriteIndex].resize(BoneCount);

					for (int32 i = 0; i < BoneCount; ++i)
					{
						InstancePtr->RenderingPalette[WriteIndex][i] = InverseBindPose[i] * ComponentSpace[i];
					}
				}

				InstancePtr->CompletedCount.fetch_add(1);
				this->PendingJobCount.fetch_sub(1);
			}
		);
	}

	QueueCV.notify_one();
	Instance.RenderFence.BeginFance(&Instance.CompletedCount);
}

bool URenderer::IsPendingJob()
{
	return PendingJobCount.load() > 0;
}

void URenderer::WorkerLoop()
{
	while (true)
	{
		TArray<std::function<void()>> Batch;
		{
			std::unique_lock<std::mutex> Lock(QueueMutex);
			QueueCV.wait(Lock, [this] {return PendingCommands.size() || IsShuttingDown;});

			if (PendingCommands.empty() && IsShuttingDown)
			{
				return;
			}

			Batch.swap(PendingCommands);
		}

		for (auto& Command : Batch)
		{
			Command();
		}
	}
}

void URenderer::UploadAndDraw(FSkeletalMeshInstance& Instance, FShaderPipeline* SkinnedShader)
{
	USkeletalMesh* Mesh = UAssetManager::Get()->GetSkeletalMesh(Instance.SkeletalMeshHandle);

	if (Mesh == nullptr)
	{
		return;
	}

	FVertexContainer* vc = Mesh->GetVertexContainer();

	if (vc == nullptr)
	{
		return;
	}

	FMatrix4x4 WorldTransform = FMatrix4x4::TransformScaleToMatrix(Instance.DrawWorldScale) *
		FMatrix4x4::TransformQuaternionToMatrix(Instance.DrawWorldRotation) *
		FMatrix4x4::TransformTranslationToMatrix(Instance.DrawWorldPosition);

	SkinnedShader->SetMatrix(URenderingStatics::Shader_uWorldTransform.c_str(), WorldTransform);

	TArray<FMatrix4x4> Palette = Instance.RenderingPalette[Instance.ReadIndex];

	if (Palette.empty())
	{
		Palette.resize(URenderingStatics::MaxBoneMatrixCount, FMatrix4x4::Identity);
	}
	else if (static_cast<int32>(Palette.size()) > URenderingStatics::MaxBoneMatrixCount)
	{
		Palette.resize(URenderingStatics::MaxBoneMatrixCount);
	}

	SkinnedShader->SetMatrices(URenderingStatics::Shader_uMatrixPalette.c_str(), Palette.data(), static_cast<uint32>(Palette.size()));

	UTexture* Tex = Mesh->GetTexture();

	if (Tex != nullptr)
	{
		Tex->SetActive();
	}

	vc->SetActive();
	glDrawElements(GL_TRIANGLES, vc->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
}

bool URenderer::LoadShaders()
{
	ShaderPipeline_Sprite = UAssetManager::Get()->LoadShaderPipeline("SpriteShader", "Shaders/Sprite.vert", "Shaders/Sprite.frag");

	ShaderPipeline_Sprite->SetActive();
	FMatrix4x4 ViewProjection = FMatrix4x4::CreateSimpleViewProj(ScreenWidth, ScreenHeight);
	ShaderPipeline_Sprite->SetMatrix(URenderingStatics::Shader_uViewProj.c_str(), ViewProjection);

	ShaderPipeline_Mesh = UAssetManager::Get()->LoadShaderPipeline("MeshShader", "Shaders/Phong.vert", "Shaders/Phong.frag");

	ShaderPipeline_Mesh->SetActive();

	Matrix_View = FMatrix4x4::CreateLookAt(FVector3::Zero, FVector3::UnitX, FVector3::UnitZ);
	
	Matrix_Projection = FMatrix4x4::CreatePerspectiveFOV(URenderingStatics::PerspectiveFOV_DegreeY,
		ScreenWidth, ScreenHeight, URenderingStatics::PerspectiveFOV_Near, URenderingStatics::PerspectiveFOV_Far);
	
	ShaderPipeline_Mesh->SetMatrix(URenderingStatics::Shader_uViewProj.c_str(), GetViewProjMatrix());

	ShaderPipeline_Skin = UAssetManager::Get()->LoadShaderPipeline("SkinShader", "Shaders/Skinned.vert", "Shaders/Phong.frag");
	
	ShaderPipeline_Skin->SetActive();
	ShaderPipeline_Skin->SetMatrix(URenderingStatics::Shader_uViewProj.c_str(), GetViewProjMatrix());

	return true;
}

void URenderer::CreateSpriteVerts()
{
	float vertices[] = {
	-0.5f, 0.5f, 0.f, 0.f, 0.f, 0.0f, 0.f, 0.f, // top left
	0.5f, 0.5f, 0.f, 0.f, 0.f, 0.0f, 1.f, 0.f, // top right
	0.5f,-0.5f, 0.f, 0.f, 0.f, 0.0f, 1.f, 1.f, // bottom right
	-0.5f,-0.5f, 0.f, 0.f, 0.f, 0.0f, 0.f, 1.f  // bottom left
	};

	unsigned int indices[] = {
		0, 1, 2,
		2, 3, 0
	};

	VertexContainer_Sprite = new FVertexContainer(vertices, 4, FVertexContainer::Layout::PosNormTex, indices, 6);
}

void URenderer::Shutdown()
{
	{
		std::lock_guard<std::mutex> Lock(QueueMutex);
		IsShuttingDown = true;
	}

	QueueCV.notify_all();

	if (WorkerThread.joinable())
	{
		WorkerThread.join();
	}

	delete VertexContainer_Sprite;

	ShaderPipeline_Sprite->Unload();
	delete ShaderPipeline_Sprite;

	ShaderPipeline_Mesh->Unload();
	ShaderPipeline_Skin->Unload();

	SDL_GL_DeleteContext(Context);
	SDL_DestroyWindow(Window);
}

void URenderer::UnloadData()
{
	//아직은 딱히 할 게 없음...
}

void URenderer::BeginFrame()
{	
	// 클리어 컬러를 밝은 회색으로 설정
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// 컬러 버퍼 클리어
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 메시 컴포넌트 그리기
	// 깊이 버퍼링 활성화/알파 블렌드 비활성화
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	ShaderPipeline_Mesh->SetActive();
	ShaderPipeline_Mesh->SetMatrix(URenderingStatics::Shader_uViewProj.c_str(), GetViewProjMatrix());
	SetLightUniforms(ShaderPipeline_Mesh);

	for (auto comp : RenderingManager->GetStaticMeshComps().GetArray())
	{
		if (comp != nullptr)
		{
			comp->Draw(ShaderPipeline_Mesh);
		}
	}

	//BeginFrame ~ EndFrame 사이에 스켈레탈 메시가 랜더링되는데 작업이 좀 복잡해서 RenderFrame에서 진행됨
}

void URenderer::RenderFrame()
{
	ShaderPipeline_Skin->SetActive();
	ShaderPipeline_Skin->SetMatrix(URenderingStatics::Shader_uViewProj.c_str(), GetViewProjMatrix());
	SetLightUniforms(ShaderPipeline_Skin);

	for (auto& Instance : RenderingManager->GetSkeletalMeshInstances())
	{
		if (Instance->IsSlotOccupied && Instance->IsAlive)
		{
			UploadAndDraw(*Instance, ShaderPipeline_Skin);
		}
	}
}

void URenderer::EndFrame()
{
	//스프라이트는 UI로만 쓸 예정

	// 모든 스프라이트 컴포넌트 그리기
	// 깊이 버퍼링 비활성화
	glDisable(GL_DEPTH_TEST);
	// 컬러 버퍼에 알파 블렌딩 활성화
	glEnable(GL_BLEND);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

	ShaderPipeline_Sprite->SetActive();

	for (auto comp : RenderingManager->GetSpriteComps().GetArray())
	{
		if (comp != nullptr)
		{
			comp->Draw(ShaderPipeline_Sprite);
		}
	}

	// 버퍼 스왑
	SDL_GL_SwapWindow(Window);
}

void URenderer::SetLightUniforms(FShaderPipeline* InShader) const
{
	FMatrix4x4 invView = Matrix_View;
	invView.Invert();
	InShader->SetVector(URenderingStatics::Shader_Light_uCameraPos.c_str(), invView.GetTranslation());
	InShader->SetVector(URenderingStatics::Shader_Light_uAmbientLight.c_str(), AmbientLight);
	InShader->SetVector(URenderingStatics::Shader_Light_uDirLight_mDirection.c_str(), DirectionalLight.Direction);
	InShader->SetVector(URenderingStatics::Shader_Light_uDirLight_mDiffuseColor.c_str(), DirectionalLight.DiffuseColor);
	InShader->SetVector(URenderingStatics::Shader_Light_uDirLight_mSpecColor.c_str(), DirectionalLight.SpecColor);
}
