#include "../../Public/Rendering/Renderer.h"
#include "../../Public/Rendering/VertexContainer.h"
#include "../../Public/Rendering/Shader.h"
#include "../../Public/Rendering/RenderingStatics.h"
#include "../../Public/Asset/AssetManager.h"

#include <GL/glew.h>

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

	// Set OpenGL attributes
	// Use the core OpenGL profile
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	// Specify version 3.3
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	// Request a color buffer with 8-bits per RGBA channel
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	// Enable double buffering
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	// Force OpenGL to use hardware acceleration
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	Window = SDL_CreateWindow("Game Programming in C++ (Chapter 6)", 100, 100,
		static_cast<int>(ScreenWidth), static_cast<int>(ScreenHeight), SDL_WINDOW_OPENGL);
	if (!Window)
	{
		SDL_Log("Failed to create window: %s", SDL_GetError());
		return false;
	}

	// Create an OpenGL context
	Context = SDL_GL_CreateContext(Window);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		SDL_Log("Failed to initialize GLEW.");
		return false;
	}

	// On some platforms, GLEW will emit a benign error code,
	// so clear it
	glGetError();

	// Make sure we can create/compile shaders
	if (LoadShaders() == false)
	{
		SDL_Log("Failed to load shaders.");
		return false;
	}

	// Create quad for drawing sprites
	CreateSpriteVerts();

	return false;
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
}

void URenderer::EndFrame()
{
}

void URenderer::SetLightUniforms(FShaderPipeline* InShader)
{
	FMatrix4x4 invView = Matrix_View;
	invView.Invert();
	InShader->SetVector(URenderingStatics::Shader_Light_uCameraPos.c_str(), invView.GetTranslation());
	InShader->SetVector(URenderingStatics::Shader_Light_uAmbientLight.c_str(), AmbientLight);
	InShader->SetVector(URenderingStatics::Shader_Light_uDirLight_mDirection.c_str(), DirectionalLight.Direction);
	InShader->SetVector(URenderingStatics::Shader_Light_uDirLight_mDiffuseColor.c_str(), DirectionalLight.DiffuseColor);
	InShader->SetVector(URenderingStatics::Shader_Light_uDirLight_mSpecColor.c_str(), DirectionalLight.SpecColor);
}
