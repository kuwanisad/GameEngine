#include "../../Public/Rendering/Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../Public/stb_image/stb_image.h"

#include <GL/glew.h>
#include <SDL.h>

UTexture::UTexture() :
	ID(0),
	width(0),
	Height(0)
{
}

UTexture::~UTexture()
{
}

void UTexture::SetName(const FString& InName)
{
	Name = InName;
}

void UTexture::SetSize(int32 InWidth, int32 InHeight)
{
	width = InWidth;
	Height = InHeight;
}

void UTexture::SetID(uint32 InID)
{
	ID = InID;
}

void UTexture::SetActive()
{
	glBindTexture(GL_TEXTURE_2D, ID);
}

void UTexture::Unload()
{
	glDeleteTextures(1, &ID);
}
