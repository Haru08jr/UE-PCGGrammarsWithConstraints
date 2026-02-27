// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PCGContext.h"
#include "automaton/NFA.hpp"
#include "Elements/Grammar/PCGSubdivisionBase.h"
#include "UObject/Object.h"

#include "GrammarConstrainer.generated.h"

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

/**
 * 
 */
UCLASS()
class CONSTRAINEDGRAMMARGENERATOR_API UGrammarConstrainer : public UObject
{
	GENERATED_BODY()
	
public:
	FString GenerateWithConstraints(FPCGContext* InContext, const FString& GrammarString, float Length, const PCGSubdivisionBase::FModuleInfoMap& Modules, const TArray<FPCGGrammarConstraint>& Constraints);
	
private:
	/** 
	 * If ConstructedNFAs does not contain a NFA corresponding to GrammarString, create a new one and add it to the map.
	 * Returns true if the NFA was created or already existed, and false if the creation of the NFA failed.
	 */
	bool MakeNFAForGrammar(FPCGContext* InContext, const FString& GrammarString);
	
	TMap<FString, NFA> ConstructedNFAs;

	static FString StdToFString(const std::string& String);
	static std::string FStringToStd(const FString& String);
};
