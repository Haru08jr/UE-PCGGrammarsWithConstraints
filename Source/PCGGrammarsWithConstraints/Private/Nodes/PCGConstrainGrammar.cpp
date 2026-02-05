// Fill out your copyright notice in the Description page of Project Settings.


#include "Nodes/PCGConstrainGrammar.h"

#include "PCGParamData.h"
#include "Data/PCGBasePointData.h"
#include "Data/PCGSplineData.h"
#include "Helpers/PCGPropertyHelpers.h"

TArray<FPCGPinProperties> UPCGConstrainGrammarSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;

	if (bShapeAsInput)
	{
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
	}

	if (bConstraintsAsInput)
	{
		FPCGPinProperties& InConstraintsPin = PinProperties.Emplace_GetRef(PCGConstrainGrammar::Constants::ConstraintsPinLabel, EPCGDataType::Point,
		                                                                   false, true, FText::FromString("The Constraints to apply to the Grammar."));
		InConstraintsPin.SetRequiredPin();
	}

	if (bModuleInfoAsInput)
	{
		FPCGPinProperties& ModuleInfoPin = PinProperties.Emplace_GetRef(PCGSubdivisionBase::Constants::ModulesInfoPinLabel, EPCGDataType::Param);
		ModuleInfoPin.SetRequiredPin();
	}

	return PinProperties;
}

TArray<FPCGPinProperties> UPCGConstrainGrammarSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(TEXT("OutGrammar"), EPCGDataType::Param);
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

	if (!Settings->bShapeAsInput)
	{
		auto ConstrainedSting = ConstrainGrammar(Settings->GrammarSelection.GrammarString, Settings->Length, ModulesInfo, Settings->Constraints);

		// TODO output, I don't want to think about this now
	}
	else
	{
		if (Settings->SubdivisionType == Spline)
		{
			// TODO stuff
		}
		else if (Settings->SubdivisionType == Segment)
		{
			// TODO stuff
		}
		else
		{
			// TODO error
		}
	}

	return false;
}

FString FPCGConstrainGrammarElement::ConstrainGrammar(const FString& GrammarString, float Length, const PCGSubdivisionBase::FModuleInfoMap& Modules, const TArray<FPCGGrammarConstraint> Constraints) const
{
	// TODO use implementation
	return GrammarString;
}

FString FPCGConstrainGrammarElement::GetGrammarString(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings) const
{
	if (!InSettings->GrammarSelection.bGrammarAsAttribute)
	{
		return InSettings->GrammarSelection.GrammarString;
	}

	//TODO other stuff
	return "";
}

TArray<FPCGGrammarConstraint> FPCGConstrainGrammarElement::GetConstraintsOnSpline(const UPCGConstrainGrammarSettings* InSettings) const
{
	return {};
}

TArray<FPCGGrammarConstraint> FPCGConstrainGrammarElement::GetConstraintsOnSegment(const UPCGConstrainGrammarSettings* InSettings) const
{
	return {};
}

/*
float FPCGConstrainGrammarElement::GetShapeLength(const FPCGContext* InContext) const
{
	const UPCGConstrainGrammarSettings* Settings = InContext->GetInputSettings<UPCGConstrainGrammarSettings>();

	if (!Settings->bShapeAsInput)
		return Settings->Length;

	const TArray<FPCGTaggedData> InputShapes = InContext->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);
	if (!Settings->SubdivisionType == Spline)
	{
		const UPCGPolyLineData* InputSplineData = Cast<const UPCGPolyLineData>(InputShapes[0].Data);
		return InputSplineData->GetLength();
	}

	if (!Settings->SubdivisionType == Segment)
	{
		const UPCGBasePointData* InputSegmentData = Cast<const UPCGBasePointData>(InputShapes[0].Data);
		auto Bounds = InputSegmentData->GetBoundsMax(0) - InputSegmentData->GetBoundsMin(0);

		switch (Settings->SubdivisionAxis)
		{
		case EPCGSplitAxis::X: return Bounds.X;
		case EPCGSplitAxis::Y: return Bounds.Y;
		case EPCGSplitAxis::Z: return Bounds.Z;
		}
	}

	// TODO error message
	return 0.0;
}*/

PCGSubdivisionBase::FModuleInfoMap FPCGConstrainGrammarElement::GetModulesInfoMap(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings,
                                                                               const UPCGParamData*& OutModuleInfoParamData) const
{
	if (InSettings->bModuleInfoAsInput)
	{
		return GetModulesInfoMap(InContext, InSettings->ModulesInfoAttributeNames, OutModuleInfoParamData);
	}
	else
	{
		return GetModulesInfoMap(InContext, InSettings->ModulesInfo, OutModuleInfoParamData);
	}
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
	PropertyNameMapping.Emplace(GET_MEMBER_NAME_CHECKED(FPCGSubdivisionSubmodule, bScalable), {InSubdivisionModuleAttributeNames.ScalableAttributeName, /*bCanBeDefaulted=*/!InSubdivisionModuleAttributeNames.bProvideScalable});
	PropertyNameMapping.Emplace(GET_MEMBER_NAME_CHECKED(FPCGSubdivisionSubmodule, DebugColor), {InSubdivisionModuleAttributeNames.DebugColorAttributeName, /*bCanBeDefaulted=*/!InSubdivisionModuleAttributeNames.bProvideDebugColor});

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
