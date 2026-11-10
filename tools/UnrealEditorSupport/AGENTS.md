# Unreal editor support

Use the public [editor support guide](../unreal-editor-support.md) and
`../Prepare-UnrealEditorSupport.ps1` for normal work. The shared mechanism knows
no game schemas or BPT contracts. Plugin source and native stubs belong to the
mod or tool that supplies them.

For tool development, `editor_support.py` owns native packaging, compatibility,
transactional installation and recovery. `tests/test_editor_support.py` checks
failure and ownership boundaries without Unreal; `tests/Test-EditorSupport.ps1`
checks the public path with a standalone native fixture and a project without
own C++ sources. [C++ consumer acceptance](../BlueprintText/Test-CppProjectSupport.ps1)
covers normal UBT builds with ready providers, stale receipt recovery and BPT
write/reopen/cook. Generated precompiled rules are managed package files, not
owner-maintained module sources.
`project_support.py` composes declared owner inputs and reuses ready providers;
`tests/test_project_support.py` covers source changes, collisions and failed updates.
Python callers hold the shared queue when calling these internal functions.
Use a fresh test output directory. Keep engine launches in the shared queue.
