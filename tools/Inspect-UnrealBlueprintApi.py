import pathlib
import unreal


def public_names(value):
    return sorted(name for name in dir(value) if not name.startswith("_"))


project_dir = pathlib.Path(unreal.Paths.project_dir())
output_path = project_dir / "Saved" / "BlueprintApi.txt"
output_path.parent.mkdir(parents=True, exist_ok=True)

interesting_types = [
    "BlueprintEditorLibrary",
    "BlueprintFactory",
    "EdGraph",
    "EdGraphNode",
    "EdGraphSchema_K2",
    "K2Node",
    "K2Node_CallFunction",
    "K2Node_DynamicCast",
    "K2Node_Event",
    "K2Node_VariableGet",
    "K2Node_VariableSet",
]

lines = []
for type_name in interesting_types:
    value = getattr(unreal, type_name, None)
    lines.append(f"[{type_name}] present={value is not None}")
    if value is not None:
        lines.extend(f"  {name}" for name in public_names(value))

output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
unreal.log(f"Blueprint API report written to {output_path}")
