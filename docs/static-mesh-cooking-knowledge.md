# StaticMesh cooking and validation knowledge

Voyage `StaticMesh` preparation has several independent contracts. Treating one
successful stage as proof of the others hides defects and encourages
mod-specific settings to become accidental repository standards.

## Independent cooked representations

Unreal builds ordinary rendering LODs, Nanite resources, Nanite fallback
geometry, and ray-tracing proxy data through related but independently
configured paths. A cooked mesh can contain valid Nanite pages while lacking a
usable ray-tracing proxy, or contain ordinary geometry with Nanite disabled.
Inspect the representation required by the consumer instead of inferring it
from a successful cook or another representation's presence.

Nanite fallback percentage, relative-error targets, precision, tangent policy,
and ray-tracing support are consumer choices. A value that worked for one mesh
does not become a default quality budget for another. Tiny geometry, collision,
visual silhouette, materials, and performance can require different policies.

Cooked StaticMesh bulk data may be split across `.uasset`, `.uexp`, `.ubulk`,
`.uptnl`, and `.m.ubulk` companions. Preserve every companion emitted for the
package. Successful parsing of the header or ordinary LOD does not prove that
streamed Nanite or ray-tracing payloads were retained.

## Separate evidence levels

Use the narrowest claim supported by the evidence:

1. Source inspection establishes properties of authored geometry.
2. Native import and cook establish that Unreal produced a cooked package.
3. Structural inspection establishes serialized records, counts, flags, and
   package ownership.
4. Complete Nanite-page decoding establishes that the available pages and
   clusters can be reconstructed by the inspected reader.
5. Source comparison establishes only the selected attributes, topology policy,
   coordinate assumptions, and tolerances.
6. A real-game test establishes the observed rendering or gameplay behavior of
   the exact installed candidate.
7. Performance requires measurement; geometry counts and successful rendering
   are not GPU or frame-time measurements.

Keep observed facts separate from acceptance policy. A missing or unrequested
criterion is not a pass. The consumer selects required attributes, numeric
tolerances, allowed topology differences, fallback budget, and ray-tracing
requirements. Reports should expose unusual facts such as zero-area triangles
rather than silently accepting or rejecting them through a universal policy.

## Materials and source comparison

A GLB can contain material slots, PBR material definitions, textures, and mesh
geometry, but they do not become one cooked Unreal object. Vertex and index
data become part of the `StaticMesh`; imported materials and textures become
separate Unreal packages referenced by that mesh. A geometry-only producer
cannot retain those references safely unless it also assigns deterministic
package identities and emits every generated dependency.

The current repository producer therefore preserves slot names but does not
cook materials or textures imported from the GLB. Engine-default replacement
and explicit references to already existing Unreal materials are predictable
contracts; neither recreates the embedded GLB appearance. An external material
reference can be serialized correctly even when an offline reader cannot
resolve the referenced package. Validate reference identity and ensure the
final game installation supplies every non-stock dependency separately.

Source-to-cooked comparison must state its coordinate, transform, instancing,
attribute, and compression assumptions. The current repository comparison path
uses local untransformed GLB geometry and caller-selected tolerances. Evidence
from that path must not be generalized to transformed nodes, instances, sparse
accessors, Draco compression, collision, shader behavior, or runtime rendering.

Use the focused [StaticMesh tool guide](../tools/static-mesh-assets.md) for the
public preparation and inspection commands.
