// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "Elements/PCGSplitPoints.h"
#include "Elements/Grammar/PCGSubdivisionBase.h"

#include "PCGConstrainGrammar.generated.h"

class UPCGPolyLineData;

USTRUCT(BlueprintType)
struct FPCGGrammarConstraint
{
	GENERATED_BODY()
	/** Symbol in the grammar. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "")
	FName Symbol = NAME_None;
	
	/** Position along the generation shape. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "")
	double Position = 100.0;
	
	/** Position along the generation shape. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "")
	bool bHasWidth = false;
	
	/** Position along the generation shape. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "", meta=(EditCondition = "bHasWidth"))
	double Width = 100.0;
	
};

UENUM()
enum SubdivisionType
{
	Spline,
	Segment
};

namespace PCGConstrainGrammar::Constants
{
	const FName ConstraintsPinLabel = TEXT("Constraints");
	const FName OutGrammarPinLabel = TEXT("OutGrammar");
	
	static const FText DuplicatedSymbolText = FText::FromString("Symbol {0} is duplicated, ignored.");
}

UCLASS(BlueprintType, ClassGroup = (Procedural))
class PCGGRAMMARSWITHCONSTRAINTS_API UPCGConstrainGrammarSettings : public UPCGSettings
{
	GENERATED_BODY()
	
public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("ConstrainGrammar")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGConstrainGrammarElement", "NodeTitle", "Constrain Grammar"); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
#endif // WITH_EDITOR
	
protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface
	
public:	
	/** If true, takes in a shape input along which the grammar will be generated. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|InputShape", meta = (PCG_Overridable))
	bool bShapeAsInput = true;
	
	/** The type of subdivision node that will be used. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|InputShape", meta = (EditCondition = "bShapeAsInput", EditConditionHides, PCG_Overridable))
	TEnumAsByte<SubdivisionType> SubdivisionType = Spline;
	
	/** Subdivision direction in point local space. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|InputShape", meta = (EditCondition = "bShapeAsInput && (SubdivisionType == SubdivisionType::Segment)", EditConditionHides, PCG_Overridable))
	EPCGSplitAxis SubdivisionAxis = EPCGSplitAxis::X;
	
	/** The length of the shape along which the grammar will be generated. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|InputShape", meta = (EditCondition = "!bShapeAsInput", EditConditionHides, PCG_Overridable))
	float Length = 0.0;
	
	/** Set it to true to pass the info as attribute set. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Modules")
	bool bModuleInfoAsInput = false;

	/** Fixed array of modules used for the subdivision. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Modules", meta = (EditCondition = "!bModuleInfoAsInput", EditConditionHides, DisplayAfter = bModuleInfoAsInput))
	TArray<FPCGSubdivisionSubmodule> ModulesInfo;

	/** Fixed array of modules used for the subdivision. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Modules", meta = (EditCondition = "bModuleInfoAsInput", EditConditionHides, DisplayAfter = bModuleInfoAsInput, DisplayName = "Attribute Names for Module Info"))
	FPCGSubdivisionModuleAttributeNames ModulesInfoAttributeNames;
	
	/** If true, takes in the constraint positions as points. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Constraints", meta = (EditCondition = "bShapeAsInput", EditConditionHides, PCG_Overridable))
	bool bConstraintsAsInput = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Constraints", meta = (EditCondition = "!bShapeAsInput || !bConstraintsAsInput", EditConditionHides, PCG_Overridable))
	TArray<FPCGGrammarConstraint> Constraints;
	
	/** An encoded string that represents how to apply a set of rules to a series of defined modules. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Grammar", meta = (ShowOnlyInnerProperties, PCG_Overridable))
	FPCGGrammarSelection GrammarSelection;
	
	// Grammar Output Attribute Name
	
};

class FPCGConstrainGrammarElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* InContext) const override;
	
	FString ConstrainGrammar(const FString& GrammarString, float Length, const PCGSubdivisionBase::FModuleInfoMap& Modules, const TArray<FPCGGrammarConstraint> Constraints) const;
	
	//float GetShapeLength(const FPCGContext* InContext) const;
	float GetSegmentLength(const UPCGBasePointData* SegmentData, int SegmentIndex, EPCGSplitAxis SubdivisionAxis) const;
	
	// Access inputs
	
	FString GetGrammarString(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings) const;
	
	TArray<FPCGGrammarConstraint> GetConstraintsOnSpline(const UPCGPolyLineData* SplineData, const UPCGBasePointData* ConstraintPointData) const;
	TArray<FPCGGrammarConstraint> GetConstraintsOnSegment(const UPCGBasePointData* SegmentData, int SegmentIndex, const UPCGBasePointData* ConstraintPointData) const;
	
	PCGSubdivisionBase::FModuleInfoMap GetModulesInfoMap(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings, const UPCGParamData*& OutModuleInfoParamData) const;
	// Copied from PCGSubdivisionBase (can't inherit because functions are not exported)
	PCGSubdivisionBase::FModuleInfoMap GetModulesInfoMap(FPCGContext* InContext, const TArray<FPCGSubdivisionSubmodule>& SubmodulesInfo, const UPCGParamData*& OutModuleInfoParamData) const;
	PCGSubdivisionBase::FModuleInfoMap GetModulesInfoMap(FPCGContext* InContext, const FPCGSubdivisionModuleAttributeNames& InSubdivisionModuleAttributeNames, const UPCGParamData*& OutModuleInfoParamData) const;
	
	
};
