#include "../../Public/Utilities/BlendSpaceManifest.h"
#include "../../Public/Asset/FbxImporter.h"
#include "../../Public/Asset/Skeleton.h"
#include "../../Public/Animation/AnimSequence.h"
#include "../../Public/Rendering/RenderingManager.h"
#include "../../Public/Core/Engine.h"
#include "../../Public/Asset/AssetManager.h"
#include "FAnimSequenceCache.h"

#include <fstream>
#include <sstream>
#include <SDL_log.h>

namespace
{
	void Trim(FString& S)
	{
		size_t Start = S.find_first_not_of(" \t\r");
		size_t End = S.find_last_not_of(" \t\r");
		S = (Start == FString::npos) ? FString() : S.substr(Start, End - Start + 1);
	}
}

TArray<FBlendSpaceManifestEntry> ParseBlendSpaceManifest(const FString& ManifestPath)
{
	TArray<FBlendSpaceManifestEntry> Result;

	std::ifstream In(ManifestPath);
	if (!In.is_open())
	{
		SDL_Log("ParseBlendSpaceManifest - failed to open '%s'", ManifestPath.c_str());
		return Result;
	}

	FString Line;
	int32 LineNumber = 0;
	while (std::getline(In, Line))
	{
		LineNumber++;

		Trim(Line);
		if (Line.empty() || Line[0] == '#')
		{
			continue;
		}

		std::stringstream ss(Line);
		FString Name, PosXStr, PosYStr, FbxPath;
		std::getline(ss, Name, ',');
		std::getline(ss, PosXStr, ',');
		std::getline(ss, PosYStr, ',');
		std::getline(ss, FbxPath, ',');

		Trim(Name); Trim(PosXStr); Trim(PosYStr); Trim(FbxPath);

		if (Name.empty() || PosXStr.empty() || PosYStr.empty() || FbxPath.empty())
		{
			SDL_Log("ParseBlendSpaceManifest - skipping malformed line %d: '%s'", LineNumber, Line.c_str());
			continue;
		}

		FBlendSpaceManifestEntry Entry;
		Entry.Name = Name;
		try
		{
			Entry.Position = FVector2(std::stof(PosXStr), std::stof(PosYStr));
		}
		catch (...)
		{
			SDL_Log("ParseBlendSpaceManifest - invalid position on line %d: '%s'", LineNumber, Line.c_str());
			continue;
		}
		Entry.FbxPath = FbxPath;
		Result.push_back(Entry);
	}

	SDL_Log("ParseBlendSpaceManifest - loaded %d entries from '%s'", static_cast<int32>(Result.size()), ManifestPath.c_str());
	return Result;
}

TArray<FBlendSpaceManifestEntry> ImportAndRegisterManifest(const FString& ManifestPath, const UReferenceSkeleton& TargetSkeleton)
{
	TArray<FBlendSpaceManifestEntry> Entries = ParseBlendSpaceManifest(ManifestPath);

	int32 CacheHitCount = 0;
	int32 FbxImportCount = 0;
	int32 FailCount = 0;

	for (FBlendSpaceManifestEntry& Entry : Entries)
	{
		// [캐시 우선] 이미 만들어둔 파이프라인(FAnimSequenceCache) 그대로 재사용.
		FString CachePath = GetAnimCachePathForFbx(Entry.FbxPath);
		UAnimSequence* Seq = LoadAnimSequenceFromBinary(CachePath, TargetSkeleton);

		if (Seq != nullptr)
		{
			CacheHitCount++;
		}
		else if (UFbxImporter::Get()->ImportFromFile(Entry.FbxPath))
		{
			// [캐시 없음 - FBX 파싱] 반드시 이 호출 전에, 스켈레톤 임포트가 필요했던
			// 씬(캐릭터 FBX)으로부터 이미 필요한 걸 다 뽑아낸 상태여야 함 - ImportFromFile이
			// 씬을 교체하므로. TargetSkeleton은 이미 그 작업이 끝난 뒤 만들어진
			// FReferenceSkeleton을 인자로 받으므로, 호출부가 순서를 지켰다는 전제.
			Seq = new UAnimSequence();
			Seq->SequenceName = Entry.Name;

			if (UFbxImporter::Get()->ImportAnimation(TargetSkeleton, *Seq))
			{
				SaveAnimSequenceToBinary(*Seq, TargetSkeleton, CachePath);
				FbxImportCount++;
			}
			else
			{
				delete Seq;
				Seq = nullptr;
			}
		}

		if (Seq != nullptr)
		{
			Entry.SequenceHandle = UAssetManager::Get()->RegisterAnimSequence(Seq);
			Entry.bImportSucceeded = true;
		}
		else
		{
			SDL_Log("ImportAndRegisterManifest - FAILED: '%s' ('%s')", Entry.Name.c_str(), Entry.FbxPath.c_str());
			FailCount++;
		}
	}

	SDL_Log("ImportAndRegisterManifest - %d entries: %d cache hits, %d newly imported, %d failed",
		static_cast<int32>(Entries.size()), CacheHitCount, FbxImportCount, FailCount);

	return Entries;
}
