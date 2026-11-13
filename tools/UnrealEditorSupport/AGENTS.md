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
[Clean-checkout acceptance](../BlueprintText/Test-CleanProjectSupport.ps1)
covers BPT-owned bootstrap, reuse across fresh project copies, overlay
invalidation and public source-mode reuse.
`project_support.py` composes declared owner inputs and reuses ready providers;
`tests/test_project_support.py` covers source changes, collisions and failed updates.
`provider_cache.py` owns the bounded shared cache, atomic publication, corruption
recovery and eviction. Offline provider tests isolate this store under `Tmp/`.
Update `BUILD_RECIPE` when changing the fixed build recipe; source and overlay
changes already invalidate reuse through their actual bytes.
Python callers hold the shared queue when calling these internal functions.
Use a fresh test output directory. Keep engine launches in the shared queue.
