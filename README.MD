#  Simple Asset Streaming
Optimize your Unreal Engine 4 project's memory usage by implementing a simple request-and-release asset streaming solution, for Unreal Engine 4.22 and higher.

### What is asset streaming?
Asset streaming is the technique of loading and unloading assets based on demand. This means you only load an asset when it is required, and unload it once nothing needs it.

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

### Preparing your project to use asset streaming.
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

### Using the AssetStreamingSubsystem
Introduced in UE4.22, subsystems are classes that are automatically instantiated by the engine, and [their lifetime depends on their type.](https://docs.unrealengine.com/en-US/Programming/Subsystems/index.html)
The `UAssetStreamingSubsystem` is a game instance subsystem, meaning it is instantiated right after the game instance, and unloaded right before the game instance is destroyed (i.e. your game is closing). 

You can access it in C++ by using the static singleton instance: 
```cpp
UAssetStreamingSubsystem* AssetStreaming = UAssetStreamingSubsystem::Get()
```
or through the game instance
```cpp
auto* AssetStreaming = GetGameInstance()->GetSubsystem<UAssetStreamingSubsystem>();
```

Subsystems in blueprints can simply be accessed using the `Get AssetStreamingSubsystem` node:

![Get AssetStreamingSubsystem node](https://github.com/Erlite/UE4-SimpleAssetStreaming/blob/master/Example02.png)
