#include "../../Public/Asset/AssetManager.h"
#include "../../Public/Core/Math.h"
#include "../../Public/Rendering/VertexContainer.h"
#include "../../Public/Rendering/RenderingStatics.h"
#include "../../Public/Rendering/Texture.h"
#include "../../Public/Rendering/Mesh.h"
#include "../../Public/Rendering/Shader.h"
#include "../../Public/stb_image/stb_image.h"
#include "../../Public/Asset/FbxImporter.h"
#include "../../Public/Animation/AnimSequence.h"
#include "../../Public/Asset/SkeletalMesh.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <rapidjson/document.h>
#include <SDL_log.h>

UAssetManager* UAssetManager::AssetManager = nullptr;
FAssetHandle UAssetManager::InvalidHandle = 0;


FAssetHandle UAssetManager::GetNewAssetHandle()
{
	return NextAssetHandle++;
}

bool UAssetManager::CompileShader(const FString& FileName, GLenum ShaderType, GLuint& OutShader)
{
	std::ifstream File(FileName);

	if (File.is_open() == false)
	{
		SDL_Log("Shader file not found: %s", FileName.c_str());
		return false;
	}

	std::stringstream ss;
	ss << File.rdbuf();
	FString Data = ss.str();
	const char* DataChar = Data.c_str();

	OutShader = glCreateShader(ShaderType);
	glShaderSource(OutShader, 1, &DataChar, nullptr);
	glCompileShader(OutShader);

	if (!IsShaderProgramValid(OutShader))
	{
		SDL_Log("Failed to compile shader %s", FileName.c_str());
		return false;
	}

	return true;
}

bool UAssetManager::IsShaderCompiled(GLuint Shader)
{
	GLint status;
	glGetShaderiv(Shader, GL_COMPILE_STATUS, &status);

	if (status != GL_TRUE)
	{
		char buffer[512];
		memset(buffer, 0, 512);
		glGetShaderInfoLog(Shader, 511, nullptr, buffer);
		SDL_Log("GLSL Compile Failed:\n%s", buffer);
		return false;
	}

	return true;
}

bool UAssetManager::IsShaderProgramValid(const FShaderPipeline* ShaderPipeline)
{
	return IsShaderProgramValid(ShaderPipeline->GetProgramID());
}

bool UAssetManager::IsShaderProgramValid(GLuint ProgramID)
{
	GLint status;
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &status);
	if (status != GL_TRUE)
	{
		char buffer[512];
		memset(buffer, 0, 512);
		glGetProgramInfoLog(ProgramID, 511, nullptr, buffer);
		SDL_Log("GLSL Link Status:\n%s", buffer);
		return false;
	}

	return true;
}

UAssetManager* UAssetManager::Get()
{
	if (AssetManager == nullptr)
	{
		AssetManager = new UAssetManager();
	}

	return AssetManager;
}

UMesh* UAssetManager::LoadMesh(const FString& FileName)
{
	FString new_mesh_name = std::filesystem::path(FileName).stem().string();
	auto itr = Meshes.find(new_mesh_name);

	if (itr != Meshes.end())
	{
		return itr->second;
	}

	std::ifstream file(FileName);

	if (file.is_open() == false)
	{
		SDL_Log("File not found: Mesh %s", FileName.c_str());
		return nullptr;
	}

	std::stringstream file_stream;
	file_stream << file.rdbuf();
	FString contents = file_stream.str();
	rapidjson::StringStream jsonStr(contents.c_str());
	rapidjson::Document doc;
	doc.ParseStream(jsonStr);

	if (!doc.IsObject())
	{
		SDL_Log("Mesh %s is not valid json", FileName.c_str());
		return nullptr;
	}

	int version = doc["version"].GetInt();

	if (version != 1)
	{
		SDL_Log("Mesh %s not version 1", FileName.c_str());
		return nullptr;
	}

	FString shader_name = doc["shader"].GetString();

	FVertexContainer::Layout layout = FVertexContainer::Layout::PosNormTex;
	size_t vertex_size = 8;

	FString vertex_format = doc["vertexformat"].GetString();
	if (vertex_format == "PosNormSkinTex")
	{
		layout = FVertexContainer::Layout::PosNormSkinTex;
		vertex_size = 10;
	}

	const rapidjson::Value& textures = doc["textures"];
	if (!textures.IsArray() || textures.Size() < 1)
	{
		SDL_Log("Mesh %s has no textures", FileName.c_str());
		return nullptr;
	}

	float spec = static_cast<float>(doc["specularPower"].GetDouble());

	bool is_default_loaded = false;

	for (rapidjson::SizeType i = 0; i < textures.Size(); i++)
	{
		FString texture_name = textures[i].GetString();
		auto current_texture = LoadTexture(texture_name);

		//기본값
		if (current_texture == nullptr && is_default_loaded == false)
		{
			is_default_loaded = true;
			LoadTexture("Assets/Default.png");
		}
	}

	const rapidjson::Value& vert_json = doc["vertices"];
	if (!vert_json.IsArray() || vert_json.Size() < 1)
	{
		SDL_Log("Mesh %s has no vertices", FileName.c_str());
		return nullptr;
	}

	std::vector<float> vertices;
	vertices.reserve(vert_json.Size() * vertex_size);

	FAABB Box;

	for (rapidjson::SizeType i = 0; i < vert_json.Size(); i++)
	{
		// For now, just assume we have 8 elements
		const rapidjson::Value& vert = vert_json[i];
		if (!vert.IsArray() || vert.Size() != 8)
		{
			SDL_Log("Unexpected vertex format for %s", FileName.c_str());
			return nullptr;
		}

		FVector3 pos(vert[0].GetDouble(), vert[1].GetDouble(), vert[2].GetDouble());
		Box.UpdateMinMax(pos);

		// Add the floats
		for (rapidjson::SizeType i = 0; i < vert.Size(); i++)
		{
			vertices.emplace_back(static_cast<float>(vert[i].GetDouble()));
		}
	}

	const rapidjson::Value& index_json = doc["indices"];
	if (!index_json.IsArray() || index_json.Size() < 1)
	{
		SDL_Log("Mesh %s has no indices", FileName.c_str());
		return nullptr;
	}

	std::vector<unsigned int> indices;
	indices.reserve(index_json.Size() * 3);
	for (rapidjson::SizeType i = 0; i < index_json.Size(); i++)
	{
		const rapidjson::Value& ind = index_json[i];
		if (!ind.IsArray() || ind.Size() != 3)
		{
			SDL_Log("Invalid indices for %s", FileName.c_str());
			return nullptr;
		}

		indices.emplace_back(ind[0].GetUint());
		indices.emplace_back(ind[1].GetUint());
		indices.emplace_back(ind[2].GetUint());
	}

	FVertexContainer* new_vert_container = new FVertexContainer(vertices.data(), static_cast<unsigned>(vertices.size()) / vertex_size,
		layout, indices.data(), static_cast<unsigned>(indices.size()));

	UMesh* new_mesh = new UMesh();
	new_mesh->SetName(new_mesh_name);
	new_mesh->SetVertexContainer(new_vert_container);
	new_mesh->SetBox(Box);

	Meshes.emplace(new_mesh_name, new_mesh);

	return new_mesh;
}

bool UAssetManager::UnloadMesh(const UMesh* Mesh)
{
	auto itr = Meshes.find(Mesh->GetName());

	if (itr != Meshes.end())
	{
		itr->second->Unload();
		return true;
	}

	return false;
}

UMesh* UAssetManager::GetMesh(const FString& Name)
{
	auto itr = Meshes.find(Name);

	if (itr != Meshes.end())
	{
		return itr->second;
	}

	SDL_Log("Mesh %s is not exists.", Name.c_str());

	return nullptr;
}

FShaderPipeline* UAssetManager::LoadShaderPipeline(const FString& PipelineName, const FString& VertName, const FString& FragName)
{
	GLuint VertexShaderID;
	GLuint FragShaderID;
	GLuint ProgramID;

	if (CompileShader(VertName, GL_VERTEX_SHADER, VertexShaderID) == false)
	{
		return nullptr;
	}

	if (CompileShader(FragName, GL_FRAGMENT_SHADER, FragShaderID) == false)
	{
		return nullptr;
	}

	ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragShaderID);
	glLinkProgram(ProgramID);

	if (IsShaderProgramValid(ProgramID) == false)
	{
		return nullptr;
	}

	auto new_pipeline = new FShaderPipeline();
	new_pipeline->SetName(PipelineName);
	new_pipeline->SetIDs(ProgramID, VertexShaderID, FragShaderID);
	ShaderPipelines.emplace(PipelineName, new_pipeline);

	return new_pipeline;
}

bool UAssetManager::UnloadShaderPipeline(FShaderPipeline* ShaderPipeline)
{
	ShaderPipeline->Unload();
	ShaderPipelines.erase(ShaderPipeline->GetName());

	return true;
}

FShaderPipeline* UAssetManager::GetShaderPipeline(const FString& Name)
{
	auto itr = ShaderPipelines.find(Name);

	if (itr != ShaderPipelines.end())
	{
		return itr->second;
	}

	SDL_Log("ShaderPipeline %s is not exists.", Name.c_str());

	return nullptr;
}

UTexture* UAssetManager::GetTexture(const FString& Name)
{
	auto itr = Textures.find(Name);

	if (itr != Textures.end())
	{
		return itr->second;
	}

	SDL_Log("Texture %s is not exists.", Name.c_str());

	return nullptr;
}

FAssetHandle UAssetManager::LoadSkeleton(const FString& FileName)
{
	auto FbxImporter = UFbxImporter::Get();

	if (FbxImporter->ImportFromFile(FileName) == false)
	{
		return InvalidHandle;
	}

	USkeleton* OutSkeleton = new USkeleton();

	if (FbxImporter->ImportSkeleton((UReferenceSkeleton&)OutSkeleton->GetReferenceSkeleton()) == false)
	{
		return InvalidHandle;
	}

	FAssetHandle Handle = GetNewAssetHandle();
	Skeletons[Handle] = OutSkeleton;

	return Handle;
}

FAssetHandle UAssetManager::LoadAnimSequence(const FString& FileName, const UReferenceSkeleton& TargetSkeleton, float SampleRate)
{
	auto FbxImporter = UFbxImporter::Get();

	if (FbxImporter->ImportFromFile(FileName) == false)
	{
		return InvalidHandle;
	}

	auto* OutSequence = new UAnimSequence();

	if (FbxImporter->ImportAnimation(TargetSkeleton, *OutSequence) == false)
	{
		return InvalidHandle;
	}

	return RegisterAnimSequence(OutSequence);
}

FAssetHandle UAssetManager::RegisterAnimSequence(UAnimSequence* Sequence)
{
	FAssetHandle Handle = GetNewAssetHandle();
	AnimSequences[Handle] = Sequence;
	return Handle;
}

void UAssetManager::PackSkeletonVertexBuffer(const TArray<FSoftSkinVertex>& SoftVertices, TArray<uint8>& OutBuffer)
{
	OutBuffer.resize(SoftVertices.size() * URenderingStatics::TotalVertexOffset);

	for (size_t i = 0; i < SoftVertices.size(); ++i)
	{
		const FSoftSkinVertex& V = SoftVertices[i];
		uint8* Dst = OutBuffer.data() + i * URenderingStatics::TotalVertexOffset;

		std::memcpy(Dst + URenderingStatics::VertexPositionOffset, V.Position.GetAsFloatPtr(), sizeof(float) * 3);
		std::memcpy(Dst + URenderingStatics::VertexNormalOffset, V.TangentZ.GetAsFloatPtr(), sizeof(float) * 3);
		std::memcpy(Dst + URenderingStatics::BoneIndexOffset, V.InfluenceBones, 4);
		std::memcpy(Dst + URenderingStatics::BoneWeightOffset, V.InfluenceWeights, 4);
		std::memcpy(Dst + URenderingStatics::TextureUVOffset, &V.UVs[0].x, sizeof(float) * 2);
	}
}

bool UAssetManager::LoadSkeletalMesh(const FString& FileName, FAssetHandle& OutMeshHandle, FAssetHandle& OutSkelHandle)
{
	OutMeshHandle = InvalidHandle;
	OutSkelHandle = InvalidHandle;

	auto Importer = UFbxImporter::Get();

	if (Importer->ImportFromFile(FileName) == false)
	{
		SDL_Log("USkeletalMesh::Load - failed to open '%s'", FileName.c_str());
		return false;
	}

	auto* NewSkeleton = new USkeleton();
	if (Importer->ImportSkeleton((UReferenceSkeleton&)NewSkeleton->GetReferenceSkeleton()) == false)
	{
		SDL_Log("USkeletalMesh::Load - failed to import skeleton");
		return false;
	}

	TArray<FBoneInfluence> Influences;
	int32 VertexCount = 0;
	if (Importer->ImportSkinWeights(NewSkeleton->GetReferenceSkeleton(), Influences, VertexCount) == false)
	{
		SDL_Log("USkeletalMesh::Load - failed to import skin weights");
		return false;
	}

	FSkeletalMeshImportData ImportData;
	if (Importer->ImportGeometry(ImportData) == false)
	{
		SDL_Log("USkeletalMesh::Load - failed to import geometry");
		return false;
	}

	FSkeletalMeshLODModel LODModel;
	if (URenderingStatics::BuildSkeletalMesh(LODModel,
		Influences, ImportData.Wedges, ImportData.Faces, ImportData.Points) == false)
	{
		SDL_Log("USkeletalMesh::Load - BuildSkeletalMesh failed");
		return false;
	}

	if (LODModel.Sections.empty())
	{
		SDL_Log("USkeletalMesh::Load - no sections produced");
		return false;
	}

	const FSkelMeshSection& Section = LODModel.Sections[0];
	TArray<uint8> VertexBuffer;
	PackSkeletonVertexBuffer(Section.SoftVertices, VertexBuffer);

	USkeletalMesh* NewSM = new USkeletalMesh();

	//나중에 텍스트 로드해야 함
	UTexture* NewTexture = UAssetManager::CreateWhite1x1();

	auto* NewVertexContainer = new FVertexContainer(VertexBuffer.data(), static_cast<unsigned int>(Section.SoftVertices.size()),
		FVertexContainer::Layout::PosNormSkinTex, Section.Indices.data(), static_cast<unsigned int>(Section.Indices.size()));

	NewSM->SetVertexContainer(NewVertexContainer);
	NewSM->SetSkeleton(NewSkeleton);
	NewSM->SetTexture(NewTexture);

	FAssetHandle NewHandle = GetNewAssetHandle();
	FAssetHandle NewHandle2 = GetNewAssetHandle();

	SkeletalMeshes[NewHandle] = NewSM;
	Skeletons[NewHandle2] = (USkeleton*)NewSM->GetSkeleton();

	SDL_Log("USkeletalMesh::Load - loaded '%s': %d bones, %d vertices, %d triangles",
		FileName.c_str(), NewSkeleton->GetReferenceSkeleton().GetBoneCount(),
		static_cast<int32>(Section.SoftVertices.size()), Section.TriangleCount);

	OutMeshHandle = NewHandle;
	OutSkelHandle = NewHandle2;

	return true;
}

const USkeleton* UAssetManager::GetSkeleton(FAssetHandle Handle) const
{
	auto it = Skeletons.find(Handle);
	return (it != Skeletons.end()) ? it->second : nullptr;
}

UAnimSequence* UAssetManager::GetAnimSequence(FAssetHandle Handle) const
{
	auto it = AnimSequences.find(Handle);
	return (it != AnimSequences.end()) ? it->second : nullptr;
}

USkeletalMesh* UAssetManager::GetSkeletalMesh(FAssetHandle Handle) const
{
	auto it = SkeletalMeshes.find(Handle);
	return (it != SkeletalMeshes.end()) ? it->second : nullptr;
}

UTexture* UAssetManager::LoadTexture(const FString& FileName)
{
	std::filesystem::path new_name = FileName;
	auto new_texture_name = new_name.stem().string();

	auto itr = Textures.find(new_texture_name);

	if (itr != Textures.end())
	{
		return itr->second;
	}

	int channels = 0;

	int32 new_width;
	int32 new_height;
	uint32 new_ID;

	unsigned char* image = stbi_load(FileName.c_str(), &new_width, &new_height, &channels, 0);

	if (image == nullptr)
	{
		SDL_Log("stb_image failed to load image %s: %s", FileName.c_str(), stbi_failure_reason());
		return nullptr;
	}

	int color_format = GL_RGB;
	if (channels == 4)
	{
		color_format = GL_RGBA;
	}
	else if (channels == 1)
	{
		color_format = GL_RED;
	}

	UTexture* new_texture = new UTexture();

	glGenTextures(1, &new_ID);
	glBindTexture(GL_TEXTURE_2D, new_ID);

	glTexImage2D(GL_TEXTURE_2D, 0, color_format, new_width, new_height, 0, color_format, GL_UNSIGNED_BYTE, image);

	stbi_image_free(image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	new_texture->SetID(new_ID);
	new_texture->SetSize(new_width, new_height);
	new_texture->SetName(new_texture_name);
	
	Textures.emplace(new_texture_name, new_texture);

	return new_texture;
}

UTexture* UAssetManager::CreateWhite1x1()
{
	auto itr = Textures.find(URenderingStatics::WhiteTextureName);

	if (itr != Textures.end())
	{
		return itr->second;
	}

	int32 width = 1;
	int32 height = 1;
	uint32 id = 0;

	auto new_texture = new UTexture();

	unsigned char white[4] = { 255, 255, 255, 255 };

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	new_texture->SetID(id);
	new_texture->SetName(URenderingStatics::WhiteTextureName);
	new_texture->SetSize(width, height);

	Textures.emplace(URenderingStatics::WhiteTextureName, new_texture);

	return new_texture;
}

bool UAssetManager::UnloadTexture(const UTexture* Texture)
{
	auto itr = Textures.find(Texture->GetName());

	if (itr != Textures.end())
	{
		itr->second->Unload();
		return true;
	}

	return false;
}
