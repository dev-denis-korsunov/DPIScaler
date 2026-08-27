using UnrealBuildTool;
using System.IO;

public class DPIScalerEditor : ModuleRules
{
	public DPIScalerEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "DPIScaler", "Private"));
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "DPIScaler", "Slate", "SlateCore", "UMG", "UMGEditor", "UnrealEd"
		});
	}
}
