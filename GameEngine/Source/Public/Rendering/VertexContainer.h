#pragma once

class FVertexContainer
{
private:
	unsigned int VertexCount;
	unsigned int IndexCount;
	unsigned int VBO_ID;
	unsigned int IBO_ID;
	unsigned int VAO_ID;

public:
	//해당 순서대로 들어감
	enum class Layout
	{
		PosNormTex,
		PosNormSkinTex
	};

	FVertexContainer(const void* _Verts, unsigned int _VertexCount, Layout _Layout,
		const unsigned int* _Indices, unsigned int _IndexCount);
	~FVertexContainer();

	void SetActive();

	unsigned int GetIndexCount() const { return IndexCount; }
	unsigned int GetVertexCount() const { return VertexCount; }
};