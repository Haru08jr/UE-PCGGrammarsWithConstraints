// Fill out your copyright notice in the Description page of Project Settings.


#include "PCGConstrainGrammarStructs.h"

#include <ranges>

std::set<std::string> FPCGGrammarConstrainingContext::GetModuleNameSet() const
{
	auto ModuleKeys = std::views::keys(ModuleMap);
	return {ModuleKeys.begin(), ModuleKeys.end()};
}
