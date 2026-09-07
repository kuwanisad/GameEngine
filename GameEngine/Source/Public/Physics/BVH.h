#pragma once
#include "../Core/Math.h"
#include "../Collision/Collision.h"

#include <vector>
#include <algorithm>
#include <cfloat>

struct FBVHNode
{
	FAABB box;

	FBVHNode* parent = nullptr;
	FBVHNode* left = nullptr;
	FBVHNode* right = nullptr;

	FCollision* object = nullptr;

	bool IsLeaf() const { return object != nullptr; }
};

struct FBVH
{
protected:
	FAABB Union(const FAABB& a, const FAABB& b);

	FBVHNode* root = nullptr;


};