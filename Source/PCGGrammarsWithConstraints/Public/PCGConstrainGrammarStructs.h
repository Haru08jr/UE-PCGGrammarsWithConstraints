
#pragma once

#include <map>
#include <string>

#include "CoreMinimal.h"
#include "Generator.hpp"
#include "automaton/NFA.hpp"
#include "Elements/Grammar/PCGSubdivisionBase.h"

#include "PCGConstrainGrammarStructs.generated.h"

USTRUCT(BlueprintType)
struct FPCGGrammarConstraint
{
	GENERATED_BODY()
	/** Symbol in the grammar. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "")
	FText Symbol;

	/** Position along the generation shape. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "")
	float Position = 100.0;

	/** Position along the generation shape. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "")
	bool bHasWidth = false;

	/** Position along the generation shape. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "", meta=(EditCondition = "bHasWidth"))
	float Width = 100.0;
};

USTRUCT(BlueprintType)
struct FPCGGrammarConstraintAttributeNames
{
	GENERATED_BODY()

	/** Mandatory. Expected type: FName. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "")
	FName SymbolAttributeName = PCGSubdivisionBase::Constants::SymbolAttributeName;
};

USTRUCT(BlueprintType)
struct FPCGConstrainedGrammarModule
{
	GENERATED_BODY()
	
	/** Symbol for the grammar. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "")
	FName Symbol = NAME_None;

	/** Size of the block, aligned on the segment direction. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "")
	double Size = 100.0;

	/** If the volume can be scaled to fit the remaining space or not. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "")
	bool bScalable = false;

	/** For easier debugging, using Point color in conjunction with PCG Debug Color Material. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Debug")
	FVector4 DebugColor = FVector4::One();
	
	/** If this is true, this module can only be placed if specified by a constraint. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "")
	bool bSpawnOnlyWithConstraint = false;
};

USTRUCT()
struct FPCGConstrainedGrammarModuleAttributeNames : public FPCGSubdivisionModuleAttributeNames
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "", meta = (InlineEditConditionToggle))
	bool bProvideSpawnOnlyWithConstraint = true;

	/** Optional. Expected type: bool. If disabled, default value will be false. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "", meta = (EditCondition = "bProvideSpawnOnlyWithConstraint"))
	FName SpawnOnlyWithConstraintAttributeName = TEXT("bSpawnOnlyWithConstraint");
};

UENUM()
enum SubdivisionType
{
	Spline,
	Segment
};

struct FPCGGrammarConstrainingContext : public FPCGContext
{
	TMap<FString, NFA> ConstructedNFAs;
	std::map<std::string, GrammarModule> ModuleMap;
};