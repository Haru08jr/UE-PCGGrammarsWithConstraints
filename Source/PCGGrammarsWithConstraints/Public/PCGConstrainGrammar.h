// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>

#include "CoreMinimal.h"
#include "PCGConstrainGrammarStructs.h"
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

	/** Set to true to pass the module info as attribute set. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Modules")
	bool bModuleInfoAsInput = false;

	/** Fixed array of modules used for the subdivision. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Modules", meta = (EditCondition = "!bModuleInfoAsInput", EditConditionHides, DisplayAfter = bModuleInfoAsInput))
	TArray<FPCGConstrainedGrammarModule> ModulesInfo;

	/** Names of the module attributes. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Modules",
		meta = (EditCondition = "bModuleInfoAsInput", EditConditionHides, DisplayAfter = bModuleInfoAsInput, DisplayName = "Attribute Names for Module Info"))
	FPCGConstrainedGrammarModuleAttributeNames ModulesInfoAttributeNames;

	/** If true, takes in the constraint positions as points. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Constraints", meta = (PCG_Overridable))
	bool bConstraintsAsInput = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Constraints", meta = (EditCondition = "bConstraintsAsInput", EditConditionHides, PCG_Overridable))
	FPCGGrammarConstraintAttributeNames ConstraintAttributeNames;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Constraints", meta = (EditCondition = "!bConstraintsAsInput", EditConditionHides, PCG_Overridable))
	TArray<FPCGGrammarConstraint> Constraints;

	/** An encoded string that represents how to apply a set of rules to a series of defined modules. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings", meta = (ShowOnlyInnerProperties, PCG_Overridable))
	FPCGGrammarSelection GrammarSelection;

	/** Name of the grammar output attribute. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings", meta = (PCG_Overridable))
	FName OutGrammarAttribute = TEXT("Grammar");
};

class FPCGConstrainGrammarElement : public IPCGElementWithCustomContext<FPCGGrammarConstrainingContext>
{
protected:
	virtual bool ExecuteInternal(FPCGContext* InContext) const override;

private:
	// Grammar constraining
	static FString GenerateWithConstraints(FPCGGrammarConstrainingContext* Context, const FString& GrammarString, float Length, const TArray<FPCGGrammarConstraint>& Constraints);
	
	/** 
	 * If the context does not have a NFA for the given grammar yet, construct one and save it.
	 * Returns true if the NFA already existed or was created, and false if the creation of the NFA failed.
	 */
	static bool MakeNFAForGrammar(FPCGGrammarConstrainingContext* InContext, const FString& GrammarString);
	
	
	// Spline helpers 
	/** Maps the incoming constraint points onto the spline. */
	static TArray<FPCGGrammarConstraint> GetConstraintsOnSpline(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings, const UPCGSplineData* SplineData,
	                                                            const UPCGBasePointData* ConstraintPointData);

	/** Calculates the approximate distance along the spline. Accuracy increases with the amount of iterations. */
	static float GetDistanceAlongSpline(const FPCGSplineStruct& Spline, const FVector& WorldPosition, int Iterations = 10);

	/** Get the position on the spline that's closest to Transform. Returns true if this position is inside Bounds. */
	static bool SamplePointOnSpline(const FPCGSplineStruct& Spline, const FTransform& Transform, const FBox& Bounds, FVector& OutPosition);

	// Segment helpers
	/** Maps the incoming constraint points onto the segment. */
	static TArray<FPCGGrammarConstraint> GetConstraintsOnSegment(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings, const UPCGBasePointData* SegmentData, int SegmentIndex,
	                                                             const UPCGBasePointData* ConstraintPointData);

	/** Calculate the length of a segment depending on the subdivision axis. */
	static float GetSegmentLength(const UPCGBasePointData* SegmentData, int SegmentIndex, EPCGSplitAxis SubdivisionAxis);

	// Attribute access helpers
	/** Read module info from input if necessary. */
	static TArray<FPCGConstrainedGrammarModule> GetModules(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings);
	
	/** Create a new attribute in Metadata and fill all entries with DefaultValue. */
	template <typename T>
	FPCGMetadataAttribute<T>* CreateOrOverwriteAttribute(FPCGContext* InContext, TObjectPtr<UPCGMetadata>& Metadata, const FName AttributeName, const T DefaultValue) const;

	/** Read the values of an attribute in InData for NumValues entries. */
	template <typename T>
	static void ReadAttributeValues(FPCGContext* InContext, const UPCGData* InData, const FName& AttributeName, int NumValues, TArray<T>& OutValues);

	/** Read the values of an attribute in InData for NumValues entries. */
	template <typename T>
	static void ReadAttributeValues(FPCGContext* InContext, const UPCGData* InData, const FPCGAttributePropertyInputSelector& Attribute, int NumValues, TArray<T>& OutValues);

	// Additional helper functions
	/** Returns vector element X when Axis is X, and so on. */
	template <typename T>
	static T GetVectorComponent(const UE::Math::TVector<T>& Vector, EPCGSplitAxis Axis);
	
	/** Conversion function from std::string to FString */ 
	static FString StdToFString(const std::string& String);
	/** Conversion function from std::string to FString */ 
	static std::string FStringToStd(const FString& String);
};

template <typename T>
FPCGMetadataAttribute<T>* FPCGConstrainGrammarElement::CreateOrOverwriteAttribute(FPCGContext* InContext, TObjectPtr<UPCGMetadata>& Metadata, const FName AttributeName, const T DefaultValue) const
{
	auto* OutAttribute = Metadata->FindOrCreateAttribute<T>(AttributeName, DefaultValue, true, true);
	if (!OutAttribute)
	{
		PCGLog::Metadata::LogFailToCreateAttributeError<T>(AttributeName, InContext);
	}
	OutAttribute->SetDefaultValue(DefaultValue);
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
