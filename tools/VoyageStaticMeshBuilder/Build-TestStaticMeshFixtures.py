"""Create tiny deterministic GLB fixtures for the private builder test suite."""

import argparse
import json
import pathlib
import struct


def align4(data: bytearray) -> None:
    while len(data) % 4:
        data.append(0)


def build(path: pathlib.Path, names: list[str]) -> None:
    binary = bytearray()
    views = []
    accessors = []

    def accessor(values, component_type, kind, count, minimum=None, maximum=None):
        align4(binary)
        offset = len(binary)
        if component_type == 5126:
            payload = struct.pack("<" + "f" * len(values), *values)
        elif component_type == 5123:
            payload = struct.pack("<" + "H" * len(values), *values)
        elif component_type == 5121:
            payload = bytes(values)
        else:
            raise ValueError(component_type)
        binary.extend(payload)
        views.append({"buffer": 0, "byteOffset": offset, "byteLength": len(payload)})
        item = {
            "bufferView": len(views) - 1,
            "componentType": component_type,
            "count": count,
            "type": kind,
        }
        if minimum is not None:
            item["min"] = minimum
            item["max"] = maximum
        if component_type == 5121:
            item["normalized"] = True
        accessors.append(item)
        return len(accessors) - 1

    meshes = []
    nodes = []
    materials = []
    for index, name in enumerate(names):
        x = float(index * 2)
        positions = [x, 0.0, 0.0, x + 1.0, 0.0, 0.0, x, 1.0, 0.0]
        normals = [0.0, 0.0, 1.0] * 3
        uvs = [0.0, 0.0, 1.0, 0.0, 0.0, 1.0]
        colors = [255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255]
        indices = [0, 1, 2]
        p = accessor(positions, 5126, "VEC3", 3, [x, 0.0, 0.0], [x + 1.0, 1.0, 0.0])
        n = accessor(normals, 5126, "VEC3", 3)
        uv = accessor(uvs, 5126, "VEC2", 3)
        color = accessor(colors, 5121, "VEC4", 3)
        ind = accessor(indices, 5123, "SCALAR", 3)
        material_name = "SlotA" if index == 0 else "SlotB"
        materials.append({"name": material_name, "pbrMetallicRoughness": {"baseColorFactor": [1, 1, 1, 1]}})
        meshes.append({
            "name": name,
            "primitives": [{
                "attributes": {"POSITION": p, "NORMAL": n, "TEXCOORD_0": uv, "COLOR_0": color},
                "indices": ind,
                "material": len(materials) - 1,
            }],
        })
        nodes.append({"name": name, "mesh": index})

    document = {
        "asset": {"version": "2.0", "generator": "VoyageStaticMeshBuilder fixture"},
        "scene": 0,
        "scenes": [{"nodes": list(range(len(nodes)))}],
        "nodes": nodes,
        "meshes": meshes,
        "materials": materials,
        "buffers": [{"byteLength": len(binary)}],
        "bufferViews": views,
        "accessors": accessors,
    }
    json_bytes = json.dumps(document, separators=(",", ":")).encode("utf-8")
    while len(json_bytes) % 4:
        json_bytes += b" "
    align4(binary)
    total = 12 + 8 + len(json_bytes) + 8 + len(binary)
    glb = bytearray(struct.pack("<4sII", b"glTF", 2, total))
    glb.extend(struct.pack("<I4s", len(json_bytes), b"JSON"))
    glb.extend(json_bytes)
    glb.extend(struct.pack("<I4s", len(binary), b"BIN\0"))
    glb.extend(binary)
    path.write_bytes(glb)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    root = pathlib.Path(args.output)
    root.mkdir(parents=True, exist_ok=False)
    build(root / "single.glb", ["Solo"])
    build(root / "multi.glb", ["Alpha", "Beta"])


if __name__ == "__main__":
    main()
