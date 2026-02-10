// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "Elements/PCGSplitPoints.h"
#include "Elements/Grammar/PCGSubdivisionBase.h"
#include "Metadata/PCGMetadata.h"
#include "Metadata/PCGMetadataAttributeTpl.h"
#include "Metadata/Accessors/PCGAttributeAccessorHelpers.h"

#include "PCGConstrainGrammar.generated.h"

struct FPCGSplineStruct;
class UPCGSplineData;
class UPCGPolyLineData;

USTRUCT(BlueprintType)
struct FPCGGrammarConstraint
{
	GENERATED_BODY()
	/** Symbol in the grammar. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "")
	FText Symbol;
	
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

USTRUCT(BlueprintType)
struct FPCGGrammarConstraintAttributeNames
{
	GENERATED_BODY()
	
	/** Mandatory. Expected type: FName. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "")
	FName SymbolAttributeName = PCGSubdivisionBase::Constants::SymbolAttributeName;
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
	
	/** Set to true to pass the info as attribute set. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	bool bModuleInfoAsInput = false;

	/** Fixed array of modules used for the subdivision. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings", meta = (EditCondition = "!bModuleInfoAsInput", EditConditionHides, DisplayAfter = bModuleInfoAsInput))
	TArray<FPCGSubdivisionSubmodule> ModulesInfo;

	/** Fixed array of modules used for the subdivision. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings", meta = (EditCondition = "bModuleInfoAsInput", EditConditionHides, DisplayAfter = bModuleInfoAsInput, DisplayName = "Attribute Names for Module Info"))
	FPCGSubdivisionModuleAttributeNames ModulesInfoAttributeNames;
	
	/** If true, takes in the constraint positions as points. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings", meta = (PCG_Overridable))
	bool bConstraintsAsInput = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings", meta = (EditCondition = "bConstraintsAsInput", EditConditionHides, PCG_Overridable))
	FPCGGrammarConstraintAttributeNames ConstraintAttributeNames;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings", meta = (EditCondition = "!bConstraintsAsInput", EditConditionHides, PCG_Overridable))
	TArray<FPCGGrammarConstraint> Constraints;
	
	/** An encoded string that represents how to apply a set of rules to a series of defined modules. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings", meta = (ShowOnlyInnerProperties, PCG_Overridable))
	FPCGGrammarSelection GrammarSelection;
	
	/** Name of the grammar output attribute. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings", meta = (PCG_Overridable))
	FName OutGrammarAttribute = TEXT("Grammar");
	
};

class FPCGConstrainGrammarElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* InContext) const override;
	
	FString ConstrainGrammar(const FString& GrammarString, float Length, const PCGSubdivisionBase::FModuleInfoMap& Modules, const TArray<FPCGGrammarConstraint> Constraints) const;

	static float GetSegmentLength(const UPCGBasePointData* SegmentData, int SegmentIndex, EPCGSplitAxis SubdivisionAxis);

	static TArray<FPCGGrammarConstraint> GetConstraintsOnSpline(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings, const UPCGSplineData* SplineData, const UPCGBasePointData* ConstraintPointData);
	static TArray<FPCGGrammarConstraint> GetConstraintsOnSegment(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings, const UPCGBasePointData* SegmentData, int SegmentIndex, const UPCGBasePointData* ConstraintPointData);
	
private:
	/** Calculates the approximate distance along the spline. Accuracy increases with the amount of iterations. */
	static float GetDistanceAlongSpline(const FPCGSplineStruct& Spline, const FVector& WorldPosition, int Iterations = 10);
	
	/** Get the position on the spline that's closest to Transform. Returns true if this position is inside Bounds. */
	static bool SamplePointOnSpline(const FPCGSplineStruct& Spline, const FTransform& Transform, const FBox& Bounds, FVector& OutPosition);
	
	/** Read Module infos if necessary and put them into a map */
	PCGSubdivisionBase::FModuleInfoMap GetModulesInfoMap(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings, const UPCGParamData*& OutModuleInfoParamData) const;
	// Copied from PCGSubdivisionBase (can't inherit because functions are not exported)
	PCGSubdivisionBase::FModuleInfoMap GetModulesInfoMap(FPCGContext* InContext, const TArray<FPCGSubdivisionSubmodule>& SubmodulesInfo, const UPCGParamData*& OutModuleInfoParamData) const;
	PCGSubdivisionBase::FModuleInfoMap GetModulesInfoMap(FPCGContext* InContext, const FPCGSubdivisionModuleAttributeNames& InSubdivisionModuleAttributeNames, const UPCGParamData*& OutModuleInfoParamData) const;
	
	/** Create a new attribute in Metadata and fill all entries with DefaultValue. */
	template<typename T>
	FPCGMetadataAttribute<T>* CreateAndValidateAttribute(FPCGContext* InContext, TObjectPtr<UPCGMetadata>& Metadata, const FName AttributeName, const T DefaultValue) const;
	
	/** Read the values of an attribute in InData for NumValues entries. */
	template<typename T>
	static void ReadAttributeValues(FPCGContext* InContext, const UPCGData* InData, const FName& AttributeName, int NumValues, TArray<T>& OutValues);
	
	/** Read the values of an attribute in InData for NumValues entries. */
	template<typename T>
	static void ReadAttributeValues(FPCGContext* InContext, const UPCGData* InData, const FPCGAttributePropertyInputSelector& Attribute, int NumValues, TArray<T>& OutValues);
	
	/** Returns vector element X when Axis is X, and so on. */
	template<typename T>
	static T GetVectorComponent(const UE::Math::TVector<T>& Vector, EPCGSplitAxis Axis);
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

template <typename T>
void FPCGConstrainGrammarElement::ReadAttributeValues(FPCGContext* InContext, const UPCGData* InData, const FName& AttributeName, int NumValues, TArray<T>& OutValues)
{
	FPCGAttributePropertyInputSelector Selector;
	Selector.SetAttributeName(AttributeName);
	ReadAttributeValues<T>(InContext, InData, Selector, NumValues, OutValues);
}

template <typename T>
void FPCGConstrainGrammarElement::ReadAttributeValues(FPCGContext* InContext, const UPCGData* InData, const FPCGAttributePropertyInputSelector& Attribute, int NumValues, TArray<T>& OutValues)
{
	const FPCGAttributePropertyInputSelector Selector = Attribute.CopyAndFixLast(InData);
	const TUniquePtr<const IPCGAttributeAccessor> Accessor = PCGAttributeAccessorHelpers::CreateConstAccessor(InData, Selector);
	const TUniquePtr<const IPCGAttributeAccessorKeys> AccessorKeys = PCGAttributeAccessorHelpers::CreateConstKeys(InData, Selector);
	if (!Accessor || !AccessorKeys)
	{
		PCGLog::Metadata::LogFailToCreateAccessorError(Selector, InContext);
	}
	
	OutValues.Empty();
	OutValues.AddDefaulted(NumValues);
	TArrayView<FString> Values(OutValues);
	
	if (!Accessor->GetRange(Values, 0, *AccessorKeys, EPCGAttributeAccessorFlags::AllowBroadcastAndConstructible))
	{
		PCGLog::Metadata::LogFailToGetAttributeError<int32>(Selector, Accessor.Get(), InContext);
	}
}

template <typename T>
T FPCGConstrainGrammarElement::GetVectorComponent(const UE::Math::TVector<T>& Vector, EPCGSplitAxis Axis)
{
	switch (Axis)
	{
	case EPCGSplitAxis::X: return Vector.X;
	case EPCGSplitAxis::Y: return Vector.Y;
	case EPCGSplitAxis::Z: return Vector.Z;
	default: return 0.0f;
	}
}
