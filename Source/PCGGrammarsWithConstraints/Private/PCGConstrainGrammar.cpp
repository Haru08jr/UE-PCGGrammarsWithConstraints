#include "PCGConstrainGrammar.h"

#include "PCGParamData.h"
#include "Data/PCGBasePointData.h"
#include "Data/PCGSplineData.h"
#include "Helpers/PCGPropertyHelpers.h"
#include "PCGGrammarsWithConstraints/PCGConstrainedGrammarGenerator/source/public/Generator.hpp"
#include "PCGGrammarsWithConstraints/PCGConstrainedGrammarGenerator/source/public/automaton/NFACompiler.hpp"
#include "PCGGrammarsWithConstraints/PCGConstrainedGrammarGenerator/source/public/regex/RegexParser.hpp"

TArray<FPCGPinProperties> UPCGConstrainGrammarSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;

	if (SubdivisionType == Spline)
	{
		FPCGPinProperties& InShapePin = PinProperties.Emplace_GetRef(PCGPinConstants::DefaultInputLabel, EPCGDataType::Spline, false, true,
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
		                                                                   false, true, FText::FromString(
			                                                                   "The Constraints to apply to the Grammar. Should have the attributes specified in Constraint Attribute Names."));
		InConstraintsPin.SetRequiredPin();
	}

	if (bModuleInfoAsInput)
	{
		FPCGPinProperties& ModuleInfoPin = PinProperties.Emplace_GetRef(PCGSubdivisionBase::Constants::ModulesInfoPinLabel, EPCGDataType::Param,
		                                                                false, true, FText::FromString(
			                                                                "The Modules used in the Grammar. Should have the attributes specified in Module Attribute Names."));
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

	auto* Context = static_cast<FPCGGrammarConstrainingContext*>(InContext);
	check(Context);

	// Read and check modules
	auto Modules = GetModules(InContext, Settings);
	if (Modules.IsEmpty())
	{
		PCGLog::LogErrorOnGraph(FText::FromString("No modules found!"), InContext);
		return true;
	}
	for (const auto& Module : Modules)
	{
		auto symbol = FStringToStd(Module.Symbol.ToString());
		if (Context->ModuleMap.contains(symbol))
		{
			PCGLog::LogWarningOnGraph(FText::Format(PCGConstrainGrammar::Constants::DuplicatedSymbolText, FText::FromName(Module.Symbol)), InContext);
			continue;
		}
		if (Module.Size <= 0)
		{
			PCGLog::LogWarningOnGraph(FText::Format(FText::FromString("Module {0} has size 0, will be ignored."), FText::FromName(Module.Symbol)), InContext);
			continue;
		}
		Context->ModuleMap.emplace(symbol, GrammarModule{symbol, static_cast<float>(Module.Size), Module.bSpawnOnlyWithConstraint});
	}

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
				auto ConstrainedString = GenerateWithConstraints(Context, Grammar, SplineData->GetLength(), Settings->Constraints);

				// copy input data to output and add Grammar attribute
				auto OutSplineData = SplineData->DuplicateData(InContext);
				Outputs.Emplace_GetRef().Data = OutSplineData;
				CreateOrOverwriteAttribute(InContext, OutSplineData->Metadata, Settings->OutGrammarAttribute, ConstrainedString);
			}
			else
			{
				// if the constraints are provided through input pins, iterate over all constraint sets
				TArray<FPCGTaggedData> ConstraintInputs = InContext->InputData.GetInputsByPin(PCGConstrainGrammar::Constants::ConstraintsPinLabel);
				for (const auto& ConstraintInput : ConstraintInputs)
				{
					const UPCGBasePointData* ConstraintPointData = Cast<const UPCGBasePointData>(ConstraintInput.Data);
					auto Constraints = GetConstraintsOnSpline(InContext, Settings, SplineData, ConstraintPointData);

					auto ConstrainedString = GenerateWithConstraints(Context, Grammar, SplineData->GetLength(), Constraints);

					// copy input data to output and add Grammar attribute
					auto OutSplineData = SplineData->DuplicateData(InContext);
					Outputs.Emplace_GetRef().Data = OutSplineData;
					CreateOrOverwriteAttribute(InContext, OutSplineData->Metadata, Settings->OutGrammarAttribute, ConstrainedString);
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
				// copy input data to output and add Grammar attribute
				auto OutSegmentData = Cast<UPCGBasePointData>(SegmentData->DuplicateData(InContext));
				Outputs.Emplace_GetRef().Data = OutSegmentData;
				FPCGMetadataAttribute<FString>* GrammarAttribute = CreateOrOverwriteAttribute<FString>(InContext, OutSegmentData->Metadata, Settings->OutGrammarAttribute, "");

				// iterate over all segments
				for (int i = 0; i < OutSegmentData->GetNumPoints(); ++i)
				{
					auto Grammar = Settings->GrammarSelection.bGrammarAsAttribute ? GrammarStrings[i] : Settings->GrammarSelection.GrammarString;

					auto ConstrainedString = GenerateWithConstraints(Context, Grammar, GetSegmentLength(SegmentData, i, Settings->SubdivisionAxis), Settings->Constraints);
					// save the generated grammar to the output grammar attribute
					GrammarAttribute->SetValue<FString>(OutSegmentData->GetMetadataEntry(i), ConstrainedString);
				}
			}
			else
			{
				// if the constraints are provided through input pins, iterate over all constraint sets
				TArray<FPCGTaggedData> ConstraintInputs = InContext->InputData.GetInputsByPin(PCGConstrainGrammar::Constants::ConstraintsPinLabel);
				for (auto ConstraintInput : ConstraintInputs)
				{
					const UPCGBasePointData* ConstraintPointData = Cast<const UPCGBasePointData>(ConstraintInput.Data);

					// copy input data to output and add Grammar attribute
					auto OutSegmentData = Cast<UPCGBasePointData>(SegmentData->DuplicateData(InContext));
					Outputs.Emplace_GetRef().Data = OutSegmentData;
					FPCGMetadataAttribute<FString>* GrammarAttribute = CreateOrOverwriteAttribute<FString>(InContext, OutSegmentData->Metadata, Settings->OutGrammarAttribute, "");

					// iterate over all segments
					for (int i = 0; i < OutSegmentData->GetNumPoints(); ++i)
					{
						auto Constraints = GetConstraintsOnSegment(InContext, Settings, SegmentData, i, ConstraintPointData);
						auto Grammar = Settings->GrammarSelection.bGrammarAsAttribute ? GrammarStrings[i] : Settings->GrammarSelection.GrammarString;

						auto ConstrainedString = GenerateWithConstraints(Context, Grammar, GetSegmentLength(SegmentData, i, Settings->SubdivisionAxis), Constraints);
						// save the generated grammar to the output grammar attribute
						GrammarAttribute->SetValue<FString>(OutSegmentData->GetMetadataEntry(i), ConstrainedString);
					}
				}
			}
		}
	}
	return true;
}

FString FPCGConstrainGrammarElement::GenerateWithConstraints(FPCGGrammarConstrainingContext* Context, const FString& GrammarString, float Length, const TArray<FPCGGrammarConstraint>& Constraints)
{
	std::vector<GenerationConstraint> GenerationConstraints;
	for (const auto& Constraint : Constraints)
	{
		auto ConstraintSymbol = FStringToStd(Constraint.Symbol.ToString());
		if (!Context->ModuleMap.contains(ConstraintSymbol))
		{
			PCGLog::LogWarningOnGraph(FText::Format(FText::FromString("Constraint symbol '{0}' at position {1} is not included in modules, will be ignored."),
			                                        Constraint.Symbol, Constraint.Position), Context);
		}
		else
		{
			GenerationConstraints.emplace_back(ConstraintSymbol, Constraint.Position, Constraint.bHasWidth ? Constraint.Width * 0.5f : 0.f);
		}
	}

	if (!MakeNFAForGrammar(Context, GrammarString))
	{
		return GrammarString;
	}
	
	Generator GrammarGenerator(Context->ModuleMap, Length, Context->ConstructedNFAs[GrammarString], GenerationConstraints);
	
	if (GrammarGenerator.wasGenerationSuccessful())
		return StdToFString(GrammarGenerator.getGenerationResult().getGeneratedString());

	if (GrammarGenerator.getErrorInfo() == GenerationErrorType::UnknownLiteral)
		PCGLog::LogErrorOnGraph(FText::Format(FText::FromString("The grammar '{0}' contains a symbol not listed in modules"), FText::FromString(GrammarString)), Context);
	else
		PCGLog::LogErrorOnGraph(FText::Format(FText::FromString("The given constraints could not be satisfied for grammar '{0}'"), FText::FromString(GrammarString)), Context);
	
	return GrammarString;
}

bool FPCGConstrainGrammarElement::MakeNFAForGrammar(FPCGGrammarConstrainingContext* InContext, const FString& GrammarString)
{
	if (!InContext->ConstructedNFAs.Contains(GrammarString))
	{
		const RegexParser Parser(FStringToStd(GrammarString));
		if (!Parser.wasParsingSuccessful())
		{
			if (Parser.getErrorInfo() == RegexErrorType::EmptyString)
				PCGLog::LogErrorOnGraph(FText::FromString("The provided grammar is empty."), InContext);
			else
				PCGLog::LogErrorOnGraph(FText::Format(FText::FromString("Grammar ({0}) could not be parsed."), FText::FromString(GrammarString)), InContext);
			return false;
		}

		const NFACompiler Compiler(Parser.getParsedRegex());
		if (!Compiler.wasConstructionSuccessful())
		{
			PCGLog::LogErrorOnGraph(FText::Format(FText::FromString("NFA could not be constructed for Grammar ({0})."), FText::FromString(GrammarString)), InContext);
			return false;
		}

		InContext->ConstructedNFAs.Emplace(GrammarString, Compiler.getConstructedNFA());
	}
	return true;
}

float FPCGConstrainGrammarElement::GetSegmentLength(const UPCGBasePointData* SegmentData, int SegmentIndex, EPCGSplitAxis SubdivisionAxis)
{
	const auto SegmentTransform = SegmentData->GetTransform(SegmentIndex);
	const auto SegmentBounds = SegmentData->GetLocalBounds(SegmentIndex).TransformBy(SegmentTransform);
	return GetVectorComponent(SegmentBounds.GetSize(), SubdivisionAxis);
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
		{
			PCGLog::LogWarningOnGraph(FText::Format(FText::FromString("Constraint symbol '{0}' is outside of the input shape, will be ignored."),
			                                        FText::FromString(Symbols[i])), InContext);
			continue;
		}

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
		{
			PCGLog::LogWarningOnGraph(FText::Format(FText::FromString("Constraint symbol '{0}' is outside of the input shape, will be ignored."),
			                                        FText::FromString(Symbols[i])), InContext);
			continue;
		}

		auto ClosestPoint = SegmentBounds.GetClosestPointTo(ConstraintTransform.GetLocation());
		auto Distances = ClosestPoint - SegmentBounds.Min;
		float Distance = GetVectorComponent(Distances, InSettings->SubdivisionAxis);

		Constraints.Emplace(FText::FromString(Symbols[i]), Distance, false, 0.0);
	}

	return Constraints;
}

TArray<FPCGConstrainedGrammarModule> FPCGConstrainGrammarElement::GetModules(FPCGContext* InContext, const UPCGConstrainGrammarSettings* InSettings)
{
	if (!InSettings->bModuleInfoAsInput)
		return InSettings->ModulesInfo;

	const TArray<FPCGTaggedData> ModulesInfoInputs = InContext->InputData.GetInputsByPin(PCGSubdivisionBase::Constants::ModulesInfoPinLabel);

	if (ModulesInfoInputs.IsEmpty())
	{
		PCGLog::LogWarningOnGraph(FText::FromString("No data was found on the module info pin."), InContext);
		return {};
	}

	const UPCGParamData* ParamData = Cast<UPCGParamData>(ModulesInfoInputs[0].Data);
	if (!ParamData)
	{
		PCGLog::LogWarningOnGraph(FText::FromString("Module info input is not of type attribute set."), InContext);
		return {};
	}

	TMap<FName, TTuple<FName, bool>> PropertyNameMapping;
	PropertyNameMapping.Emplace(GET_MEMBER_NAME_CHECKED(FPCGConstrainedGrammarModule, Symbol), {InSettings->ModulesInfoAttributeNames.SymbolAttributeName, /*bCanBeDefaulted=*/false});
	PropertyNameMapping.Emplace(GET_MEMBER_NAME_CHECKED(FPCGConstrainedGrammarModule, Size), {InSettings->ModulesInfoAttributeNames.SizeAttributeName, /*bCanBeDefaulted=*/false});
	PropertyNameMapping.Emplace(GET_MEMBER_NAME_CHECKED(FPCGConstrainedGrammarModule, bScalable), {
		                            InSettings->ModulesInfoAttributeNames.ScalableAttributeName, /*bCanBeDefaulted=*/!InSettings->ModulesInfoAttributeNames.bProvideScalable
	                            });
	PropertyNameMapping.Emplace(GET_MEMBER_NAME_CHECKED(FPCGConstrainedGrammarModule, DebugColor), {
		                            InSettings->ModulesInfoAttributeNames.DebugColorAttributeName, /*bCanBeDefaulted=*/!InSettings->ModulesInfoAttributeNames.bProvideDebugColor
	                            });
	PropertyNameMapping.Emplace(GET_MEMBER_NAME_CHECKED(FPCGConstrainedGrammarModule, bSpawnOnlyWithConstraint), {
		                            InSettings->ModulesInfoAttributeNames.SpawnOnlyWithConstraintAttributeName, /*bCanBeDefaulted=*/
		                            !InSettings->ModulesInfoAttributeNames.bProvideSpawnOnlyWithConstraint
	                            });

	return PCGPropertyHelpers::ExtractAttributeSetAsArrayOfStructs<FPCGConstrainedGrammarModule>(ParamData, &PropertyNameMapping, InContext);
}

FString FPCGConstrainGrammarElement::StdToFString(const std::string& String)
{
	return UTF8_TO_TCHAR(String.c_str());
}

std::string FPCGConstrainGrammarElement::FStringToStd(const FString& String)
{
	return TCHAR_TO_UTF8(*String);
}
