using UnrealBuildTool;
using System.IO;

public class SpartaArcade : ModuleRules
{
	public SpartaArcade(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "NavigationSystem", "AIModule", "Niagara", "EnhancedInput" });
        
        PublicIncludePaths.AddRange(new string[]
        {
	        ModuleDirectory,
	        Path.Combine(ModuleDirectory, "Characters/Public"),
	        Path.Combine(ModuleDirectory, "Framework/Public"),
	        Path.Combine(ModuleDirectory, "Items/Public"),
	        Path.Combine(ModuleDirectory, "Level/Public"),
	        Path.Combine(ModuleDirectory, "Spawn/Public"),
	        Path.Combine(ModuleDirectory, "Systems/Public"),
	        Path.Combine(ModuleDirectory, "UI/Public")
        });

        PrivateIncludePaths.AddRange(new string[]
        {
	        Path.Combine(ModuleDirectory, "Characters/Private"),
	        Path.Combine(ModuleDirectory, "Framework/Private"),
	        Path.Combine(ModuleDirectory, "Items/Private"),
	        Path.Combine(ModuleDirectory, "Level/Private"),
	        Path.Combine(ModuleDirectory, "Spawn/Private"),
	        Path.Combine(ModuleDirectory, "Systems/Private"),
	        Path.Combine(ModuleDirectory, "UI/Private")
        });
    }
}
