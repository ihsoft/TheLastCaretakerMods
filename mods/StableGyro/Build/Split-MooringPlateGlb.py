"""Split the freshly exported stock Gyro mooring plate into its two disconnected sides.

Run only in Blender 5+ background mode. Import and export use Blender's built-in
glTF operators. The script selects each disconnected side, preserves all faces,
and restores the source corner normals in the exported NORMAL accessor.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

import bpy


def require(value, message):
    if not value:
        raise RuntimeError(message)


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def rounded(values):
    values = [round(value, 5) for value in values]
    return tuple(0.0 if value == 0.0 else value for value in values)


def raw_glb_source_normals(path):
    data = path.read_bytes()
    json_size, json_kind = struct.unpack_from("<II", data, 12)
    require(json_kind == 0x4E4F534A, "Missing source GLB JSON chunk")
    document = json.loads(data[20:20 + json_size].decode("utf-8"))
    binary_start = 20 + json_size + 8
    primitives = [primitive for mesh in document["meshes"] for primitive in mesh["primitives"]]
    require(len(primitives) == 1, "Source GLB must contain exactly one primitive")

    def accessor(index):
        item = document["accessors"][index]
        view = document["bufferViews"][item["bufferView"]]
        formats = {5126: "f", 5125: "I", 5123: "H", 5121: "B", 5122: "h"}
        dimensions = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}
        code = formats[item["componentType"]]
        width = struct.calcsize("<" + code)
        count = dimensions[item["type"]]
        stride = view.get("byteStride", width * count)
        start = binary_start + view.get("byteOffset", 0) + item.get("byteOffset", 0)
        return [struct.unpack_from("<" + code * count, data, start + row * stride)
                for row in range(item["count"])]

    attributes = primitives[0]["attributes"]
    positions = accessor(attributes["POSITION"])
    normals = accessor(attributes["NORMAL"])
    uvs = [accessor(attributes[f"TEXCOORD_{index}"]) for index in range(4)]
    indices = [row[0] for row in accessor(primitives[0]["indices"])]
    records = {}
    for offset in range(0, len(indices), 3):
        corners = []
        for vertex in indices[offset:offset + 3]:
            point = positions[vertex]
            blender_point = (point[0], -point[2], point[1])
            normal = normals[vertex]
            blender_normal = (normal[0], -normal[2], normal[1])
            length = math.sqrt(sum(value * value for value in blender_normal))
            key = rounded(list(blender_point) +
                          [value for uv in uvs for value in (uv[vertex][0], 1.0 - uv[vertex][1])])
            corners.append((key, tuple(value / length for value in blender_normal)))
        corners.sort(key=lambda corner: corner[0])
        records.setdefault(tuple(corner[0] for corner in corners), []).append(
            tuple(corner[1] for corner in corners))
    return records


def restore_raw_source_normals(obj, source):
    records = raw_glb_source_normals(source)
    mesh = obj.data
    mesh.calc_loop_triangles()
    restored = [None] * len(mesh.loops)
    matched = 0
    for triangle in mesh.loop_triangles:
        corners = []
        for loop_index in triangle.loops:
            loop = mesh.loops[loop_index]
            point = obj.matrix_world @ mesh.vertices[loop.vertex_index].co
            values = list(point)
            for layer in mesh.uv_layers[:4]:
                values.extend(layer.data[loop_index].uv)
            corners.append((rounded(values), loop_index,
                            tuple(mesh.corner_normals[loop_index].vector.normalized())))
        corners.sort(key=lambda corner: corner[0])
        key = tuple(corner[0] for corner in corners)
        candidates = records.get(key)
        require(candidates, "Raw source normal triangle identity was not found")
        candidate_index = min(range(len(candidates)), key=lambda index:
            max(math.degrees(math.acos(max(-1.0, min(1.0,
                sum(a * b for a, b in zip(corners[corner][2], candidates[index][corner]))))))
                for corner in range(3)))
        candidate = candidates.pop(candidate_index)
        for corner, normal in zip(corners, candidate):
            previous = restored[corner[1]]
            require(previous is None or max(abs(a - b) for a, b in zip(previous, normal)) < 1e-6,
                    "One Blender loop maps to conflicting raw source normals")
            restored[corner[1]] = normal
        matched += 1
    require(matched == 13790 and all(not values for values in records.values()),
            "Raw source normal restoration did not consume every triangle")
    require(all(normal is not None for normal in restored), "A Blender loop has no restored source normal")
    mesh.normals_split_custom_set(restored)
    return matched


def restore_exported_glb_normals(source, output):
    def load(path):
        data = bytearray(path.read_bytes())
        json_size, json_kind = struct.unpack_from("<II", data, 12)
        require(json_kind == 0x4E4F534A, "Missing GLB JSON chunk")
        document = json.loads(data[20:20 + json_size].decode("utf-8"))
        return data, document, 20 + json_size + 8

    def accessor(data, document, binary_start, index):
        item = document["accessors"][index]
        view = document["bufferViews"][item["bufferView"]]
        formats = {5126: "f", 5125: "I", 5123: "H", 5121: "B", 5122: "h"}
        dimensions = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}
        code = formats[item["componentType"]]
        width, count = struct.calcsize("<" + code), dimensions[item["type"]]
        stride = view.get("byteStride", width * count)
        start = binary_start + view.get("byteOffset", 0) + item.get("byteOffset", 0)
        values = [struct.unpack_from("<" + code * count, data, start + row * stride)
                  for row in range(item["count"])]
        return values, start, stride, code

    def triangles(path):
        data, document, binary_start = load(path)
        primitives = [primitive for mesh in document["meshes"] for primitive in mesh["primitives"]]
        require(len(primitives) == 1, "Normal restoration requires one GLB primitive")
        primitive = primitives[0]
        attributes = primitive["attributes"]
        positions, _, _, _ = accessor(data, document, binary_start, attributes["POSITION"])
        normals, normal_start, normal_stride, normal_code = accessor(
            data, document, binary_start, attributes["NORMAL"])
        require(normal_code == "f", "Normal accessor must use float components")
        uvs = [accessor(data, document, binary_start, attributes[f"TEXCOORD_{index}"])[0]
               for index in range(4)]
        indices = [row[0] for row in accessor(data, document, binary_start, primitive["indices"])[0]]
        records = {}
        for offset in range(0, len(indices), 3):
            corners = []
            for vertex in indices[offset:offset + 3]:
                key = rounded(list(positions[vertex]) +
                              [value for uv in uvs for value in uv[vertex]])
                normal = normals[vertex]
                length = math.sqrt(sum(value * value for value in normal))
                corners.append((key, vertex, tuple(value / length for value in normal)))
            corners.sort(key=lambda corner: corner[0])
            records.setdefault(tuple(corner[0] for corner in corners), []).append(tuple(corners))
        return data, records, normal_start, normal_stride

    _, source_records, _, _ = triangles(source)
    data, output_records, normal_start, normal_stride = triangles(output)
    before = bytes(data)
    assignments = {}
    matched = 0
    for key, output_triangles in output_records.items():
        candidates = list(source_records.get(key, []))
        require(len(candidates) >= len(output_triangles), "Exported triangle lacks a raw source normal record")
        for target in output_triangles:
            selected = min(range(len(candidates)), key=lambda index:
                max(math.degrees(math.acos(max(-1.0, min(1.0,
                    sum(a * b for a, b in zip(target[corner][2], candidates[index][corner][2]))))))
                    for corner in range(3)))
            source_triangle = candidates.pop(selected)
            for target_corner, source_corner in zip(target, source_triangle):
                vertex, normal = target_corner[1], source_corner[2]
                previous = assignments.get(vertex)
                require(previous is None or max(abs(a - b) for a, b in zip(previous, normal)) < 1e-6,
                        "Exported vertex maps to conflicting raw source normals")
                assignments[vertex] = normal
            matched += 1
    for vertex, normal in assignments.items():
        struct.pack_into("<fff", data, normal_start + vertex * normal_stride, *normal)
    allowed = {offset for vertex in assignments
               for offset in range(normal_start + vertex * normal_stride,
                                   normal_start + vertex * normal_stride + 12)}
    changed = [index for index, (left, right) in enumerate(zip(before, data)) if left != right]
    require(all(index in allowed for index in changed),
            "Post-export normal restoration changed bytes outside the NORMAL accessor")
    output.write_bytes(data)
    return {"triangles": matched, "vertices": len(assignments),
            "changedNormalBytes": len(changed), "nonNormalBytesChanged": 0}


def mesh_evidence(obj):
    mesh = obj.data
    mesh.calc_loop_triangles()
    require(len(mesh.polygons) == len(mesh.loop_triangles), "Source must already be triangulated")
    require(len(mesh.uv_layers) >= 4, "Stock mooring plate must retain at least four UV channels")
    points = [obj.matrix_world @ vertex.co for vertex in mesh.vertices]
    exact_zero_area = 0
    for triangle in mesh.loop_triangles:
        a, b, c = (points[index] for index in triangle.vertices)
        exact_zero_area += int((b - a).cross(c - a).length_squared == 0.0)
    return {
        "name": obj.name,
        "vertices": len(mesh.vertices),
        "triangles": len(mesh.loop_triangles),
        "uvChannels": len(mesh.uv_layers),
        "colorAttributes": len(mesh.color_attributes),
        "materials": len(mesh.materials),
        "exactZeroAreaTriangles": exact_zero_area,
        "boundsMin": [min(point[axis] for point in points) for axis in range(3)],
        "boundsMax": [max(point[axis] for point in points) for axis in range(3)],
    }


def triangle_records(obj):
    mesh = obj.data
    mesh.calc_loop_triangles()
    records = []
    for triangle in mesh.loop_triangles:
        corners = []
        for loop_index in triangle.loops:
            loop = mesh.loops[loop_index]
            point = obj.matrix_world @ mesh.vertices[loop.vertex_index].co
            values = list(point)
            for layer in mesh.uv_layers[:4]:
                values.extend(layer.data[loop_index].uv)
            corners.append(rounded(values))
        records.append(tuple(sorted(corners)))
    return sorted(records)


def records_hash(records):
    payload = json.dumps(records, separators=(",", ":")).encode("utf-8")
    return hashlib.sha256(payload).hexdigest().upper()


def keep_side(source, name, positive_y):
    source_mesh = source.data
    # Voyage's exported UE coordinates import into Blender as (UE.X, -UE.Y, UE.Z).
    # Positive UE Y therefore keeps negative Blender Y, and vice versa. Rebuild
    # the selected disconnected faces through Blender's Mesh API so all corner
    # attributes are copied explicitly without an edit-mode normal rewrite.
    polygons = [polygon for polygon in source_mesh.polygons
                if all((source_mesh.vertices[index].co.y < 0.0) == positive_y
                       for index in polygon.vertices)]
    used = sorted({index for polygon in polygons for index in polygon.vertices})
    remap = {old: new for new, old in enumerate(used)}
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([source_mesh.vertices[index].co.copy() for index in used], [],
                     [[remap[index] for index in polygon.vertices] for polygon in polygons])
    mesh.update()
    loop_pairs = []
    for old_polygon, new_polygon in zip(polygons, mesh.polygons):
        new_polygon.material_index = old_polygon.material_index
        loop_pairs.extend(zip(old_polygon.loop_indices, new_polygon.loop_indices))
    for source_layer in source_mesh.uv_layers:
        target_layer = mesh.uv_layers.new(name=source_layer.name)
        for old_loop, new_loop in loop_pairs:
            target_layer.data[new_loop].uv = source_layer.data[old_loop].uv
    for source_color in source_mesh.color_attributes:
        target_color = mesh.color_attributes.new(
            name=source_color.name, type=source_color.data_type, domain=source_color.domain)
        if source_color.domain == "POINT":
            for old_vertex, new_vertex in remap.items():
                target_color.data[new_vertex].color = source_color.data[old_vertex].color
        elif source_color.domain == "CORNER":
            for old_loop, new_loop in loop_pairs:
                target_color.data[new_loop].color = source_color.data[old_loop].color
        else:
            raise RuntimeError("Unsupported stock color-attribute domain: " + source_color.domain)
    custom_normals = [None] * len(mesh.loops)
    for old_loop, new_loop in loop_pairs:
        custom_normals[new_loop] = source_mesh.corner_normals[old_loop].vector.copy()
    mesh.normals_split_custom_set(custom_normals)
    for material in source_mesh.materials:
        mesh.materials.append(material)
    obj = bpy.data.objects.new(name, mesh)
    obj.matrix_world = source.matrix_world.copy()
    bpy.context.collection.objects.link(obj)
    return obj


def export_one(obj, output):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    props = bpy.ops.export_scene.gltf.get_rna_type().properties
    options = dict(
        filepath=str(output), export_format="GLB", export_yup=True,
        export_normals=True, export_tangents=False, export_animations=False,
        export_cameras=False, export_apply=False, export_extras=False,
        export_materials="PLACEHOLDER",
    )
    options["use_selection" if "use_selection" in props else "export_selected"] = True
    if "export_lights" in props:
        options["export_lights"] = False
    require("FINISHED" in bpy.ops.export_scene.gltf(**options), "GLB export failed: " + str(output))


def add_material_binding_metadata(path):
    data = path.read_bytes()
    require(struct.unpack_from("<4sII", data) == (b"glTF", 2, len(data)), "Invalid GLB header")
    json_size, json_kind = struct.unpack_from("<II", data, 12)
    require(json_kind == 0x4E4F534A, "Missing GLB JSON chunk")
    document = json.loads(data[20:20 + json_size].decode("utf-8"))
    primitives = [primitive for mesh in document["meshes"] for primitive in mesh["primitives"]]
    require(len(primitives) == 1, "Material binding requires one GLB primitive")
    require(not document.get("materials") and "material" not in primitives[0],
            "Split GLB unexpectedly already contains material metadata")
    document["materials"] = [{"name": "Gyrocopter_Seat_p1"}]
    primitives[0]["material"] = 0
    encoded = json.dumps(document, separators=(",", ":")).encode("utf-8")
    encoded += b" " * ((4 - len(encoded) % 4) % 4)
    preserved_chunks = data[20 + json_size:]
    preserved_sha256 = hashlib.sha256(preserved_chunks).hexdigest().upper()
    rebuilt = (struct.pack("<4sII", b"glTF", 2, 20 + len(encoded) + len(preserved_chunks)) +
               struct.pack("<II", len(encoded), 0x4E4F534A) + encoded + preserved_chunks)
    path.write_bytes(rebuilt)
    require(hashlib.sha256(rebuilt[20 + len(encoded):]).hexdigest().upper() == preserved_sha256,
            "Material metadata update changed non-JSON GLB chunks")
    return {
        "slot": "Gyrocopter_Seat_p1",
        "primitiveMaterialIndex": 0,
        "preservedNonJsonChunksSha256": preserved_sha256,
    }


def glb_attributes(path):
    data = path.read_bytes()
    require(struct.unpack_from("<4sII", data) == (b"glTF", 2, len(data)), "Invalid GLB header")
    size, kind = struct.unpack_from("<II", data, 12)
    require(kind == 0x4E4F534A, "Missing GLB JSON chunk")
    doc = json.loads(data[20:20 + size].decode("utf-8"))
    primitives = [primitive for mesh in doc["meshes"] for primitive in mesh["primitives"]]
    require(len(primitives) == 1, "Split GLB must contain exactly one primitive")
    return sorted(primitives[0]["attributes"])


def readback(output):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    require("FINISHED" in bpy.ops.import_scene.gltf(filepath=str(output)), "GLB readback import failed")
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    require(len(meshes) == 1, "Expected one mesh in split GLB readback")
    return mesh_evidence(meshes[0]), triangle_records(meshes[0])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--positive-y", type=Path, required=True)
    parser.add_argument("--negative-y", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
    source = args.source.resolve()
    outputs = [args.positive_y.resolve(), args.negative_y.resolve(), args.report.resolve()]
    require(bpy.app.background and bpy.app.version >= (5, 0, 0), "Requires Blender 5+ background mode")
    require(source.is_file() and source.suffix.lower() == ".glb", "Source must be a GLB")
    require(all(not path.exists() for path in outputs), "Outputs must be fresh")
    bpy.ops.wm.read_factory_settings(use_empty=True)
    require("FINISHED" in bpy.ops.import_scene.gltf(filepath=str(source)), "Stock GLB import failed")
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    require(len(meshes) == 1, "Expected exactly one stock mooring-plate mesh")
    original = meshes[0]
    require(original.matrix_world.is_identity, "Stock mesh transform must remain identity")
    restored_normal_triangles = restore_raw_source_normals(original, source)
    original.data.calc_loop_triangles()
    crossing = 0
    for triangle in original.data.loop_triangles:
        ys = [original.data.vertices[index].co.y for index in triangle.vertices]
        crossing += int(min(ys) < 0.0 < max(ys))
    require(crossing == 0, "A stock triangle crosses the Y=0 split plane")
    source_evidence = mesh_evidence(original)
    source_records = triangle_records(original)
    require(source_evidence["triangles"] == 13790, "Unexpected decoded Nanite triangle count")
    # The public decoder reports 114 exact-zero faces in UE centimeter space.
    # After glTF meter conversion and Blender float import, 26 collinear faces
    # are no longer bit-exact zero; the 13,790-face source count remains intact.
    require(source_evidence["exactZeroAreaTriangles"] == 88,
            "Blender-imported Nanite exact-zero-area face contract changed: " +
            str(source_evidence["exactZeroAreaTriangles"]))
    positive = keep_side(original, "StableGyroMooringPlatePositiveY", True)
    negative = keep_side(original, "StableGyroMooringPlateNegativeY", False)
    positive_evidence = mesh_evidence(positive)
    negative_evidence = mesh_evidence(negative)
    split_records = sorted(triangle_records(positive) + triangle_records(negative))
    require(split_records == source_records, "Split changed position/UV0..3 triangle values")
    require(positive_evidence["triangles"] + negative_evidence["triangles"] == source_evidence["triangles"],
            "Split triangle total differs from stock")
    require(positive_evidence["uvChannels"] == source_evidence["uvChannels"] and
            negative_evidence["uvChannels"] == source_evidence["uvChannels"],
            "Split changed the UV-channel contract")
    args.positive_y.parent.mkdir(parents=True, exist_ok=True)
    export_one(positive, args.positive_y)
    export_one(negative, args.negative_y)
    material_bindings = {
        "positiveY": add_material_binding_metadata(args.positive_y),
        "negativeY": add_material_binding_metadata(args.negative_y),
    }
    restored_output_normals = {
        "positiveY": restore_exported_glb_normals(source, args.positive_y),
        "negativeY": restore_exported_glb_normals(source, args.negative_y),
    }
    positive_readback, positive_readback_records = readback(args.positive_y)
    negative_readback, negative_readback_records = readback(args.negative_y)
    for expected, actual in ((positive_evidence, positive_readback),
                             (negative_evidence, negative_readback)):
        require(actual["triangles"] == expected["triangles"], "Export changed triangle count")
        require(actual["uvChannels"] == source_evidence["uvChannels"], "Export changed UV channels")
        require(actual["materials"] == 1, "Split GLB lost the named material slot")
    source_attributes = glb_attributes(source)
    positive_attributes = glb_attributes(args.positive_y)
    negative_attributes = glb_attributes(args.negative_y)
    required_source_attributes = ["COLOR_0", "NORMAL", "POSITION", "TEXCOORD_0", "TEXCOORD_1",
                           "TEXCOORD_2", "TEXCOORD_3"]
    required_split_attributes = required_source_attributes
    require(all(name in source_attributes for name in required_source_attributes) and
            "TANGENT" not in source_attributes, "Decoded Nanite attribute contract changed")
    require(all(name in positive_attributes and name in negative_attributes for name in required_split_attributes) and
            "TANGENT" not in positive_attributes and "TANGENT" not in negative_attributes,
            "Split exports did not retain the source NORMAL/COLOR/UV contract")
    final_records = sorted(positive_readback_records + negative_readback_records)
    require(final_records == source_records,
            "Final GLB readback changed position/UV0..3 triangle values")
    report = {
        "schemaVersion": 2,
        "status": "passed",
        "method": "Blender built-in glTF import/export with disconnected-side vertex deletion",
        "blender": bpy.app.version_string,
        "source": str(source),
        "sourceSha256": sha256(source),
        "crossingTriangles": crossing,
        "manualFacesRemoved": source_evidence["triangles"] -
                              positive_evidence["triangles"] - negative_evidence["triangles"],
        "rawSourceNormalTrianglesRestored": restored_normal_triangles,
        "exportedRawNormalTrianglesRestored": restored_output_normals,
        "sourceMesh": source_evidence,
        "sourceAttributes": source_attributes,
        "sourceTrianglePositionUvSha256": records_hash(source_records),
        "splitTrianglePositionUvSha256": records_hash(split_records),
        "finalReadbackTrianglePositionUvSha256": records_hash(final_records),
        "positiveY": {**positive_evidence, "readback": positive_readback,
                      "path": str(args.positive_y), "sha256": sha256(args.positive_y)},
        "negativeY": {**negative_evidence, "readback": negative_readback,
                      "path": str(args.negative_y), "sha256": sha256(args.negative_y)},
        "originPreserved": True,
        "materialExport": "PLACEHOLDER",
        "materialBindings": material_bindings,
        "pbrTexturesExported": False,
        "positiveYAttributes": positive_attributes,
        "negativeYAttributes": negative_attributes,
    }
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("STABLEGYRO_MOORING_SPLIT_OK " + json.dumps(report))


if __name__ == "__main__":
    main()
