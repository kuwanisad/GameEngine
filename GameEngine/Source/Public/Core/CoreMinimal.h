#pragma once
#include "PCH.h"

//향후 변경에 대응하기 위함
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using engine_size_t = size_t;

//이런 식으로 명명하는 이유는 언리얼을 교재로 삼았음을 뜻함
using FString = std::string;

template <typename T>
using TArray = std::vector<T>;

template <typename T>
using TSet = std::unordered_set<T>;

template <typename KeyType, typename ValueType>
using TMap = std::unordered_map<KeyType, ValueType>;

template <typename T>
using TQueue = std::queue<T>;

template <typename T, typename U>
using TPair = std::pair<T, U>;

// UE의 INDEX_NONE(부모 없음/찾지 못함 등을 나타내는 값)
constexpr int32 INDEX_NONE = -1;

// UE의 WITH_DEV_AUTOMATION_TESTS 대응
// 테스트 전용 코드는 전부 이거로 감싸고
// 나중에 Shipping 빌드 개념이 생기면 이 값만 0으로 바꿔서 테스트 코드를 통째로 컴파일에서 제외
#ifndef WITH_DEV_AUTOMATION_TESTS
#define WITH_DEV_AUTOMATION_TESTS 1
#endif
