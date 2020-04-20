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