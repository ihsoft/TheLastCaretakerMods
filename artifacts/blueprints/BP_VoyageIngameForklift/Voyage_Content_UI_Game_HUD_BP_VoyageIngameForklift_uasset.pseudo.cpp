class UBP_VoyageIngameForklift_C : public UVoyageIngameForkliftWidget
{
public:
    class UWidgetTree* WidgetTree = "WidgetTree'/Game/UI/Game/HUD/BP_VoyageIngameForklift.BP_VoyageIngameForklift_C:WidgetTree'";
    bool bClassRequiresNativeTick = true;
    TArray<class UWidgetAnimation*> Animations = {
        "WidgetAnimation'/Game/UI/Game/HUD/BP_VoyageIngameForklift.BP_VoyageIngameForklift_C:RevealAnim_INST'",
        "WidgetAnimation'/Game/UI/Game/HUD/BP_VoyageIngameForklift.BP_VoyageIngameForklift_C:OnAnchorAnim_INST'",
        "WidgetAnimation'/Game/UI/Game/HUD/BP_VoyageIngameForklift.BP_VoyageIngameForklift_C:HideAnim_INST'"
    };
    class UFunction* UberGraphFunction = "Function'/Game/UI/Game/HUD/BP_VoyageIngameForklift.BP_VoyageIngameForklift_C:ExecuteUbergraph_BP_VoyageIngameForklift'";
    TMap<FName, struct FGuid> CookedPropertyGuids = {
        { FName("OnAnchorAnim"), FGuid(0xB686FC80, 0xE51B3213, 0x99DB5DF2, 0x5979B4AD) }, 
        { FName("BP_ButtonInfoContainer_Action"), FGuid(0x29D082C3, 0x342434E9, 0xB03119EA, 0x16CFCF56) }, 
        { FName("BP_DynamicPlayerInputHorizontalWidget_Bottom"), FGuid(0xD92BF38D, 0x4C8B8495, 0x2D6CF2AD, 0xB6AD8208) }, 
        { FName("BP_DynamicPlayerInputHorizontalWidget_Center"), FGuid(0x061F2BEF, 0x4C24F9EA, 0x651F459D, 0xECA48D83) }, 
        { FName("BP_LocatorHudWidget"), FGuid(0x11A00443, 0xFF9732BE, 0xA04FF0AB, 0x11872B33) }, 
        { FName("Image"), FGuid(0xDB10C029, 0x40345407, 0xC1791684, 0x9EFD310F) }, 
        { FName("InvalidationBox_0"), FGuid(0xF2113A7D, 0x4630EA34, 0x9F0624AC, 0x111CE101) }, 
        { FName("ItemDropArea"), FGuid(0x32A6F85A, 0xFFFA3F1F, 0x829A254B, 0x779512B2) }, 
        { FName("KeybindRoot"), FGuid(0x86FBF66B, 0xB3E93E43, 0xB4F7FE4B, 0x953B987F) }, 
        { FName("LocatorRoot"), FGuid(0x58B5176F, 0x42F24D03, 0x28884DBE, 0xD9F041C2) }, 
        { FName("RootCanvasPanel"), FGuid(0x62934EE9, 0xA047307A, 0x96741991, 0x60B4350B) }, 
        { FName("SpeedometerRoot"), FGuid(0xF1EA7CB3, 0x4942AD7B, 0x945575BC, 0x403B4987) }
    };
    struct FPointerToUberGraphFrame UberGraphFrame = {};
    enum ResourceType = EModuleResourceType::Petrol;
    bool bHasScriptImplementedTick = false;
    bool bHasScriptImplementedPaint = false;
    class UOverlay* SpeedometerRoot;
    class UCanvasPanel* RootCanvasPanel;
    class UOverlay* LocatorRoot;
    class UVerticalBox* KeybindRoot;
    class UBP_DropWidgetArea_C* ItemDropArea;
    class UInvalidationBox* InvalidationBox_0;
    class UImage* Image;
    class UBP_LocatorHudWidget_C* BP_LocatorHudWidget;
    class UBP_DynamicPlayerInputHorizontalWidget_C* BP_DynamicPlayerInputHorizontalWidget_Center;
    class UBP_DynamicPlayerInputHorizontalWidget_C* BP_DynamicPlayerInputHorizontalWidget_Bottom;
    class UBP_ButtonInfo_Vertical_Container_C* BP_ButtonInfoContainer_Action;
    class UWidgetAnimation* OnAnchorAnim;

};