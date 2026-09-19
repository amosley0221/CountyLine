#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class ACLPlayerController;
class SVerticalBox;

class SCLCountyBook : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCLCountyBook) {} SLATE_ARGUMENT(ACLPlayerController*, Owner) SLATE_ARGUMENT(bool, ReportCover) SLATE_ARGUMENT(bool, Pause) SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnPreviewKeyDown(const FGeometry&, const FKeyEvent& Event) override;
    TSharedPtr<SWidget> InitialFocus() const { return FirstButton; }
private:
    TWeakObjectPtr<ACLPlayerController> Owner;
    TSharedPtr<SVerticalBox> Page;
    int32 ActivePage = 2;
    bool bCover = false;
    bool bPause = false;
    FString Notice;
    mutable TSharedPtr<SWidget> FirstButton;
    void FocusFirst();
    void Rebuild();
    TSharedRef<SWidget> Text(const FString& Copy, int32 Size = 22, bool bMuted = false) const;
    TSharedRef<SWidget> Button(const FString& Copy, TFunction<void()> Action, bool bEnabled = true) const;
    void Line(const FString& Copy, int32 Size = 22, bool bMuted = false);
};
