#include "../../Public/Rendering/VertexContainer.h"
#include "../../Public/Rendering/RenderingStatics.h"
#include "../../Public/Core/CoreMinimal.h"

#include <GL/glew.h>

FVertexContainer::FVertexContainer(const void* _Verts, unsigned int _VertexCount, Layout _Layout, const unsigned int* _Indices, unsigned int _IndexCount)
	: VertexCount(_VertexCount),
	IndexCount(_IndexCount)

{
	glGenVertexArrays(1, &VAO_ID);
	glBindVertexArray(VAO_ID);

	//위치 float 3, 노멀 float 3, 텍스처 uv 2
	engine_size_t VertexSize = RenderingStatics::VertexDefaultSize;

	switch (_Layout)
	{
	case FVertexContainer::Layout::PosNormTex:
		break;
	case FVertexContainer::Layout::PosNormSkinTex:
		//본 인덱스 + 보간값이라서 *2
		VertexSize += RenderingStatics::VertexLerpBoneCount * 2;
		break;
	}

	//버텍스 버퍼
	glGenBuffers(1, &VBO_ID);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_ID);
	glBufferData(GL_ARRAY_BUFFER, _VertexCount * VertexSize, _Verts, GL_STATIC_DRAW);

	//인덱스 버퍼
	glGenBuffers(1, &IBO_ID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_ID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, _IndexCount * sizeof(unsigned int), _Indices, GL_STATIC_DRAW);

	size_t PointerOffset = 0;
	int ShaderLayout = 0;

	//위치
	glEnableVertexAttribArray(ShaderLayout);
	glVertexAttribPointer(ShaderLayout, 3, GL_FLOAT, GL_FALSE, VertexSize, reinterpret_cast<void*>(PointerOffset));
	++ShaderLayout;
	PointerOffset += RenderingStatics::VertexPosSize;

	//노멀
	glEnableVertexAttribArray(ShaderLayout);
	glVertexAttribPointer(ShaderLayout, 3, GL_FLOAT, GL_FALSE, VertexSize, reinterpret_cast<void*>(PointerOffset));
	++ShaderLayout;
	PointerOffset += RenderingStatics::VertexNormSize;

	switch (_Layout)
	{
	case FVertexContainer::Layout::PosNormTex:
		//텍스처 uv
		glEnableVertexAttribArray(ShaderLayout);
		glVertexAttribPointer(ShaderLayout, 2, GL_FLOAT, GL_FALSE, VertexSize, reinterpret_cast<void*>(PointerOffset));
		break;
	case FVertexContainer::Layout::PosNormSkinTex:
		//스키닝 본 인덱스(int)
		glEnableVertexAttribArray(ShaderLayout);

		//4개의 본을 가지고 보간
		glVertexAttribIPointer(ShaderLayout, RenderingStatics::VertexLerpBoneCount, GL_UNSIGNED_BYTE, VertexSize, reinterpret_cast<void*>(PointerOffset));
		++ShaderLayout;
		PointerOffset += sizeof(char) * RenderingStatics::VertexLerpBoneCount;

		//스키닝 본 가중치(float)
		glEnableVertexAttribArray(ShaderLayout);
		glVertexAttribPointer(ShaderLayout, RenderingStatics::VertexLerpBoneCount, GL_UNSIGNED_BYTE, GL_TRUE, VertexSize, reinterpret_cast<void*>(PointerOffset));
		++ShaderLayout;
		PointerOffset += sizeof(char) * RenderingStatics::VertexLerpBoneCount;

		//텍스처 uv
		glEnableVertexAttribArray(ShaderLayout);
		glVertexAttribPointer(ShaderLayout, 2, GL_FLOAT, GL_FALSE, VertexSize, reinterpret_cast<void*>(PointerOffset));
		break;
	}
}

FVertexContainer::~FVertexContainer()
{
	glDeleteBuffers(1, &VBO_ID);
	glDeleteBuffers(1, &IBO_ID);
	glDeleteVertexArrays(1, &VAO_ID);
}

void FVertexContainer::SetActive()
{
	glBindVertexArray(VAO_ID);
}
