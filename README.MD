#  Simple Asset Streaming
Optimize your Unreal Engine 4 project's memory usage by implementing a simple request-and-release asset streaming solution, for Unreal Engine 4.22 and higher.

### What is asset streaming?
Asset streaming is the technique of loading and unloading assets based on demand. This means you only load an asset when it is required, and unload it once nothing needs it.

### Why use asset streaming? Doesn't UE4 already have it?
Asset streaming reduces your memory usage since you won't just load all assets in memory at game launch. By default, UE4 loads all assets referenced by classes through direct references (i.e. pointers). While this means all assets can easily be accessed, imagine what happens when your classes reference multiple gigabytes of assets.

### Why use this over the AssetManager provided by UE4?
It uses the same construct as the AssetManager: the StreamableManager.
This is a simple wrapper around it, allowing your objects to "request" assets, and release them once they don't need it. It will automatically asynchronously load the asset the first time you request it and unload the asset once nothing references it. 

### Alright, where do I start?

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
