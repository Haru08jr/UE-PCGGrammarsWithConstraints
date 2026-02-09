// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "Elements/PCGSplitPoints.h"
#include "Elements/Grammar/PCGSubdivisionBase.h"
#include "Metadata/PCGMetadata.h"
#include "Metadata/PCGMetadataAttributeTpl.h"

#include "PCGConstrainGrammar.generated.h"

class UPCGSplineData;
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
	
	/** The type of subdivision node that will be used. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings", meta = ( PCG_Overridable))
	TEnumAsByte<SubdivisionType> SubdivisionType = Spline;
	
	/** Subdivision direction in point local space. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings", meta = (EditCondition = "SubdivisionType == SubdivisionType::Segment", EditConditionHides, PCG_Overridable))
	EPCGSplitAxis SubdivisionAxis = EPCGSplitAxis::X;
	
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
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Constraints", meta = (PCG_Overridable))
	bool bConstraintsAsInput = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Constraints", meta = (EditCondition = " !bConstraintsAsInput", EditConditionHides, PCG_Overridable))
	TArray<FPCGGrammarConstraint> Constraints;
	
	/** An encoded string that represents how to apply a set of rules to a series of defined modules. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings", meta = (ShowOnlyInnerProperties, PCG_Overridable))
	FPCGGrammarSelection GrammarSelection;
	
	/** Name of the grammar output attribute. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Output", meta = (PCG_Overridable))
	FName GrammarAttributeName = TEXT("Grammar");
	
};

class FPCGConstrainGrammarElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* InContext) const override;
	
	FString ConstrainGrammar(const FString& GrammarString, float Length, const PCGSubdivisionBase::FModuleInfoMap& Modules, const TArray<FPCGGrammarConstraint> Constraints) const;
	
	float GetSegmentLength(const UPCGBasePointData* SegmentData, int SegmentIndex, EPCGSplitAxis SubdivisionAxis) const;
	
	// Accessing inputs
	
	FString GetGrammarString(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings) const;
	
	TArray<FPCGGrammarConstraint> GetConstraintsOnSpline(const UPCGPolyLineData* SplineData, const UPCGBasePointData* ConstraintPointData) const;
	TArray<FPCGGrammarConstraint> GetConstraintsOnSegment(const UPCGBasePointData* SegmentData, int SegmentIndex, const UPCGBasePointData* ConstraintPointData) const;
	
	PCGSubdivisionBase::FModuleInfoMap GetModulesInfoMap(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings, const UPCGParamData*& OutModuleInfoParamData) const;
	// Copied from PCGSubdivisionBase (can't inherit because functions are not exported)
	PCGSubdivisionBase::FModuleInfoMap GetModulesInfoMap(FPCGContext* InContext, const TArray<FPCGSubdivisionSubmodule>& SubmodulesInfo, const UPCGParamData*& OutModuleInfoParamData) const;
	PCGSubdivisionBase::FModuleInfoMap GetModulesInfoMap(FPCGContext* InContext, const FPCGSubdivisionModuleAttributeNames& InSubdivisionModuleAttributeNames, const UPCGParamData*& OutModuleInfoParamData) const;
	
private:
	static UPCGSplineData* CopySplineDataToOutput(FPCGContext* InContext, FPCGTaggedData& OutputData, const UPCGSplineData* InSplineData);
	static UPCGBasePointData* CopyPointDataToOutput(FPCGContext* InContext, FPCGTaggedData& OutputData, const UPCGBasePointData* InPointData);
	
	// Accessing & adding attributes
	
	template<typename T>
	FPCGMetadataAttribute<T>* CreateAndValidateAttribute(FPCGContext* InContext, TObjectPtr<UPCGMetadata>& Metadata, const FName AttributeName, const T DefaultValue) const;
	
	
};

template <typename T>
FPCGMetadataAttribute<T>* FPCGConstrainGrammarElement::CreateAndValidateAttribute(FPCGContext* InContext, TObjectPtr<UPCGMetadata>& Metadata, const FName AttributeName, const T DefaultValue) const
{	
	auto OutAttribute = Metadata->FindOrCreateAttribute<T>(AttributeName, DefaultValue, false, true);
	if (!OutAttribute)
	{
		PCGLog::Metadata::LogFailToCreateAttributeError<T>(AttributeName, InContext);
	}
	return OutAttribute;
}
