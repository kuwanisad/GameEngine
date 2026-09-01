#pragma once


template<typename T, typename U>
T* Cast(U* comp)
{
	return dynamic_cast<T*>(comp);
}