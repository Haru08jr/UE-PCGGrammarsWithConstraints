// Fill out your copyright notice in the Description page of Project Settings.


#include "Nodes/PCGConstrainGrammar.h"

#include "Data/PCGBasePointData.h"
#include "Data/PCGSplineData.h"

TArray<FPCGPinProperties> UPCGConstrainGrammarSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;

	if (bShapeAsInput)
	{
		FPCGPinProperties& InShapePin = PinProperties.Emplace_GetRef(PCGPinConstants::DefaultInputLabel, SubdivisionType == Spline ? EPCGDataType::PolyLine : EPCGDataType::Point,
		                                                             false, true, FText::FromString("The Spline on which the Grammar will be generated. Should match the In pin of Subdivide Spline."));
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
	const FModuleInfoMap ModulesInfo = GetModulesInfo(InContext, Settings, ModuleInfoParamData);

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

FString FPCGConstrainGrammarElement::ConstrainGrammar(const FString& GrammarString, float Length, const FModuleInfoMap& Modules, const TArray<FPCGGrammarConstraint> Constraints) const
{
	// TODO use implementation
	return GrammarString;
}

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
}

PCGSubdivisionBase::FModuleInfoMap FPCGConstrainGrammarElement::GetModulesInfo(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings,
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

FString FPCGConstrainGrammarElement::GetGrammarString(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings) const
{
	if (!InSettings->GrammarSelection.bGrammarAsAttribute)
	{
		return InSettings->GrammarSelection.GrammarString;
	}
	
	//TODO other stuff
	return "";
}
