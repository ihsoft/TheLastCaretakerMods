class UBP_ButtonInfoLarge_C : public UVoyageButtonInfoWidget
{
public:
    class UWidgetTree* WidgetTree = "WidgetTree'/Game/UI/Game/Inputs/BP_ButtonInfoLarge.BP_ButtonInfoLarge_C:WidgetTree'";
    bool bClassRequiresNativeTick = true;
    TArray<class UWidgetAnimation*> Animations = {
        "WidgetAnimation'/Game/UI/Game/Inputs/BP_ButtonInfoLarge.BP_ButtonInfoLarge_C:HoverAnim_INST'"
    };
    bool bHasScriptImplementedTick = false;
    bool bHasScriptImplementedPaint = false;
};