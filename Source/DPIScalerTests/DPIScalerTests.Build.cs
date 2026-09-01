using UnrealBuildTool;
using System.IO;

public class DPIScalerTests : ModuleRules
{
	public DPIScalerTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "DPIScaler", "Private"));
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "DPIScaler"
		});
	}
}
