# GLB support tools index

This directory contains source-only GLB inspection, selection, preview, validation,
and Blender conversion helpers. Choose the operation in [GLB tools](../glb-tools.md)
and invoke the documented script directly.

The inspectors never execute `extras`; preview and conversion require Blender,
while structural inspection and face selection use Python. Inspection outputs
must not replace their source GLB merely to inspect it; explicit selection
creates a separate authoring input.
