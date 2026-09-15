#pragma once
#include "../Component/ShapeComponent.h"
#include "../Collision/Collision.h"

template<typename T, typename U>
T* Cast(U* comp)
{
	return dynamic_cast<T*>(comp);
}