// Copyright 2020 Younes Ait Amer Meziane

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "AssetHandlePair.generated.h"

USTRUCT()
struct SIMPLEASSETSTREAMING_API FAssetHandlePair
{
	GENERATED_BODY()

public:

	FAssetHandlePair()
		: Asset(nullptr)
		, Handle(nullptr)
	{}

	FAssetHandlePair(const TSoftObjectPtr<UObject>& InAsset, const TSharedPtr<FStreamableHandle>& InHandle)
		: Asset(InAsset)
		, Handle(InHandle)
	{}

	FORCEINLINE bool operator==(const FAssetHandlePair& RHS) const
	{
		return Asset == RHS.Asset;
	}

	TSoftObjectPtr<UObject> Asset;

	TSharedPtr<FStreamableHandle> Handle;
};