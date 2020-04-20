#  Simple Asset Streaming
Optimize your Unreal Engine 4 project's memory usage by implementing a simple request-and-release asset streaming solution, for Unreal Engine 4.22 and higher.

Feel free to contribute to the plugin by opening issues or pull requests! A wiki will be added soon to remove all of the clutter below.
### What is asset streaming?
Asset streaming is the technique of loading and unloading assets based on demand. This means you only load an asset when it is required, and unload it once nothing needs it.

Once unloaded, the asset will be valid until garbage collected, which only happens when no references to the assets are active. By default, the GC collects once every minute. I request changing it to about three seconds during development for testing (**Project Settings -> Garbage Collection -> Time Between Purging Pending Kill Objects)**.

### Why use asset streaming? Doesn't UE4 already have it?
Asset streaming reduces your memory usage since you won't just load all assets in memory at game launch. By default, UE4 loads all assets referenced by classes through direct references (i.e. pointers). While this means all assets can easily be accessed, imagine what happens when your classes reference multiple gigabytes of assets.

### Why use this over the AssetManager provided by UE4?
It uses the same construct as the AssetManager: the StreamableManager.
This is a simple wrapper around it, allowing your objects to "request" assets, and release them once they don't need it. It will automatically asynchronously load the asset the first time you request it and unload the asset once nothing references it. 

### Alright, how do I install this?

Clone this repository and copy the folder into the Plugins directory of your engine version:
```
C:\Program Files\Epic Games\UE_4.XX\Engine\Plugins\
```
or your project's Plugins directory
```
\YourProject\Plugins\
```
Update your project's `Build.cs` file by adding the `SimpleAssetStreaming` module:
```csharp
PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "SimpleAssetStreaming" });
```
You can now compile your project to gain access to the system in C++ and Blueprints.

## Preparing your project to use asset streaming.
You'll need to modify how you reference your assets in order to implement asset streaming. It takes as much as one direct reference to an asset for it to be loaded immediately, making asset streaming useless for it.

### C++: using `TSoftObjectPtr<>` 
In C++, all references made to assets need to be wrapped using `TSoftObjectPtr<UObject>`.
```cpp
UPROPERTY(EditAnywhere, Category = "Item Assets")
TSoftObjectPtr<UTexture2D> ItemIcon;

UPROPERTY(EditAnywhere, Category = "Item Assets")
TSoftObjectPtr<UStaticMesh> ItemMesh;
```
These will appear in the editor as simple asset pickers depending on the type you provide to the soft object pointer.

### Blueprints: using soft object references.
In Blueprints, all references made to assets need to be set to `Soft Object Reference`.

![Setting your blueprint asset references to soft object references](https://github.com/Erlite/UE4-SimpleAssetStreaming/blob/master/Example01.png)

### Accessing the AssetStreamingSubsystem
Introduced in UE4.22, subsystems are classes that are automatically instantiated by the engine, and [their lifetime depends on their type.](https://docs.unrealengine.com/en-US/Programming/Subsystems/index.html)
The `UAssetStreamingSubsystem` is a game instance subsystem, meaning it is instantiated right after the game instance, and unloaded right before the game instance is destroyed (i.e. your game is closing). 

You can access it in C++ by using the static singleton instance: 
```cpp
UAssetStreamingSubsystem* AssetStreaming = UAssetStreamingSubsystem::Get()
```
or through the game instance:
```cpp
auto* AssetStreaming = GetGameInstance()->GetSubsystem<UAssetStreamingSubsystem>();
```

In Blueprints, the subsystem can simply be accessed using the `Get AssetStreamingSubsystem` node:

![Get AssetStreamingSubsystem node](https://github.com/Erlite/UE4-SimpleAssetStreaming/blob/master/Example02.png)

## Implementing asset streaming
After you've prepared your project accordingly, it's time to implement the plugin.

### How it works:
Let's say you have an item, that has an icon (`UTexture2D`) and a mesh (`UStaticMesh`).

- When the item is created in the world, you tell the asset streaming subsystem you need these assets.
- The asset streaming subsystem gives you a request guid.
- When you don't need the assets anymore (i.e. the item is destroyed), you tell the system to release the assets attributed to the request guid it gave you.

For objects that reference the same assets during their lifetime, you can use `BeginPlay` to request the assets and `BeginDestroy` to release them. 

For more complex scenarios, you can request and release assets as much as you want.

## C++ Implementation
### Requesting and releasing assets.
You can request assets by using `UAssetStreamingSubsystem::RequestAssetStreaming()` with either a `TSoftObjectPtr<>` or a `TArray<TSoftObjectPtr<>>`:
```cpp
FGuid SingleRequestGuid;
FGuid ArrayRequestGuid;
if (auto* Subsystem = UAssetStreamingSubsystem::Get())
{
    Subsystem->RequestAssetStreaming(MySingleAsset, nullptr, SingleRequestGuid);
    Subsystem->RequestAssetStreaming(MyArrayOfAssets, nullptr, ArrayRequestGuid);
}
```
These functions will return a boolean, true if the request was successful. If the request wasn't successful, the request guid will also be invalidated.
Releasing assets is done by using `UAssetStreaming::ReleaseAssets()`, passing in the request guid.
```cpp
Subsystem->ReleaseAssets(SingleRequestGuid);
```
The request guild will be invalidated, and the assets will be "released". They will not be unloaded if other objects need them.

### Getting a callback when an asset is loaded.

You can get a callback when the asset is loaded by using the `IAssetStreamingCallback` interface, and passing it an object that implements it in your streaming request.
```cpp
// Assuming 'this' implements the IAssetStreamingCallback interface.
Subsystem->RequestAssetStreaming(MySingleAsset, this, MyRequestGuid);

// IAssetStreamingCallback implementation
void OnAssetLoaded_Implementation(const TSoftObjectPtr<UObject>& LoadedAsset, const bool bAlreadyLoaded)
{
    // Your asset is loaded, hurray!
}
```
## Blueprint Implementation
### Requesting and releasing assets.
You can request assets by using the `Request Asset Streaming` node or `Request Multiple Asset Streaming`, that take a `Soft Object Reference` or an array of `Soft Object Reference` respectively.
They will return a boolean (true on a successful request) and a request guid (invalid guid if the request wasn't successful).

![Request asset blueprint example](https://github.com/Erlite/UE4-SimpleAssetStreaming/blob/master/Example03.png)

You can then release the assets by using the `Release Assets` node and the request guid that was given for the request.

![Release asset blueprint example](https://github.com/Erlite/UE4-SimpleAssetStreaming/blob/master/Example04.png)

### Getting a callback when an asset is loaded.
You can implement the `AssetStreamingCallback` interface in your object's `Class Settings` and then use the `OnAssetLoaded` event which will be called for every asset requested by your object when they're loaded:

![Asset callback example](https://github.com/Erlite/UE4-SimpleAssetStreaming/blob/master/Example05.png)

## Do's and Dont's

### Do
- Release assets when you don't need them anymore. It takes one reference to keep it active.
- Check that the AssetStreamingSubsystem is valid, especially in `Destroyed/BeginDestroy` functions or events as these can be called when the game is closing and the subsystem will have been destroyed.
- Make sure the request guid you received isn't invalid before trying to release assets.
- Use the IAssetStreamingCallback interface to setup anything dependent on the loaded assets.

### Don't
- Load a lot of assets for no reason. 
- Request/release assets for another object than your own.
- Set the GC time before purging pending kill objects too low outside of testing, as it can cause hitches in performance.
- Use assets without making a request, even if they're valid. They could be valid until the next GC cycle hits, which will cull them out of memory, unless a reference is made by the subsystem.
