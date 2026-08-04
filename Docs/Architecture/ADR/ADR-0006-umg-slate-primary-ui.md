# ADR-0006: Native UMG/Slate as Primary Fallback UI Framework

## Context
The project contains C++ ViewModels for NoesisGUI, but the NoesisGUI Unreal Engine plugin is missing from `Plugins/`, blocking standard UBT builds.

## Decision
Designate native Unreal UMG/Slate (`Content/RA4UI/Widgets/`) as the primary active UI framework for Unreal Engine builds until NoesisGUI plugin binaries are integrated into `Plugins/NoesisGUI`.

## Rationale
- Allows immediate compilation and packaging of shipping client executables without external plugin dependency blockers.

## Status
**ACCEPTED / IMPLEMENTED**.
