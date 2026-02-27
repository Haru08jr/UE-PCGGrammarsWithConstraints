using System.IO;
using UnrealBuildTool;

public class ConstrainedGrammarGenerator : ModuleRules
{
    public ConstrainedGrammarGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
	    PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "PCG" });
	    //Type = ModuleType.External;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
	    
	 
        // Add any macros that need to be set
        //PublicDefinitions.Add("WITH_CONSTRAINEDGRAMMARGENERATOR=1");
	 
        // Add any include paths for the plugin
        PublicIncludePaths.AddRange(new string[]
        {
	        Path.Combine(ModuleDirectory, "PCGConstrainedGrammarGenerator/source/public"),
	        Path.Combine(ModuleDirectory, "PCGConstrainedGrammarGenerator/libraries")
        });
	 
        // Add any import libraries or static libraries
        //PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "foo.a"));
    }
}