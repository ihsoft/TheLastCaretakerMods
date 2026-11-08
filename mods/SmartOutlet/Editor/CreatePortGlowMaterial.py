"""One-time focused authoring step; never called by the release builder.

Run through Invoke-UnrealQueued.ps1 with Unreal's Python commandlet. The saved
material in Content is authoritative. Refuse to overwrite an existing asset.
"""
import unreal

package = '/Game/Mods/SmartOutlet'
name = 'M_PortGlow'
assert not unreal.EditorAssetLibrary.does_asset_exist(package + '/' + name), 'Material already exists; edit the maintained asset instead'
material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, package, unreal.Material, unreal.MaterialFactoryNew())
assert material
material.set_editor_property('used_with_nanite', True)
library = unreal.MaterialEditingLibrary

color = library.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -700, -100)
color.set_editor_property('parameter_name', 'ColorTint')
color.set_editor_property('default_value', unreal.LinearColor(1.0, 0.02, 0.02, 1.0))

strength = library.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -700, 150)
strength.set_editor_property('parameter_name', 'GlowStrength')
strength.set_editor_property('default_value', 2.0)
strength.set_editor_property('slider_min', 0.0)
strength.set_editor_property('slider_max', 5.0)

emissive = library.create_material_expression(material, unreal.MaterialExpressionMultiply, -350, 100)
assert library.connect_material_expressions(color, 'RGB', emissive, 'A')
assert library.connect_material_expressions(strength, '', emissive, 'B')
assert library.connect_material_property(emissive, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)

# Retain subdued surface shading instead of turning the whole rim white.
base = library.create_material_expression(material, unreal.MaterialExpressionMultiply, -350, -180)
base.set_editor_property('const_b', 0.12)
assert library.connect_material_expressions(color, 'RGB', base, 'A')
assert library.connect_material_property(base, '', unreal.MaterialProperty.MP_BASE_COLOR)
for field, value, y in [(unreal.MaterialProperty.MP_METALLIC, 0.25, 350),
                        (unreal.MaterialProperty.MP_ROUGHNESS, 0.5, 500)]:
    node = library.create_material_expression(material, unreal.MaterialExpressionConstant, -350, y)
    node.set_editor_property('r', value)
    assert library.connect_material_property(node, '', field)

errors = library.recompile_material(material)
assert not errors, errors
assert unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log('SmartOutlet: authored M_PortGlow, ColorTint * GlowStrength (2.0), lit opaque surface')
