"""Independent reflected fixture operations, never reads BPT JSON to check values."""
import json
import os
from pathlib import Path
import unreal

stage = os.environ['BPT_NATIVE_STAGE']
output = Path(os.environ['BPT_NATIVE_OUTPUT'])
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
path = '/Game/BP_NativeDefaults'

if stage == 'create':
    # Any external UObject asset exercises the reference; DataAsset itself is abstract.
    item_factory = unreal.CurveFloatFactory()
    item = asset_tools.create_asset('DA_NativeItem', '/Game', unreal.CurveFloat, item_factory)
    assert unreal.EditorAssetLibrary.save_loaded_asset(item)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.BPTNativeActor.static_class())
    bp = asset_tools.create_asset('BP_NativeDefaults', '/Game', unreal.Blueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    cdo = unreal.get_default_object(bp.generated_class())
    component = cdo.get_editor_property('module_component')
    assert component.get_editor_property('config_data').get_editor_property('resource_bandwidth_input') == 17
    assert component.get_editor_property('nested').get_editor_property('value') == 23
    component.set_editor_property('item_asset', item)
    config = component.get_editor_property('config_data')
    config.set_editor_property('auto_start_module', True)
    config.set_editor_property('resource_bandwidth_input', 1000.0)
    config.set_editor_property('max_resource_amount', 1)
    component.set_editor_property('config_data', config)
    component.get_editor_property('nested').set_editor_property('value', 41)
    assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
    child_factory = unreal.BlueprintFactory()
    child_factory.set_editor_property('parent_class', bp.generated_class())
    child = asset_tools.create_asset('BP_NativeDefaultsChild', '/Game', unreal.Blueprint, child_factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(child)
    unreal.get_default_object(child.generated_class()).get_editor_property('module_component').get_editor_property('nested').set_editor_property('value', 47)
    assert unreal.EditorAssetLibrary.save_loaded_asset(child)
else:
    expected_zero = stage == 'zero'
    checked = []
    for suffix in ('', 'Child'):
        bp = unreal.load_asset(path + suffix)
        cdo = unreal.get_default_object(bp.generated_class())
        component = cdo.get_editor_property('module_component')
        config = component.get_editor_property('config_data')
        item = component.get_editor_property('item_asset')
        assert (item is None) if expected_zero else item.get_path_name() == '/Game/DA_NativeItem.DA_NativeItem'
        assert config.get_editor_property('auto_start_module') == (not expected_zero)
        assert config.get_editor_property('resource_bandwidth_input') == (0 if expected_zero else 1000)
        assert config.get_editor_property('max_resource_amount') == (0 if expected_zero else 1)
        nested_value = component.get_editor_property('nested').get_editor_property('value')
        expected_nested = 47 if suffix else (0 if expected_zero else 41)
        assert nested_value == expected_nested, (bp.get_path_name(), 'Nested.Value', nested_value)
        assert component.get_editor_property('peer') == cdo.get_editor_property('other_component')
        assert cdo.get_editor_property('other_component').get_outer() == cdo
        assert component.get_outer() == cdo
        assert component.get_editor_property('nested').get_outer() == component
        checked.append(bp.get_path_name())
    (output / (stage + '-checked.json')).write_text(json.dumps({'status': 'passed', 'assets': checked}))
