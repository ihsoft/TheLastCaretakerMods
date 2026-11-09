"""Create editor-only object identities used by the runtime mode hint.

Steam25982825 / UE5.8: /Game/Game/Input/Character/IAV_InteractThree is a
Boolean input action. No mappings, stock asset payload or runtime override.
The producer's explicit package inventory excludes this compilation stub.
"""
import unreal

path = '/Game/Game/Input/Character/IAV_InteractThree'
asset = unreal.load_asset(path)
if not asset:
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'IAV_InteractThree', '/Game/Game/Input/Character',
        unreal.InputAction, unreal.InputAction_Factory())
    assert asset
    unreal.EditorAssetLibrary.save_loaded_asset(asset)

assert isinstance(asset, unreal.InputAction)
assert asset.get_editor_property('value_type') == unreal.InputActionValueType.BOOLEAN

# Leaf card identity; the game's real asset supplies its full widget tree.
leaf = '/Game/UI/Game/Inputs/BP_ButtonInfoDetailed_Vertical'
if not unreal.load_asset(leaf):
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class', unreal.VoyageButtonInfoWidget)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'BP_ButtonInfoDetailed_Vertical', '/Game/UI/Game/Inputs',
        unreal.WidgetBlueprint, factory)
    assert asset
    unreal.EditorAssetLibrary.set_metadata_tag(asset, 'BlueprintText.Role', 'stub')
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)

context = '/Game/Game/Input/Character/DA_Input_Context_Character'
if not unreal.load_asset(context):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.VoyageInputContextAsset)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'DA_Input_Context_Character', '/Game/Game/Input/Character',
        unreal.VoyageInputContextAsset, factory)
    assert asset
    unreal.EditorAssetLibrary.save_loaded_asset(asset)

# Verified from the stock WidgetBlueprintGeneratedClass.SuperStruct, not its name.
# Detailed_Vertical is a VoyageButtonInfoWidget leaf, NOT a SetActions container.
hint = '/Game/UI/Game/Inputs/BP_ButtonInfo_Vertical_Container'
if not unreal.load_asset(hint):
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class', unreal.VoyageButtonInfoContainerWidget)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'BP_ButtonInfo_Vertical_Container', '/Game/UI/Game/Inputs',
        unreal.WidgetBlueprint, factory)
    assert asset
    unreal.EditorAssetLibrary.set_metadata_tag(asset, 'BlueprintText.Role', 'stub')
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
