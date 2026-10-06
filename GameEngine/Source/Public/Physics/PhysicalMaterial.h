#pragma once
#include "../../Public/Core/PCH.h"

struct UPhysicalMaterial
{
public:
	// 반발 계수
	float Restitution = 0.3f;

	// 마찰 계수
	float Friction = 0.7f;
};