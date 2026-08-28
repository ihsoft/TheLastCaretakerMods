class UBP_CharacterInputHintWidget_C : public UUserWidget
{
public:
    class UWidgetTree* WidgetTree = "WidgetTree'/Game/UI/Game/Inputs/BP_CharacterInputHintWidget.BP_CharacterInputHintWidget_C:WidgetTree'";
    bool bHasScriptImplementedTick = false;
    bool bHasScriptImplementedPaint = false;
    class UBP_ButtonInfo_Vertical_Container_C* BP_ButtonInfoContainer;
};