// Copyright 2020 Younes Ait Amer Meziane

#pragma once

#include "CoreMinimal.h"
#include "AssetHandlePair.h"
#include "Engine/StreamableManager.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AssetStreamingSubsystem.generated.h"

class UObject;
class IAssetStreamingCallback;
typedef TArray<FAssetHandlePair> FAssetHandleArray;
typedef TArray<TSharedRef<FStreamableHandle>> FStreamableHandleArray;

/**
 * Subsystem used to asynchronously load and unload assets when required.
 */
UCLASS()
class SIMPLEASSETSTREAMING_API UAssetStreamingSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    UAssetStreamingSubsystem()
        : StreamableManager()
        , UnloadDelaySeconds(5.0f) // Modify this to change the delay before assets are finally unloaded. Cannot be negative.
        , RegisteredAssets()
        , AssetRequestCount()
        , KeepAlive()
    {}

    // Returns the singleton instance of the asset streaming subsystem.
    FORCEINLINE static UAssetStreamingSubsystem* Get() { return Instance; }

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    virtual void Deinitialize() override;

    /**
     * Request streaming of multiple assets.
     * Each asset will be streamed one by one.
     * @param AssetsToStream The assets to asynchronously stream.
     * @param AssetLoadedCallback The callback to call when an asset is loaded. Will be called once by asset loaded.
     * @param OutAssetRequestId The request id assigned for your request. Use it to release the assets you streamed. Invalidated if the request was invalid.
     * @returns True if the request was successful.
     */
    bool RequestAssetStreaming(const TArray<TSoftObjectPtr<UObject>>& AssetsToStream, const TScriptInterface<IAssetStreamingCallback>& AssetLoadedCallback, FGuid& OutAssetRequestId);

    /**
     * Request streaming of a single asset.
     * @param AssetToStream The asset to asynchronously stream.
     * @param AssetLoadedCallback The callback to call when an asset is loaded.
     * @param OutAssetRequestId The request id assigned for your request. Use it to release the asset you streamed. Invalidated if the request was invalid.
     * @returns True if the request was successful.
     */
    bool RequestAssetStreaming(const TSoftObjectPtr<UObject>& AssetToStream, const TScriptInterface<IAssetStreamingCallback>& AssetLoadedCallback, FGuid& OutAssetRequestId);

    /**
     * Release the asset you streamed.
     * Warning: must be called when you don't need the streamed assets anymore!
     * @param RequestId The id returned by the streaming request. Will be invalidated after releasing assets.
     * @returns True if releasing was successful.
     */
    bool ReleaseAssets(FGuid& RequestId);

protected:

    /**
     * Request streaming of multiple assets.
     * Each asset will be streamed one by one.
     * @param AssetsToStream The assets to asynchronously stream.
     * @param AssetLoadedCallback The callback to call when an asset is loaded. Will be called once by asset loaded.
     * @param OutAssetRequestId The request id assigned for your request. Use it to release the assets you streamed. Invalidated if the request was invalid.
     * @returns True if the request was successful.
     */
    UFUNCTION(BlueprintCallable, DisplayName = "Request Multiple Asset Streaming", Category = "Asset Streaming Functions")
    bool K2_RequestMultipleAssetStreaming(const TArray<TSoftObjectPtr<UObject>>& AssetsToStream, const TScriptInterface<IAssetStreamingCallback>& AssetLoadedCallback, FGuid& OutAssetRequestId);

    /**
     * Request streaming of a single asset.
     * @param AssetToStream The asset to asynchronously stream.
     * @param AssetLoadedCallback The callback to call when an asset is loaded.
     * @param OutAssetRequestId The request id assigned for your request. Use it to release the asset you streamed. Invalidated if the request was invalid.
     * @returns True if the request was successful.
     */
    UFUNCTION(BlueprintCallable, DisplayName = "Request Asset Streaming", Category = "Asset Streaming Functions")
    bool K2_RequestAssetStreaming(const TSoftObjectPtr<UObject>& AssetToStream, const TScriptInterface<IAssetStreamingCallback>& AssetLoadedCallback, FGuid& OutAssetRequestId);

    /**
     * Release the asset you streamed.
     * Warning: must be called when you don't need the streamed assets anymore!
     * @param RequestId The id returned by the streaming request. Will be invalidated.
     * @returns True if releasing was successful.
     */
    UFUNCTION(BlueprintCallable, DisplayName = "Release Assets", Category = "Asset Streaming Functions")
    bool K2_ReleaseAssets(UPARAM(Ref) FGuid& RequestId);

private:

    void StreamAsset(const TSoftObjectPtr<UObject>& AssetToStream, const FGuid& RequestId, const TScriptInterface<IAssetStreamingCallback>& AssetLoadedCallback);

    void RegisterAssetToId(const TSoftObjectPtr<UObject>& Asset, const TSharedPtr<FStreamableHandle> Handle, const FGuid& Id);

    void IncrementAssetReference(const TSoftObjectPtr<UObject>& Asset);

    void HandleAssetLoaded(const TSoftObjectPtr<UObject>& LoadedAsset, const TScriptInterface<IAssetStreamingCallback>& AssetLoadedCallback, const bool& bAlreadyLoaded);

    void ScheduleAssetUnloading(const FAssetHandleArray& Assets);

    void FinalUnloadAssets(const FAssetHandleArray& Assets);

    // Singleton instance.
    static UAssetStreamingSubsystem* Instance;

    // The manager used to load/unload assets from memory.
    FStreamableManager StreamableManager;

    // The amount of time to wait before finally unloading an asset when its references drop to zero.
    float UnloadDelaySeconds;

    // Maps request guid to the requested assets and their handle.
    TMap<FGuid, FAssetHandleArray> RegisteredAssets;

    // Maps asset paths to the number of requests they have.
    TMap<FSoftObjectPath, int32> AssetRequestCount;

    // Handles to keep alive until we finally unload the asset.
    TMap<FSoftObjectPath, TSharedPtr<FStreamableHandle>> KeepAlive;
};
