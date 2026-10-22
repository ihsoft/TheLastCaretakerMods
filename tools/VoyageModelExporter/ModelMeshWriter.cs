using System.Numerics;
using CUE4Parse.UE4.Objects.Core.Math;
using CUE4Parse_Conversion.Dto;
using CUE4Parse_Conversion.Writers.Gltf;
using SharpGLTF.Geometry;
using SharpGLTF.Geometry.VertexTypes;
using SharpGLTF.Materials;
using SharpGLTF.Schema2;

namespace VoyageModelExporter;

internal static class ModelMeshWriter
{
    const float UnitScale = .01f;

    internal static IMeshBuilder<MaterialBuilder> Build(
        string name,
        MeshLodDto<MeshVertex> lod,
        MaterialBuilder[] materials,
        bool includeTangents)
    {
        return includeTangents
            ? BuildWithTangents(name, lod, materials)
            : BuildWithoutTangents(name, lod, materials);
    }

    internal static NaniteMeshData CaptureNanite(MeshLodDto<MeshVertex> lod)
    {
        var positions = new float[lod.Vertices.Length * 3];
        var normals = new float[lod.Vertices.Length * 3];
        var colors = new float[lod.Vertices.Length * 4];
        var uvs = Enumerable.Range(0, 1 + lod.ExtraUvs.Length)
            .Select(_ => new float[lod.Vertices.Length * 2]).ToArray();
        FColor[]? sourceColors = null;
        if (lod.VertexColors is { Length: > 0 }) sourceColors = lod.VertexColors[0].Colors;
        var hasMeaningfulColors = sourceColors?.Any(color =>
            color.R != byte.MinValue || color.G != byte.MinValue ||
            color.B != byte.MinValue || color.A != byte.MinValue) == true;
        for (var index = 0; index < lod.Vertices.Length; index++)
        {
            var position = SwapYZ(lod.Vertices[index].Position * UnitScale);
            var normal = SwapYZAndNormalize((FVector)lod.Vertices[index].Normal);
            positions[index * 3] = position.X;
            positions[index * 3 + 1] = position.Y;
            positions[index * 3 + 2] = position.Z;
            normals[index * 3] = normal.X;
            normals[index * 3 + 1] = normal.Y;
            normals[index * 3 + 2] = normal.Z;
            var color = hasMeaningfulColors ? (Vector4)sourceColors![index] : Vector4.One;
            colors[index * 4] = color.X;
            colors[index * 4 + 1] = color.Y;
            colors[index * 4 + 2] = color.Z;
            colors[index * 4 + 3] = color.W;
            uvs[0][index * 2] = lod.Vertices[index].Uv.U;
            uvs[0][index * 2 + 1] = lod.Vertices[index].Uv.V;
            for (var uv = 0; uv < lod.ExtraUvs.Length; uv++)
            {
                uvs[uv + 1][index * 2] = lod.ExtraUvs[uv][index].U;
                uvs[uv + 1][index * 2 + 1] = lod.ExtraUvs[uv][index].V;
            }
        }
        var sections = lod.Sections.Select(section =>
        {
            var indices = new uint[checked(section.NumFaces * 3)];
            Array.Copy(lod.Indices, section.FirstIndex, indices, 0, indices.Length);
            return indices;
        }).ToArray();
        return new NaniteMeshData(positions, normals, colors, uvs, sections);
    }

    internal static void ApplyNaniteAccessors(ModelRoot model, NaniteMeshData data)
    {
        var mesh = model.LogicalMeshes.Single();
        if (mesh.Primitives.Count != data.SectionIndices.Length)
            throw new InvalidDataException("Nanite material-section count changed during GLB assembly.");
        var vertexCount = data.Positions.Length / 3;
        var position = VertexAccessor(model, "Nanite_POSITION", data.Positions, vertexCount, 3, DimensionType.VEC3);
        var normal = VertexAccessor(model, "Nanite_NORMAL", data.Normals, vertexCount, 3, DimensionType.VEC3);
        var color = VertexAccessor(model, "Nanite_COLOR_0", data.Colors, vertexCount, 4, DimensionType.VEC4);
        var uvAccessors = data.Uvs.Select((values, index) =>
            VertexAccessor(model, $"Nanite_TEXCOORD_{index}", values, vertexCount, 2, DimensionType.VEC2)).ToArray();
        for (var section = 0; section < data.SectionIndices.Length; section++)
        {
            var primitive = mesh.Primitives[section];
            primitive.SetVertexAccessor("POSITION", position);
            primitive.SetVertexAccessor("NORMAL", normal);
            primitive.SetVertexAccessor("COLOR_0", color);
            foreach (var pair in uvAccessors.Select((accessor, index) => (accessor, index)))
                primitive.SetVertexAccessor($"TEXCOORD_{pair.index}", pair.accessor);
            foreach (var attribute in primitive.VertexAccessors.Keys
                         .Where(name => name.StartsWith("TEXCOORD_", StringComparison.Ordinal) &&
                             int.Parse(name[9..]) >= uvAccessors.Length)
                         .ToArray())
                primitive.SetVertexAccessor(attribute, null!);
            var bytes = ToBytes(data.SectionIndices[section]);
            var view = model.UseBufferView(new ArraySegment<byte>(bytes), 0, BufferMode.ELEMENT_ARRAY_BUFFER);
            var indices = model.CreateAccessor($"Nanite_INDICES_{section}");
            indices.SetIndexData(view, 0, data.SectionIndices[section].Length, IndexEncodingType.UNSIGNED_INT);
            primitive.SetIndexAccessor(indices);
        }
    }

    static Accessor VertexAccessor(ModelRoot model, string name, float[] values, int count, int components, DimensionType dimensions)
    {
        var bytes = ToBytes(values);
        var view = model.UseBufferView(new ArraySegment<byte>(bytes), components * sizeof(float), BufferMode.ARRAY_BUFFER);
        var accessor = model.CreateAccessor(name);
        accessor.SetVertexData(view, 0, count, dimensions, EncodingType.FLOAT, false);
        return accessor;
    }

    static byte[] ToBytes(Array values)
    {
        var bytes = new byte[System.Buffer.ByteLength(values)];
        System.Buffer.BlockCopy(values, 0, bytes, 0, bytes.Length);
        return bytes;
    }

    static IMeshBuilder<MaterialBuilder> BuildWithTangents(
        string name, MeshLodDto<MeshVertex> lod, MaterialBuilder[] materials)
    {
        var builder = new MeshBuilder<VertexPositionNormalTangent, VertexColorXTextureX, VertexEmpty>(name);
        FColor[]? colors = null;
        if (lod.VertexColors is { Length: > 0 }) colors = lod.VertexColors[0].Colors;
        var hasMeaningfulVertexColors = colors?.Any(color =>
            color.R != byte.MinValue || color.G != byte.MinValue ||
            color.B != byte.MinValue || color.A != byte.MinValue) == true;
        var uvCount = 1 + lod.ExtraUvs.Length;
        var uv1 = new Vector2[uvCount];
        var uv2 = new Vector2[uvCount];
        var uv3 = new Vector2[uvCount];
        for (var sectionIndex = 0; sectionIndex < lod.Sections.Length; sectionIndex++)
        {
            var section = lod.Sections[sectionIndex];
            if (section.FirstIndex < 0 || section.NumFaces < 0 || section.FirstIndex + section.NumFaces * 3 > lod.Indices.Length)
                throw new InvalidDataException($"Invalid mesh section {sectionIndex} in {name}");
            if (section.MaterialIndex < 0 || section.MaterialIndex >= materials.Length)
                throw new InvalidDataException($"Material index {section.MaterialIndex} is outside the material table for {name}");
            var primitive = builder.UsePrimitive(materials[section.MaterialIndex]);
            for (var face = 0; face < section.NumFaces; face++)
            {
                var i0 = checked((int)lod.Indices[section.FirstIndex + face * 3]);
                var i1 = checked((int)lod.Indices[section.FirstIndex + face * 3 + 1]);
                var i2 = checked((int)lod.Indices[section.FirstIndex + face * 3 + 2]);
                if ((uint)i0 >= lod.Vertices.Length || (uint)i1 >= lod.Vertices.Length || (uint)i2 >= lod.Vertices.Length)
                    throw new InvalidDataException($"Triangle index is outside the vertex buffer for {name}");
                var v0 = lod.Vertices[i0];
                var v1 = lod.Vertices[i1];
                var v2 = lod.Vertices[i2];
                FillUvs(uv1, lod, v0, i0);
                FillUvs(uv2, lod, v1, i1);
                FillUvs(uv3, lod, v2, i2);
                var a = new VertexBuilder<VertexPositionNormalTangent, VertexColorXTextureX, VertexEmpty>(
                    Geometry(v0), new VertexColorXTextureX(uv1, VertexColor(i0)));
                var b = new VertexBuilder<VertexPositionNormalTangent, VertexColorXTextureX, VertexEmpty>(
                    Geometry(v1), new VertexColorXTextureX(uv2, VertexColor(i1)));
                var c = new VertexBuilder<VertexPositionNormalTangent, VertexColorXTextureX, VertexEmpty>(
                    Geometry(v2), new VertexColorXTextureX(uv3, VertexColor(i2)));
                primitive.AddTriangle(a, b, c);
            }
        }
        return builder;

        Vector4 VertexColor(int vertexIndex) => hasMeaningfulVertexColors
            ? colors![vertexIndex]
            : Vector4.One;
    }

    static IMeshBuilder<MaterialBuilder> BuildWithoutTangents(
        string name, MeshLodDto<MeshVertex> lod, MaterialBuilder[] materials)
    {
        var builder = new MeshBuilder<VertexPositionNormal, VertexColorXTextureX, VertexEmpty>(name);
        FColor[]? colors = null;
        if (lod.VertexColors is { Length: > 0 }) colors = lod.VertexColors[0].Colors;
        var hasMeaningfulVertexColors = colors?.Any(color =>
            color.R != byte.MinValue || color.G != byte.MinValue ||
            color.B != byte.MinValue || color.A != byte.MinValue) == true;
        var uvCount = 1 + lod.ExtraUvs.Length;
        var uv1 = new Vector2[uvCount];
        var uv2 = new Vector2[uvCount];
        var uv3 = new Vector2[uvCount];
        for (var sectionIndex = 0; sectionIndex < lod.Sections.Length; sectionIndex++)
        {
            var section = lod.Sections[sectionIndex];
            if (section.FirstIndex < 0 || section.NumFaces < 0 || section.FirstIndex + section.NumFaces * 3 > lod.Indices.Length)
                throw new InvalidDataException($"Invalid mesh section {sectionIndex} in {name}");
            if (section.MaterialIndex < 0 || section.MaterialIndex >= materials.Length)
                throw new InvalidDataException($"Material index {section.MaterialIndex} is outside the material table for {name}");
            var primitive = builder.UsePrimitive(materials[section.MaterialIndex]);
            for (var face = 0; face < section.NumFaces; face++)
            {
                var i0 = checked((int)lod.Indices[section.FirstIndex + face * 3]);
                var i1 = checked((int)lod.Indices[section.FirstIndex + face * 3 + 1]);
                var i2 = checked((int)lod.Indices[section.FirstIndex + face * 3 + 2]);
                if ((uint)i0 >= lod.Vertices.Length || (uint)i1 >= lod.Vertices.Length || (uint)i2 >= lod.Vertices.Length)
                    throw new InvalidDataException($"Triangle index is outside the vertex buffer for {name}");
                var v0 = lod.Vertices[i0];
                var v1 = lod.Vertices[i1];
                var v2 = lod.Vertices[i2];
                FillUvs(uv1, lod, v0, i0);
                FillUvs(uv2, lod, v1, i1);
                FillUvs(uv3, lod, v2, i2);
                var a = new VertexBuilder<VertexPositionNormal, VertexColorXTextureX, VertexEmpty>(
                    GeometryWithoutTangent(v0), new VertexColorXTextureX(uv1, VertexColor(i0)));
                var b = new VertexBuilder<VertexPositionNormal, VertexColorXTextureX, VertexEmpty>(
                    GeometryWithoutTangent(v1), new VertexColorXTextureX(uv2, VertexColor(i1)));
                var c = new VertexBuilder<VertexPositionNormal, VertexColorXTextureX, VertexEmpty>(
                    GeometryWithoutTangent(v2), new VertexColorXTextureX(uv3, VertexColor(i2)));
                primitive.AddTriangle(a, b, c);
            }
        }
        return builder;

        Vector4 VertexColor(int vertexIndex) => hasMeaningfulVertexColors
            ? colors![vertexIndex]
            : Vector4.One;
    }

    static void FillUvs(Vector2[] destination, MeshLodDto<MeshVertex> lod, MeshVertex vertex, int vertexIndex)
    {
        destination[0] = (Vector2)vertex.Uv;
        for (var i = 0; i < lod.ExtraUvs.Length; i++) destination[i + 1] = (Vector2)lod.ExtraUvs[i][vertexIndex];
    }

    static VertexPositionNormalTangent Geometry(MeshVertex vertex)
    {
        return new VertexPositionNormalTangent(SwapYZ(vertex.Position * UnitScale),
            SwapYZAndNormalize((FVector)vertex.Normal), SwapYZAndNormalize((Vector4)vertex.Tangent));
    }

    static VertexPositionNormal GeometryWithoutTangent(MeshVertex vertex)
    {
        return new VertexPositionNormal(
            SwapYZ(vertex.Position * UnitScale), SwapYZAndNormalize((FVector)vertex.Normal));
    }

    static FVector SwapYZ(FVector value) => new(value.X, value.Z, value.Y);
    static FVector SwapYZAndNormalize(FVector value)
    {
        var result = SwapYZ(value);
        result.Normalize();
        return result;
    }
    static Vector4 SwapYZAndNormalize(Vector4 value) => Vector4.Normalize(new Vector4(value.X, value.Z, value.Y, value.W));
}

internal sealed record NaniteMeshData(
    float[] Positions, float[] Normals, float[] Colors, float[][] Uvs, uint[][] SectionIndices);
