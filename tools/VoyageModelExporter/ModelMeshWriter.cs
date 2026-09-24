using System.Numerics;
using CUE4Parse.UE4.Objects.Core.Math;
using CUE4Parse_Conversion.Dto;
using CUE4Parse_Conversion.Writers.Gltf;
using SharpGLTF.Geometry;
using SharpGLTF.Geometry.VertexTypes;
using SharpGLTF.Materials;

namespace VoyageModelExporter;

internal static class ModelMeshWriter
{
    const float UnitScale = .01f;

    internal static IMeshBuilder<MaterialBuilder> Build(string name, MeshLodDto<MeshVertex> lod, MaterialBuilder[] materials)
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

    static FVector SwapYZ(FVector value) => new(value.X, value.Z, value.Y);
    static FVector SwapYZAndNormalize(FVector value)
    {
        var result = SwapYZ(value);
        result.Normalize();
        return result;
    }
    static Vector4 SwapYZAndNormalize(Vector4 value) => Vector4.Normalize(new Vector4(value.X, value.Z, value.Y, value.W));
}
