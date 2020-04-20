/*

MIT License

Copyright (c) 2020 Younes AIT AMER MEZIANE

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "AssetStreamingSubsystem.h"
#include "AssetStreamingCallback.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPtr.h"
#include "SimpleAssetStreaming.h"

// Singleton instance initialization.
UAssetStreamingSubsystem* UAssetStreamingSubsystem::Instance = nullptr;

void UAssetStreamingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Set the singleton instance to this instance.
	Instance = this;

	// Sanity check on the asset unload delay.
	if (UnloadDelaySeconds < 0.0f)
	{
		UE_LOG(LogAssetStreaming, Error, TEXT("UnloadDelaySeconds cannot be a negative number. Setting it to 5 seconds."));
		UnloadDelaySeconds = 5.0f;
	}
}

void UAssetStreamingSubsystem::Deinitialize()
{
	Instance = nullptr;
}

bool UAssetStreamingSubsystem::RequestAssetStreaming(const TArray<TSoftObjectPtr<>>& AssetsToStream, const TScriptInterface<IAssetStreamingCallback>& AssetLoadedCallback, FGuid& OutAssetRequestId)
{
	CheckThis();

	// Invalidate the request id if there is nothing to stream.
	if (AssetsToStream.Num() == 0)
	{
		OutAssetRequestId.Invalidate();
		return false;
	}

	// Assign a new guid to the request.
	OutAssetRequestId = FGuid::NewGuid();

	UE_LOG(LogAssetStreaming, Verbose, TEXT("Request to stream %s asset(s) received. Request Id: %s"), *FString::FromInt(AssetsToStream.Num()), *OutAssetRequestId.ToString());
	for (TSoftObjectPtr<UObject> Asset : AssetsToStream)
	{
		StreamAsset(Asset, OutAssetRequestId, AssetLoadedCallback);
	}

	// Any asset streaming operation that passes assertions but still isn't valid will cause the request id to invalidate.
	return OutAssetRequestId.IsValid();
}

bool UAssetStreamingSubsystem::RequestAssetStreaming(const TSoftObjectPtr<UObject>& AssetToStream, const TScriptInterface<IAssetStreamingCallback>& AssetLoadedCallback, FGuid& OutAssetRequestId)
{
	CheckThis();

	// Assign a new guid to the request.
	OutAssetRequestId = FGuid::NewGuid();
	StreamAsset(AssetToStream, OutAssetRequestId, AssetLoadedCallback);

	return OutAssetRequestId.IsValid();
}

bool UAssetStreamingSubsystem::ReleaseAssets(FGuid& RequestId)
{
	CheckThis();

	if (!RequestId.IsValid())
	{
		UE_LOG(LogAssetStreaming, Warning, TEXT("Attempted to release assets using an invalid Guid."));
		return false;
	}

	if (!RegisteredAssets.Contains(RequestId))
	{
		UE_LOG(LogAssetStreaming, Warning, TEXT("Attempted to release assets using id '%s' but it leads to no assets."), *RequestId.ToString());
		RequestId.Invalidate();
		return false;
	}

	FAssetHandleArray Assets = RegisteredAssets[RequestId];

	// Decrement the amount of references to each of these assets.
	for (int Index = Assets.Num() - 1; Index >= 0; Index--)
	{
		FAssetHandlePair& Pair = Assets[Index];
		TSoftObjectPtr<UObject> Asset = Pair.Asset;
		FSoftObjectPath AssetPath = Asset.ToSoftObjectPath();

		checkf(AssetRequestCount.Contains(AssetPath), TEXT("Attempted to release asset '%s' but we're not tracking it's count."), *Asset.GetAssetName());
		checkf(!Asset.IsNull(), TEXT("Attempted to release null asset."));
		checkf(Pair.Handle.Get(), TEXT("Asset handle is null."));

		AssetRequestCount[AssetPath]--;

		// If we still have references to this asset, remove it from the array since we're going to re-use it to schedule unloading.
		if (AssetRequestCount[AssetPath] > 0) Assets.RemoveAt(Index);
		// If not, we just remove it from the asset request map.
		else AssetRequestCount.Remove(AssetPath);

		// If the handle of this asset isn't the one to keep alive, cancel it immediately.
		if (Pair.Handle != KeepAlive[AssetPath])
		{
			UE_LOG(LogAssetStreaming, VeryVerbose, TEXT("Handle to release isn't keep-alive, cancelling it."));
			Pair.Handle.Get()->CancelHandle();
		}
		else
		{
			UE_LOG(LogAssetStreaming, VeryVerbose, TEXT("Handle to release is keep-alive, skipping it."));
		}
	}

	// Remove the registered assets for this request id.
	RegisteredAssets.Remove(RequestId);

	// Any asset remaining in the assets array needs to be scheduled for unloading.
	if (Assets.Num() == 0)
	{
		UE_LOG(LogAssetStreaming, Verbose, TEXT("Finished releasing assets without any need for unloading. Request id: '%s'"), *RequestId.ToString());
		RequestId.Invalidate();
		return true;
	}

	// Schedule all assets for unloading. Unload will happen after a delay, to verify that no other objects need the assets one more time.
	ScheduleAssetUnloading(Assets);

	UE_LOG(LogAssetStreaming, Verbose, TEXT("Scheduled %s assets for unloading. Request id: '%s'"), *FString::FromInt(Assets.Num()), *RequestId.ToString());
	RequestId.Invalidate();

	return true;
}

bool UAssetStreamingSubsystem::K2_RequestMultipleAssetStreaming(const TArray<TSoftObjectPtr<UObject>>& AssetsToStream, const TScriptInterface<IAssetStreamingCallback>& AssetLoadedCallback, FGuid& OutAssetRequestId)
{
	return RequestAssetStreaming(AssetsToStream, AssetLoadedCallback, OutAssetRequestId);
}

bool UAssetStreamingSubsystem::K2_RequestAssetStreaming(const TSoftObjectPtr<UObject>& AssetToStream, const TScriptInterface<IAssetStreamingCallback>& AssetLoadedCallback, FGuid& OutAssetRequestId)
{
	return RequestAssetStreaming(AssetToStream, AssetLoadedCallback, OutAssetRequestId);
}

bool UAssetStreamingSubsystem::K2_ReleaseAssets(UPARAM(Ref) FGuid& RequestId)
{
	return ReleaseAssets(RequestId);
}

void UAssetStreamingSubsystem::StreamAsset(const TSoftObjectPtr<UObject>& AssetToStream, const FGuid& RequestId, const TScriptInterface<IAssetStreamingCallback>& AssetLoadedCallback)
{
	checkf(!AssetToStream.IsNull(), TEXT("Attempted to stream null soft object pointer."));

	// Request an asynchronous load of the asset, even if the asset is already loaded. We'll keep the handle.
	FStreamableDelegate OnLoaded;
	const bool bIsAssetLoaded = AssetToStream.IsValid();
	OnLoaded.BindLambda([this, AssetToStream, AssetLoadedCallback, bIsAssetLoaded]() { HandleAssetLoaded(AssetToStream, AssetLoadedCallback, bIsAssetLoaded); });
	TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestAsyncLoad(AssetToStream.ToSoftObjectPath(), OnLoaded, FStreamableManager::DefaultAsyncLoadPriority, true);

	// Register the asset and its handle to the request Id.
	RegisterAssetToId(AssetToStream, Handle, RequestId);

	// We need to keep one handle alive at all times so that we choose when to unload the asset.
	// To do this, we keep the first handle for each asset and never unload it until we really want to release the asset.
	if (!KeepAlive.Contains(AssetToStream.ToSoftObjectPath()))
	{
		KeepAlive.Add(AssetToStream.ToSoftObjectPath(), Handle);
	}

	// Increment the number of references for the asset.
	IncrementAssetReference(AssetToStream);
}

void UAssetStreamingSubsystem::RegisterAssetToId(const TSoftObjectPtr<UObject>& Asset, const TSharedPtr<FStreamableHandle> Handle, const FGuid& Id)
{
	FAssetHandlePair AssetPair = FAssetHandlePair(Asset, Handle);

	if (RegisteredAssets.Contains(Id))
	{
		if (RegisteredAssets[Id].Contains(AssetPair))
		{
			UE_LOG(LogAssetStreaming, Error, TEXT("Attempted to register asset '%s' to Id '%s' but it already exists there."), *Asset.GetAssetName(), *Id.ToString());
			return;
		}

		RegisteredAssets[Id].Add(AssetPair);
	}
	else
	{
		FAssetHandleArray Array;
		Array.Add(AssetPair);
		RegisteredAssets.Add(Id, Array);
	}
	UE_LOG(LogAssetStreaming, Verbose, TEXT("Registered asset '%s' to Id '%s'."), *Asset.GetAssetName(), *Id.ToString());
}

void UAssetStreamingSubsystem::IncrementAssetReference(const TSoftObjectPtr<UObject>& Asset)
{
	checkf(!Asset.IsNull(), TEXT("Cannot increment asset reference of null asset."));
	FSoftObjectPath AssetPath = Asset.ToSoftObjectPath();

	if (AssetRequestCount.Contains(AssetPath))
	{
		AssetRequestCount[AssetPath]++;
	}
	else
	{
		AssetRequestCount.Add(AssetPath, 1);
	}
}

void UAssetStreamingSubsystem::HandleAssetLoaded(const TSoftObjectPtr<UObject>& LoadedAsset, const TScriptInterface<IAssetStreamingCallback>& AssetLoadedCallback, const bool& bAlreadyLoaded)
{
	if (LoadedAsset.IsValid() && AssetLoadedCallback.GetObject()->IsValidLowLevel())
	{
		IAssetStreamingCallback::Execute_OnAssetLoaded(AssetLoadedCallback.GetObject(), LoadedAsset, bAlreadyLoaded);
	}
}

void UAssetStreamingSubsystem::ScheduleAssetUnloading(const FAssetHandleArray& Assets)
{
	if (Assets.Num() == 0)
	{
		UE_LOG(LogAssetStreaming, Warning, TEXT("Attempted to schedule asset unloading with an empty array."));
		return;
	}

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	FTimerHandle Handle;
	FTimerDelegate Delegate;
	Delegate.BindLambda([this, Assets]() { FinalUnloadAssets(Assets); });

	TimerManager.SetTimer(Handle, Delegate, UnloadDelaySeconds, false);
}

void UAssetStreamingSubsystem::FinalUnloadAssets(const FAssetHandleArray& Assets)
{
	// If this somehow runs while the game is quitting, ignore.
	if (!this) return;

	int32 UnloadedAssetsCount = 0;
	for (FAssetHandlePair Pair : Assets)
	{
		TSoftObjectPtr<UObject> Asset = Pair.Asset;
		checkf(!Asset.IsNull(), TEXT("Attempted to unload null asset pointer."));

		FSoftObjectPath AssetPath = Asset.ToSoftObjectPath();

		// Check if a new request to this asset was made. If so, we won't unload it.
		if (AssetRequestCount.Contains(AssetPath)) continue;

		UE_LOG(LogAssetStreaming, VeryVerbose, TEXT("Unloading asset '%s'."), *Asset.GetAssetName());

		// Remove the handle from the KeepAlive array.
		KeepAlive[AssetPath] = nullptr;
		KeepAlive.Remove(AssetPath);

		// Get the active handles for the asset and cancel them. Normally, we should only find one.
		// Cancelling will also stop them from completing if they haven't been loaded yet. The callback won't be called.
		TArray<TSharedRef<FStreamableHandle>> ActiveHandles;
		if (StreamableManager.GetActiveHandles(AssetPath, ActiveHandles, true))
		{
			for (TSharedRef<FStreamableHandle> Handle : ActiveHandles)
			{
				Handle.Get().CancelHandle();
			}

			UnloadedAssetsCount++;
		}
		else
		{
			UE_LOG(LogAssetStreaming, Error, TEXT("Attempted to unload asset '%s' but no active handles were found. We should at least find one?"), *Asset.GetAssetName());
		}
	}

	UE_LOG(LogAssetStreaming, Verbose, TEXT("Finally unloaded %s assets."), *FString::FromInt(UnloadedAssetsCount));
}