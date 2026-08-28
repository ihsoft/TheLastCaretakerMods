class UBP_ButtonInfo_Vertical_Container_C : public UVoyageButtonInfoContainerWidget
{
public:
    class UWidgetTree* WidgetTree = "WidgetTree'/Game/UI/Game/Inputs/BP_ButtonInfo_Vertical_Container.BP_ButtonInfo_Vertical_Container_C:WidgetTree'";
    bool bClassRequiresNativeTick = true;
    TMap<FName, struct FGuid> CookedPropertyGuids = {
        { FName("BP_ButtonInfo"), FGuid(0x0CB97D6D, 0xA96931E9, 0xB0BDD8D8, 0xAD6D84F8) }
    };
    TMap<enum, class UVoyageInputPlatformConfig*> PlatformConfigurations = {
        { EInputPlatformType::MouseKeyboard, "VoyageInputPlatformConfig'/Game/UI/PlatformInputs/DA_Platform_Config_Desktop.DA_Platform_Config_Desktop'" }, 
        { EInputPlatformType::Gamepad, "VoyageInputPlatformConfig'/Game/UI/PlatformInputs/DA_Platform_Config_Desktop.DA_Platform_Config_Desktop'" }, 
        { EInputPlatformType::Xbox, "VoyageInputPlatformConfig'/Game/UI/PlatformInputs/DA_Platform_Config_Xbox.DA_Platform_Config_Xbox'" }, 
        { EInputPlatformType::PS, "VoyageInputPlatformConfig'/Game/UI/PlatformInputs/DA_Platform_Config_PS.DA_Platform_Config_PS'" }, 
        { EInputPlatformType::Switch, "VoyageInputPlatformConfig'/Game/UI/PlatformInputs/DA_Platform_Config_Switch.DA_Platform_Config_Switch'" }
    };
    class UTexture2D* EmptyIcon = "Texture2D'/Game/External/ControllerIconPack/Platform-PC/Light/T_PC_Light_128x128.T_PC_Light_128x128'";
    bool bShowBackground = false;
    bool bHasScriptImplementedTick = false;
    bool bHasScriptImplementedPaint = false;
    class UBP_ButtonInfo_C* BP_ButtonInfo;
};