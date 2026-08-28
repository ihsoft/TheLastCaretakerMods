class UBP_ButtonInfoDetailed_Vertical_C : public UVoyageButtonInfoWidget
{
public:
    class UWidgetTree* WidgetTree = "WidgetTree'/Game/UI/Game/Inputs/BP_ButtonInfoDetailed_Vertical.BP_ButtonInfoDetailed_Vertical_C:WidgetTree'";
    bool bClassRequiresNativeTick = true;
    TArray<class UWidgetAnimation*> Animations = {
        "WidgetAnimation'/Game/UI/Game/Inputs/BP_ButtonInfoDetailed_Vertical.BP_ButtonInfoDetailed_Vertical_C:HoverAnim_INST'"
    };
    class UFunction* UberGraphFunction = "Function'/Game/UI/Game/Inputs/BP_ButtonInfoDetailed_Vertical.BP_ButtonInfoDetailed_Vertical_C:ExecuteUbergraph_BP_ButtonInfoDetailed_Vertical'";
    TMap<FName, struct FGuid> CookedPropertyGuids = {
        { FName("BackgroundImageSecondary"), FGuid(0x7A81DB6B, 0x43261BB2, 0x58C60EA9, 0x8C998E74) }, 
        { FName("TextBox"), FGuid(0xFF793F85, 0x0F91322C, 0x87EC6C41, 0x01152BBE) }
    };
    struct FPointerToUberGraphFrame UberGraphFrame = {};
    bool bHasScriptImplementedTick = false;
    bool bHasScriptImplementedPaint = false;
    class UHorizontalBox* TextBox;
    class UImage* BackgroundImageSecondary;

};