#include "../../Public/Utilities/AnimSequenceCache.h"
#include "../../Public/Animation/AnimSequence.h"
#include "../../Public/Asset/Skeleton.h"

#include <fstream>
#include <cstdint>
#include <SDL_log.h>

namespace
{
	constexpr char kMagicBytes[4] = { 'A', 'N', 'I', 'M' };
	constexpr uint32_t kVersion = 1;

	template<typename T>
	void WriteRaw(std::ofstream& Out, const T& Value)
	{
		Out.write(reinterpret_cast<const char*>(&Value), sizeof(T));
	}

	template<typename T>
	bool ReadRaw(std::ifstream& In, T& OutValue)
	{
		In.read(reinterpret_cast<char*>(&OutValue), sizeof(T));
		return In.good();
	}

	void WriteString(std::ofstream& Out, const FString& Str)
	{
		uint32_t Len = static_cast<uint32_t>(Str.size());
		WriteRaw(Out, Len);
		Out.write(Str.data(), Len);
	}

	bool ReadString(std::ifstream& In, FString& OutStr)
	{
		uint32_t Len = 0;
		if (!ReadRaw(In, Len))
		{
			return false;
		}
		OutStr.resize(Len);
		if (Len > 0)
		{
			In.read(&OutStr[0], Len);
		}
		return In.good();
	}
}

FString GetAnimCachePathForFbx(const FString& FbxFilePath)
{
	size_t DotPos = FbxFilePath.find_last_of('.');
	if (DotPos == FString::npos)
	{
		return FbxFilePath + ".animcache";
	}
	return FbxFilePath.substr(0, DotPos) + ".animcache";
}

bool SaveAnimSequenceToBinary(const UAnimSequence& Sequence, const UReferenceSkeleton& SourceSkeleton, const FString& FilePath)
{
	std::ofstream Out(FilePath, std::ios::binary);
	if (!Out.is_open())
	{
		SDL_Log("SaveAnimSequenceToBinary - failed to open '%s' for writing", FilePath.c_str());
		return false;
	}

	Out.write(kMagicBytes, 4);
	WriteRaw(Out, kVersion);
	WriteRaw(Out, Sequence.PlayLength);
	uint8_t bLoopByte = Sequence.bLoop ? 1 : 0;
	WriteRaw(Out, bLoopByte);

	// [트랙 데이터가 있는 본만 저장] 빈 트랙(바인드포즈 폴백 대상)까지 저장하면
	// 용량 낭비 - 로드 시 매칭 안 된 본은 어차피 빈 채로 남는 게 정상 동작.
	TArray<int32> NonEmptyBoneIndices;
	for (int32 i = 0; i < static_cast<int32>(Sequence.BoneTracks.size()); i++)
	{
		if (!Sequence.BoneTracks[i].Times.empty())
		{
			NonEmptyBoneIndices.push_back(i);
		}
	}

	uint32_t NumTracks = static_cast<uint32_t>(NonEmptyBoneIndices.size());
	WriteRaw(Out, NumTracks);

	for (int32 BoneIndex : NonEmptyBoneIndices)
	{
		// 인덱스가 아니라 이름을 저장 - GetRefBoneInfo(idx).Name이 실제 접근자
		// (Skeleton.h로 확인됨 - GetBoneName이라는 별도 함수는 없음).
		WriteString(Out, SourceSkeleton.GetRefBoneInfo(BoneIndex).Name);

		const FAnimTrack& Track = Sequence.BoneTracks[BoneIndex];
		uint32_t NumKeys = static_cast<uint32_t>(Track.Times.size());
		WriteRaw(Out, NumKeys);

		for (uint32_t k = 0; k < NumKeys; k++)
		{
			WriteRaw(Out, Track.Times[k]);
			WriteRaw(Out, Track.Values[k].Rotation.x);
			WriteRaw(Out, Track.Values[k].Rotation.y);
			WriteRaw(Out, Track.Values[k].Rotation.z);
			WriteRaw(Out, Track.Values[k].Rotation.w);
			WriteRaw(Out, Track.Values[k].Translation.x);
			WriteRaw(Out, Track.Values[k].Translation.y);
			WriteRaw(Out, Track.Values[k].Translation.z);
		}
	}

	SDL_Log("SaveAnimSequenceToBinary - saved %u tracks to '%s'", NumTracks, FilePath.c_str());
	return true;
}

UAnimSequence* LoadAnimSequenceFromBinary(const FString& FilePath, const UReferenceSkeleton& TargetSkeleton)
{
	std::ifstream In(FilePath, std::ios::binary);
	if (!In.is_open())
	{
		// 캐시 파일이 없는 건 에러가 아님 - 호출부가 "없으면 FBX로 임포트" 폴백.
		return nullptr;
	}

	char Magic[4];
	In.read(Magic, 4);
	if (!In.good() || Magic[0] != 'A' || Magic[1] != 'N' || Magic[2] != 'I' || Magic[3] != 'M')
	{
		SDL_Log("LoadAnimSequenceFromBinary - '%s' is not a valid cache file (bad magic)", FilePath.c_str());
		return nullptr;
	}

	uint32_t Version = 0;
	if (!ReadRaw(In, Version) || Version != kVersion)
	{
		SDL_Log("LoadAnimSequenceFromBinary - '%s' has unsupported version", FilePath.c_str());
		return nullptr;
	}

	UAnimSequence* Sequence = new UAnimSequence();
	if (!ReadRaw(In, Sequence->PlayLength))
	{
		delete Sequence;
		return nullptr;
	}

	uint8_t bLoopByte = 0;
	ReadRaw(In, bLoopByte);
	Sequence->bLoop = (bLoopByte != 0);

	Sequence->BoneTracks.clear();
	Sequence->BoneTracks.resize(TargetSkeleton.GetBoneCount());

	uint32_t NumTracks = 0;
	if (!ReadRaw(In, NumTracks))
	{
		delete Sequence;
		return nullptr;
	}

	int32 MatchedCount = 0;
	for (uint32_t t = 0; t < NumTracks; t++)
	{
		FString BoneName;
		if (!ReadString(In, BoneName))
		{
			delete Sequence;
			return nullptr;
		}

		uint32_t NumKeys = 0;
		if (!ReadRaw(In, NumKeys))
		{
			delete Sequence;
			return nullptr;
		}

		int32 BoneIndex = TargetSkeleton.FindBoneIndex(BoneName);
		FAnimTrack* Track = (BoneIndex != INDEX_NONE) ? &Sequence->BoneTracks[BoneIndex] : nullptr;
		if (Track != nullptr)
		{
			Track->Times.reserve(NumKeys);
			Track->Values.reserve(NumKeys);
			++MatchedCount;
		}

		// [매칭 안 되면 읽기만 하고 버림] 파일이 순차 구조라, 스킵하더라도 바이트는
		// 다 읽어줘야 다음 트랙 위치가 안 어긋남.
		for (uint32_t k = 0; k < NumKeys; k++)
		{
			float Time = 0.0f, qx = 0.0f, qy = 0.0f, qz = 0.0f, qw = 1.0f, tx = 0.0f, ty = 0.0f, tz = 0.0f;
			ReadRaw(In, Time);
			ReadRaw(In, qx); ReadRaw(In, qy); ReadRaw(In, qz); ReadRaw(In, qw);
			ReadRaw(In, tx); ReadRaw(In, ty); ReadRaw(In, tz);

			if (Track != nullptr)
			{
				Track->Times.emplace_back(Time);
				Track->Values.emplace_back(FQuaternion(qx, qy, qz, qw), FVector3(tx, ty, tz));
			}
		}
	}

	SDL_Log("LoadAnimSequenceFromBinary - loaded '%s': %d/%u tracks matched", FilePath.c_str(), MatchedCount, NumTracks);
	return Sequence;
}
