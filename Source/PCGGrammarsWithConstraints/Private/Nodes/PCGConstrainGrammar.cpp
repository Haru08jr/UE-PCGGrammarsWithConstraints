// Fill out your copyright notice in the Description page of Project Settings.


#include "Nodes/PCGConstrainGrammar.h"

#include "PCGParamData.h"
#include "Data/PCGBasePointData.h"
#include "Data/PCGSplineData.h"
#include "Helpers/PCGPropertyHelpers.h"

TArray<FPCGPinProperties> UPCGConstrainGrammarSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;


	if (SubdivisionType == Spline)
	{
		FPCGPinProperties& InShapePin = PinProperties.Emplace_GetRef(PCGPinConstants::DefaultInputLabel, EPCGDataType::PolyLine, false, true,
		                                                             FText::FromString("The Spline on which the Grammar will be generated. Should match the In pin of Subdivide Spline."));
		InShapePin.SetRequiredPin();
	}
	else
	{
		FPCGPinProperties& InShapePin = PinProperties.Emplace_GetRef(PCGPinConstants::DefaultInputLabel, EPCGDataType::Point, false, true,
		                                                             FText::FromString("The Segments on which the Grammar will be generated. Should match the In pin of Subdivide Segment."));
		InShapePin.SetRequiredPin();
	}

	if (bConstraintsAsInput)
	{
		FPCGPinProperties& InConstraintsPin = PinProperties.Emplace_GetRef(PCGConstrainGrammar::Constants::ConstraintsPinLabel, EPCGDataType::Point,
		                                                                   false, true, FText::FromString("The Constraints to apply to the Grammar."));
		InConstraintsPin.SetRequiredPin();
	}

	if (bModuleInfoAsInput)
	{
		FPCGPinProperties& ModuleInfoPin = PinProperties.Emplace_GetRef(PCGSubdivisionBase::Constants::ModulesInfoPinLabel, EPCGDataType::Param,
		                                                                false, true, FText::FromString("The Modules used in the Grammar."));
		ModuleInfoPin.SetRequiredPin();
	}

	return PinProperties;
}

TArray<FPCGPinProperties> UPCGConstrainGrammarSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;

	if (SubdivisionType == Spline)
		PinProperties.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Spline, false, true);
	else
		PinProperties.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Point, false, true);

	return PinProperties;
}

FPCGElementPtr UPCGConstrainGrammarSettings::CreateElement() const
{
	return MakeShared<FPCGConstrainGrammarElement>();
}

bool FPCGConstrainGrammarElement::ExecuteInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGSubdivideSplineElement::Execute);

	const UPCGConstrainGrammarSettings* Settings = InContext->GetInputSettings<UPCGConstrainGrammarSettings>();
	check(Settings);

	TArray<FPCGTaggedData>& Outputs = InContext->OutputData.TaggedData;

	const UPCGParamData* ModuleInfoParamData = nullptr;
	const PCGSubdivisionBase::FModuleInfoMap ModulesInfo = GetModulesInfoMap(InContext, Settings, ModuleInfoParamData);

	if (Settings->SubdivisionType == Spline)
	{
		const TArray<FPCGTaggedData> SplineInputs = InContext->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);

		for (const auto& SplineInput : SplineInputs)
		{
			const UPCGSplineData* SplineData = Cast<const UPCGSplineData>(SplineInput.Data);

			FString Grammar = Settings->GrammarSelection.GrammarString;
			if (Settings->GrammarSelection.bGrammarAsAttribute)
			{
				TArray<FString> GrammarStrings;
				ReadAttributeValues(InContext, SplineData, Settings->GrammarSelection.GrammarAttribute, 1, GrammarStrings);
				Grammar = GrammarStrings[0];
			}

			if (!Settings->bConstraintsAsInput)
			{
				auto ConstrainedString = ConstrainGrammar(Grammar, SplineData->GetLength(), ModulesInfo, Settings->Constraints);

				auto OutSplineData = SplineData->DuplicateData(InContext);
				Outputs.Emplace_GetRef().Data = OutSplineData;
				CreateAndValidateAttribute(InContext, OutSplineData->Metadata, Settings->OutGrammarAttribute, ConstrainedString);
			}
			else
			{
				TArray<FPCGTaggedData> ConstraintInputs = InContext->InputData.GetInputsByPin(PCGConstrainGrammar::Constants::ConstraintsPinLabel);
				for (const auto& ConstraintInput : ConstraintInputs)
				{
					const UPCGBasePointData* ConstraintPointData = Cast<const UPCGBasePointData>(ConstraintInput.Data);
					auto Constraints = GetConstraintsOnSpline(InContext, Settings, SplineData, ConstraintPointData);

					auto ConstrainedString = ConstrainGrammar(Grammar, SplineData->GetLength(), ModulesInfo, Constraints);

					auto OutSplineData = SplineData->DuplicateData(InContext);
					Outputs.Emplace_GetRef().Data = OutSplineData;
					CreateAndValidateAttribute(InContext, OutSplineData->Metadata, Settings->OutGrammarAttribute, ConstrainedString);
				}
			}
		}
	}
	else if (Settings->SubdivisionType == Segment)
	{
		const TArray<FPCGTaggedData> SegmentInputs = InContext->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);

		for (auto SegmentInput : SegmentInputs)
		{
			const UPCGBasePointData* SegmentData = Cast<const UPCGBasePointData>(SegmentInput.Data);

			TArray<FString> GrammarStrings;
			ReadAttributeValues(InContext, SegmentData, Settings->GrammarSelection.GrammarAttribute, SegmentData->GetNumPoints(), GrammarStrings);

			if (!Settings->bConstraintsAsInput)
			{
				//auto OutSegmentData = CopyPointDataToOutput(InContext, Outputs.Emplace_GetRef(), SegmentData);
				auto OutSegmentData = Cast<UPCGBasePointData>(SegmentData->DuplicateData(InContext));
				Outputs.Emplace_GetRef().Data = OutSegmentData;
				FPCGMetadataAttribute<FString>* GrammarAttribute = CreateAndValidateAttribute<FString>(InContext, OutSegmentData->Metadata, Settings->OutGrammarAttribute, "");

				for (int i = 0; i < OutSegmentData->GetNumPoints(); ++i)
				{
					auto Grammar = Settings->GrammarSelection.bGrammarAsAttribute ? GrammarStrings[i] : Settings->GrammarSelection.GrammarString;

					auto ConstrainedString = ConstrainGrammar(Grammar, GetSegmentLength(SegmentData, i, Settings->SubdivisionAxis), ModulesInfo, Settings->Constraints);
					GrammarAttribute->SetValue<FString>(OutSegmentData->GetMetadataEntry(i), ConstrainedString);
				}
			}
			else
			{
				TArray<FPCGTaggedData> ConstraintInputs = InContext->InputData.GetInputsByPin(PCGConstrainGrammar::Constants::ConstraintsPinLabel);
				for (auto ConstraintInput : ConstraintInputs)
				{
					const UPCGBasePointData* ConstraintPointData = Cast<const UPCGBasePointData>(ConstraintInput.Data);

					//auto OutSegmentData = CopyPointDataToOutput(InContext, Outputs.Emplace_GetRef(), SegmentData);
					auto OutSegmentData = Cast<UPCGBasePointData>(SegmentData->DuplicateData(InContext));
					Outputs.Emplace_GetRef().Data = OutSegmentData;
					FPCGMetadataAttribute<FString>* GrammarAttribute = CreateAndValidateAttribute<FString>(InContext, OutSegmentData->Metadata, Settings->OutGrammarAttribute, "");

					for (int i = 0; i < OutSegmentData->GetNumPoints(); ++i)
					{
						auto Constraints = GetConstraintsOnSegment(InContext, Settings, SegmentData, i, ConstraintPointData);

						auto Grammar = Settings->GrammarSelection.bGrammarAsAttribute ? GrammarStrings[i] : Settings->GrammarSelection.GrammarString;
						auto ConstrainedString = ConstrainGrammar(Grammar, GetSegmentLength(SegmentData, i, Settings->SubdivisionAxis), ModulesInfo, Constraints);
						GrammarAttribute->SetValue<FString>(OutSegmentData->GetMetadataEntry(i), ConstrainedString);
					}
				}
			}
		}
	}
	return true;
}

FString FPCGConstrainGrammarElement::ConstrainGrammar(const FString& GrammarString, float Length, const PCGSubdivisionBase::FModuleInfoMap& Modules,
                                                      const TArray<FPCGGrammarConstraint> Constraints) const
{
	// TODO use implementation
	return GrammarString;
}

float FPCGConstrainGrammarElement::GetSegmentLength(const UPCGBasePointData* SegmentData, int SegmentIndex, EPCGSplitAxis SubdivisionAxis)
{
	auto Bounds = SegmentData->GetLocalBounds(SegmentIndex);
	return GetVectorComponent(Bounds.GetSize(), SubdivisionAxis);
}

TArray<FPCGGrammarConstraint> FPCGConstrainGrammarElement::GetConstraintsOnSpline(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings, const UPCGSplineData* SplineData,
                                                                                  const UPCGBasePointData* ConstraintPointData)
{
	TArray<FPCGGrammarConstraint> Constraints;

	TArray<FString> Symbols;
	ReadAttributeValues(InContext, ConstraintPointData, InSettings->ConstraintAttributeNames.SymbolAttributeName, ConstraintPointData->GetNumPoints(), Symbols);

	for (int i = 0; i < ConstraintPointData->GetNumPoints(); i++)
	{
		FVector PositionOnSpline;
		if (!SamplePointOnSpline(SplineData->SplineStruct, ConstraintPointData->GetTransform(i), FBox(ConstraintPointData->GetBoundsMin(i), ConstraintPointData->GetBoundsMax(i)), PositionOnSpline))
			continue;

		auto Distance = GetDistanceAlongSpline(SplineData->SplineStruct, PositionOnSpline);
		if (Distance < 0.0f || Distance > SplineData->GetLength())
			continue;

		Constraints.Emplace(FText::FromString(Symbols[i]), Distance, false, 0.0);
	}

	return Constraints;
}

float FPCGConstrainGrammarElement::GetDistanceAlongSpline(const FPCGSplineStruct& Spline, const FVector& WorldPosition, int Iterations)
{
	const auto ClosestSplinePointKey = Spline.FindInputKeyClosestToWorldLocation(WorldPosition);

	auto LowerBound = Spline.GetDistanceAlongSplineAtSplinePoint(ClosestSplinePointKey);
	auto HigherBound = Spline.GetDistanceAlongSplineAtSplinePoint(ClosestSplinePointKey + 1);

	auto DistanceEstimate = (LowerBound + HigherBound) * 0.5f;

	// TODO check if this actually works
	for (auto i = 0; i < Iterations; ++i)
	{
		auto MiddleInputKey = Spline.GetInputKeyAtDistanceAlongSpline(DistanceEstimate);

		if (ClosestSplinePointKey < MiddleInputKey)
			HigherBound = DistanceEstimate;
		else
			LowerBound = DistanceEstimate;

		DistanceEstimate = (LowerBound + HigherBound) * 0.5f;
	}

	return DistanceEstimate;
}

bool FPCGConstrainGrammarElement::SamplePointOnSpline(const FPCGSplineStruct& Spline, const FTransform& Transform, const FBox& Bounds, FVector& OutPosition)
{
	const FVector InPosition = Transform.GetLocation();
	float NearestPointKey = Spline.FindInputKeyClosestToWorldLocation(InPosition);
	FTransform NearestTransform = Spline.GetTransformAtSplineInputKey(NearestPointKey, ESplineCoordinateSpace::World, true);

	OutPosition = NearestTransform.GetLocation();

	FVector LocalPoint = NearestTransform.InverseTransformPosition(InPosition);
	if (Bounds.IsInside(LocalPoint))
		return true;
	return false;
}

TArray<FPCGGrammarConstraint> FPCGConstrainGrammarElement::GetConstraintsOnSegment(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings, const UPCGBasePointData* SegmentData,
                                                                                   int SegmentIndex, const UPCGBasePointData* ConstraintPointData)
{
	TArray<FPCGGrammarConstraint> Constraints;

	TArray<FString> Symbols;
	ReadAttributeValues(InContext, ConstraintPointData, InSettings->ConstraintAttributeNames.SymbolAttributeName, ConstraintPointData->GetNumPoints(), Symbols);

	const auto SegmentTransform = SegmentData->GetTransform(SegmentIndex);
	const auto SegmentBounds = SegmentData->GetLocalBounds(SegmentIndex).TransformBy(SegmentTransform);

	for (int i = 0; i < ConstraintPointData->GetNumPoints(); i++)
	{
		const auto ConstraintTransform = ConstraintPointData->GetTransform(i);
		const auto ConstraintBounds = ConstraintPointData->GetLocalBounds(i).TransformBy(ConstraintTransform);

		if (!SegmentBounds.Intersect(ConstraintBounds))
			continue;

		auto Distances = SegmentBounds.GetClosestPointTo(ConstraintTransform.GetLocation()) - SegmentBounds.Min;
		float Distance = GetVectorComponent(Distances, InSettings->SubdivisionAxis);

		Constraints.Emplace(FText::FromString(Symbols[i]), Distance, false, 0.0);
	}

	return Constraints;
}

PCGSubdivisionBase::FModuleInfoMap FPCGConstrainGrammarElement::GetModulesInfoMap(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings,
                                                                                  const UPCGParamData*& OutModuleInfoParamData) const
{
	if (InSettings->bModuleInfoAsInput)
		return GetModulesInfoMap(InContext, InSettings->ModulesInfoAttributeNames, OutModuleInfoParamData);
	else
		return GetModulesInfoMap(InContext, InSettings->ModulesInfo, OutModuleInfoParamData);
}

PCGSubdivisionBase::FModuleInfoMap FPCGConstrainGrammarElement::GetModulesInfoMap(FPCGContext* InContext, const TArray<FPCGSubdivisionSubmodule>& SubmodulesInfo,
                                                                                  const UPCGParamData*& OutModuleInfoParamData) const
{
	PCGSubdivisionBase::FModuleInfoMap ModulesInfo;
	OutModuleInfoParamData = nullptr;

	ModulesInfo.Reserve(SubmodulesInfo.Num());
	for (const FPCGSubdivisionSubmodule& SubdivisionModule : SubmodulesInfo)
	{
		if (ModulesInfo.Contains(SubdivisionModule.Symbol))
		{
			PCGLog::LogWarningOnGraph(FText::Format(PCGConstrainGrammar::Constants::DuplicatedSymbolText, FText::FromName(SubdivisionModule.Symbol)), InContext);
			continue;
		}

		ModulesInfo.Emplace(SubdivisionModule.Symbol, SubdivisionModule);
	}

	return ModulesInfo;
}

PCGSubdivisionBase::FModuleInfoMap FPCGConstrainGrammarElement::GetModulesInfoMap(FPCGContext* InContext, const FPCGSubdivisionModuleAttributeNames& InSubdivisionModuleAttributeNames,
                                                                                  const UPCGParamData*& OutModuleInfoParamData) const
{
	PCGSubdivisionBase::FModuleInfoMap ModulesInfo;
	OutModuleInfoParamData = nullptr;

	const TArray<FPCGTaggedData> ModulesInfoInputs = InContext->InputData.GetInputsByPin(PCGSubdivisionBase::Constants::ModulesInfoPinLabel);

	if (ModulesInfoInputs.IsEmpty())
	{
		PCGLog::LogWarningOnGraph(FText::FromString("No data was found on the module info pin."), InContext);
		return ModulesInfo;
	}

	const UPCGParamData* ParamData = Cast<UPCGParamData>(ModulesInfoInputs[0].Data);
	if (!ParamData)
	{
		PCGLog::LogWarningOnGraph(FText::FromString("Module info input is not of type attribute set."), InContext);
		return ModulesInfo;
	}

	TMap<FName, TTuple<FName, bool>> PropertyNameMapping;
	PropertyNameMapping.Emplace(GET_MEMBER_NAME_CHECKED(FPCGSubdivisionSubmodule, Symbol), {InSubdivisionModuleAttributeNames.SymbolAttributeName, /*bCanBeDefaulted=*/false});
	PropertyNameMapping.Emplace(GET_MEMBER_NAME_CHECKED(FPCGSubdivisionSubmodule, Size), {InSubdivisionModuleAttributeNames.SizeAttributeName, /*bCanBeDefaulted=*/false});
	PropertyNameMapping.Emplace(GET_MEMBER_NAME_CHECKED(FPCGSubdivisionSubmodule, bScalable), {
		                            InSubdivisionModuleAttributeNames.ScalableAttributeName, /*bCanBeDefaulted=*/!InSubdivisionModuleAttributeNames.bProvideScalable
	                            });
	PropertyNameMapping.Emplace(GET_MEMBER_NAME_CHECKED(FPCGSubdivisionSubmodule, DebugColor), {
		                            InSubdivisionModuleAttributeNames.DebugColorAttributeName, /*bCanBeDefaulted=*/!InSubdivisionModuleAttributeNames.bProvideDebugColor
	                            });

	const TArray<FPCGSubdivisionSubmodule> AllModules = PCGPropertyHelpers::ExtractAttributeSetAsArrayOfStructs<FPCGSubdivisionSubmodule>(ParamData, &PropertyNameMapping, InContext);

	ModulesInfo.Reserve(AllModules.Num());

	for (int32 i = 0; i < AllModules.Num(); ++i)
	{
		if (ModulesInfo.Contains(AllModules[i].Symbol))
		{
			PCGLog::LogWarningOnGraph(FText::Format(PCGConstrainGrammar::Constants::DuplicatedSymbolText, FText::FromName(AllModules[i].Symbol)), InContext);
			continue;
		}

		ModulesInfo.Emplace(AllModules[i].Symbol, std::move(AllModules[i]));
	}

	OutModuleInfoParamData = ParamData;

	return ModulesInfo;
}
