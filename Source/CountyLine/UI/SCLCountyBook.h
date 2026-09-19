#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class ACLPlayerController;
class SVerticalBox;

class SCLCountyBook : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCLCountyBook) : _FieldAction(-1) {} SLATE_ARGUMENT(ACLPlayerController*, Owner) SLATE_ARGUMENT(bool, ReportCover) SLATE_ARGUMENT(bool, Pause) SLATE_ARGUMENT(bool, Conversation) SLATE_ARGUMENT(int32, FieldAction) SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnPreviewKeyDown(const FGeometry&, const FKeyEvent& Event) override;
    virtual FReply OnAnalogValueChanged(const FGeometry&, const FAnalogInputEvent& Event) override;
    TSharedPtr<SWidget> InitialFocus() const { return FirstButton; }
private:
    TWeakObjectPtr<ACLPlayerController> Owner;
    TSharedPtr<SVerticalBox> Page;
    int32 ActivePage = 2;
    bool bCover = false;
    bool bPause = false;
    bool bConversation = false;
    int32 ConversationStep = 0;
    int32 FieldAction = -1;
    double NextAnalogNavigation = 0;
    double NextHorizontalNavigation = 0;
    FString Notice;
    mutable TSharedPtr<SWidget> FirstButton;
    mutable TArray<TSharedPtr<SWidget>> Controls;
    mutable TArray<TFunction<void()>> ControlActions;
    TSharedPtr<class SScrollBox> Scroll;
    int32 RestoreFocusIndex = 0;
    int32 FocusedIndex() const;
    void MoveFocus(int32 Direction);
    TSharedRef<SWidget> Control(TSharedRef<SWidget> Widget, TFunction<void()> Action, bool bEnabled = true) const;
    void FocusFirst();
    void Rebuild();
    TSharedRef<SWidget> Text(const FString& Copy, int32 Size = 22, bool bMuted = false) const;
    TSharedRef<SWidget> Button(const FString& Copy, TFunction<void()> Action, bool bEnabled = true) const;
    void Line(const FString& Copy, int32 Size = 22, bool bMuted = false);
};
