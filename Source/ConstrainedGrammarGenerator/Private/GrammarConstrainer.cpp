// Fill out your copyright notice in the Description page of Project Settings.

#include "GrammarConstrainer.h"

#include "Generator.hpp"
#include "automaton/NFACompiler.hpp"
#include "Containers/StringConv.h"
#include "Elements/Grammar/PCGSubdivisionBase.h"
#include "regex/RegexParser.hpp"
#include "Utils/PCGLogErrors.h"

FString UGrammarConstrainer::GenerateWithConstraints(FPCGContext* InContext, const FString& GrammarString, float Length, const PCGSubdivisionBase::FModuleInfoMap& Modules,
                                                     const TArray<FPCGGrammarConstraint>& Constraints)
{
	std::vector<GenerationConstraint> GenerationConstraints;
	for (const auto& Constraint : Constraints)
	{
		if (!Modules.Contains(FName(Constraint.Symbol.ToString())))
		{
			PCGLog::LogErrorOnGraph(FText::Format(FText::FromString("Constraint symbol '{0}' is not included in modules"), Constraint.Symbol), InContext);
			return "";
		}
		GenerationConstraints.emplace_back(FStringToStd(Constraint.Symbol.ToString()), Constraint.Position, Constraint.bHasWidth? Constraint.Width * 0.5f : 0.f);
	}
	
	if (!MakeNFAForGrammar(InContext, GrammarString))
	{
		return "";
	}
	
	std::map<std::string, float> Symbols;
	for (const auto& [ModuleName, Module] : Modules)
	{
		Symbols.emplace(FStringToStd(ModuleName.ToString()), Module.Size);
	}
	
	auto result = Generator::generate(Symbols, Length, ConstructedNFAs[GrammarString], GenerationConstraints);
	
	if (!result.isValid())
	{
		PCGLog::LogErrorOnGraph(FText::Format(FText::FromString("The given constraints could not be satisfied for grammar '{0}'"), FText::FromString(GrammarString)), InContext);
		return "";
	}
	return StdToFString(result.getGeneratedString());
}

bool UGrammarConstrainer::MakeNFAForGrammar(FPCGContext* InContext, const FString& GrammarString)
{
	if (!ConstructedNFAs.Contains(GrammarString))
	{
		const RegexParser Parser(FStringToStd(GrammarString));
		if (!Parser.wasParsingSuccessful())
		{
			PCGLog::LogErrorOnGraph(FText::Format(FText::FromString("Grammar ({0}) could not be parsed."), FText::FromString(GrammarString)), InContext);
			return false;
		}
		
		const NFACompiler Compiler(Parser.getParsedRegex());
		if (!Compiler.wasConstructionSuccessful())
		{
			PCGLog::LogErrorOnGraph(FText::Format(FText::FromString("NFA could not be constructed for Grammar ({0})."), FText::FromString(GrammarString)), InContext);
			return false;
		}

		ConstructedNFAs.Emplace(GrammarString, Compiler.getConstructedNFA());
	}
	return true;
}

FString UGrammarConstrainer::StdToFString(const std::string& String)
{
	return UTF8_TO_TCHAR(String.c_str());
}

std::string UGrammarConstrainer::FStringToStd(const FString& String)
{
	return TCHAR_TO_UTF8(*String);
}
