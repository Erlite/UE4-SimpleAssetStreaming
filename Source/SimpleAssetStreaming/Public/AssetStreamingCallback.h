// Copyright 2020 Younes Ait Amer Meziane

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AssetStreamingCallback.generated.h"

class UObject;
template<typename T = UObject> struct TSoftObjectPtr;

#define CheckThis() checkf(this, TEXT("Attempted to access asset streaming subsystem but it has already been destroyed. Check the pointer first."))

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UAssetStreamingCallback : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface used to receive callbacks when an asset is loaded by the asset streaming subsystem.
 * Callbacks will be called each time a requested asset is loaded, or directly if it was already loaded.
 */
class SIMPLEASSETSTREAMING_API IAssetStreamingCallback
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, DisplayName = "On Asset Loaded")
	void OnAssetLoaded(const TSoftObjectPtr<UObject>& LoadedAsset, const bool bWasAlreadyLoaded);
	virtual void OnAssetLoaded_Implementation(const TSoftObjectPtr<UObject>& LoadedAsset, const bool bWasAlreadyLoaded);
};