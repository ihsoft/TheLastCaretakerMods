using UnrealBuildTool;
public class VoyageItemPatchEditor : ModuleRules { public VoyageItemPatchEditor(ReadOnlyTargetRules Target) : base(Target) { PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs; PrivateDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","Voyage","UnrealEd","Kismet","KismetCompiler","BlueprintGraph","Json","AssetRegistry"}); } }
