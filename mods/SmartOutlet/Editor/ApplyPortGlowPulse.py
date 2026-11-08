"""Focused one-time material update; run through the shared Unreal queue.

The saved material remains authoritative. This script is not a build step and
refuses an already modified emissive graph. Blueprint logic is unchanged.
"""
import unreal

library = unreal.MaterialEditingLibrary
material = unreal.load_asset('/Game/Mods/SmartOutlet/M_PortGlow')
assert material, 'Maintained glow material missing'
emissive = library.get_material_property_input_node(material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
assert isinstance(emissive, unreal.MaterialExpressionMultiply), 'Unexpected emissive graph'
inputs = library.get_inputs_for_material_expression(material, emissive)
colors = [node for node in inputs if isinstance(node, unreal.MaterialExpressionVectorParameter)
          and str(node.get_editor_property('parameter_name')) == 'ColorTint']
strengths = [node for node in inputs if isinstance(node, unreal.MaterialExpressionScalarParameter)
             and str(node.get_editor_property('parameter_name')) == 'GlowStrength']
assert len(colors) == len(strengths) == 1, 'Expected original ColorTint * GlowStrength; pulse may already be applied'
color = colors[0]


def node(kind, x, y, **properties):
    expression = library.create_material_expression(material, kind, x, y)
    for key, value in properties.items():
        expression.set_editor_property(key, value)
    return expression


def wire(source, output, target, input_name):
    assert library.connect_material_expressions(source, output, target, input_name), input_name


green = node(unreal.MaterialExpressionScalarParameter, -1300, 450,
             parameter_name='GreenPulseSeconds', default_value=6.0, slider_min=1.0, slider_max=12.0)
red = node(unreal.MaterialExpressionScalarParameter, -1300, 650,
           parameter_name='RedPulseSeconds', default_value=2.0, slider_min=1.0, slider_max=12.0)
period = node(unreal.MaterialExpressionIf, -1000, 400)
names = list(library.get_material_expression_input_names(period))
assert names[:2] == ['A', 'B'] and len(names) >= 5, names
# If inputs follow A, B, AGreaterThanB, AEqualsB, ALessThanB.
wire(color, 'G', period, names[0])
wire(color, 'R', period, names[1])
wire(green, '', period, names[2])
wire(green, '', period, names[3])
wire(red, '', period, names[4])

time = node(unreal.MaterialExpressionTime, -1000, 150, ignore_pause=False)
phase = node(unreal.MaterialExpressionDivide, -750, 350)
wire(time, '', phase, 'A')
wire(period, '', phase, 'B')
sine = node(unreal.MaterialExpressionSine, -550, 350, period=1.0)
wire(phase, '', sine, '')
amplitude = node(unreal.MaterialExpressionMultiply, -350, 350, const_b=0.475)
wire(sine, '', amplitude, 'A')
envelope = node(unreal.MaterialExpressionAdd, -150, 350, const_b=0.525)
wire(amplitude, '', envelope, 'A')
pulsed_emissive = node(unreal.MaterialExpressionMultiply, 100, 100)
wire(emissive, '', pulsed_emissive, 'A')
wire(envelope, '', pulsed_emissive, 'B')
assert library.connect_material_property(pulsed_emissive, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)

errors = library.recompile_material(material)
assert not errors, errors
assert unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
unreal.log('SmartOutlet: glow pulse applied; green 6s, red 2s, brightness 5-100%, material Time only')
