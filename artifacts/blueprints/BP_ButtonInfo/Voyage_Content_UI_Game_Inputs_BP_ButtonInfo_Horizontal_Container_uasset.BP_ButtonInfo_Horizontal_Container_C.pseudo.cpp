class UBP_ButtonInfo_Horizontal_Container_C : public UVoyageButtonInfoContainerWidget
{
public:
    class UWidgetTree* WidgetTree = "WidgetTree'/Game/UI/Game/Inputs/BP_ButtonInfo_Horizontal_Container.BP_ButtonInfo_Horizontal_Container_C:WidgetTree'";
    bool bClassRequiresNativeTick = true;
    TMap<FName, struct FGuid> CookedPropertyGuids = {
        { FName("BackGroundColor"), FGuid(0x8AAFF959, 0x47688D4D, 0x2F086389, 0x11BB40E2) }, 
        { FName("BP_ButtonInfo"), FGuid(0x53B597B4, 0x8D173106, 0xB8C8DB02, 0x404182DB) }
    };
    struct FLinearColor BackGroundColor = FLinearColor(0.513889, 0.513889, 0.513889, 1);
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