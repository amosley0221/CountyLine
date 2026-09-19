#include "UI/SCLCountyBook.h"
#include "Player/CLPlayerController.h"
#include "Paper/CLCaseState.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/Paths.h"
#include "Fonts/CompositeFont.h"
#include "Brushes/SlateColorBrush.h"

// Keep left-stick navigation in the book's ordered control list. Slate's default
// focusable widgets consume analog events before they can bubble to the book.
class SCLBookButton : public SButton
{
public:
    virtual FReply OnAnalogValueChanged(const FGeometry& Geometry,const FAnalogInputEvent& Event) override
    {
        return (Event.GetKey()==EKeys::Gamepad_LeftY || Event.GetKey()==EKeys::Gamepad_LeftX)?FReply::Unhandled():SButton::OnAnalogValueChanged(Geometry,Event);
    }
};

class SCLBookCheckBox : public SCheckBox
{
public:
    virtual FReply OnAnalogValueChanged(const FGeometry& Geometry,const FAnalogInputEvent& Event) override
    {
        return (Event.GetKey()==EKeys::Gamepad_LeftY || Event.GetKey()==EKeys::Gamepad_LeftX)?FReply::Unhandled():SCheckBox::OnAnalogValueChanged(Geometry,Event);
    }
};

namespace CLPaper
{
    const FLinearColor Cream = FLinearColor::FromSRGBColor(FColor(239,229,204));
    const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(42,37,32));
    const FLinearColor Carbon = FLinearColor::FromSRGBColor(FColor(62,56,80));
    const FLinearColor Dust = FLinearColor::FromSRGBColor(FColor(200,180,140));
}

TSharedRef<SWidget> SCLCountyBook::Text(const FString& Copy, int32 Size, bool bMuted) const
{
    const bool bTyped = !Owner.IsValid() || !Owner->Case() || Owner->Case()->bTypedCopy;
    const FString Face=Size>=30?TEXT("OldStandardTT-Regular.ttf"):(bTyped?TEXT("CourierPrime-Regular.ttf"):TEXT("Caveat.ttf"));
    static TMap<FString,TSharedPtr<const FCompositeFont>> Fonts;
    if(!Fonts.Contains(Face)) Fonts.Add(Face,MakeShared<FCompositeFont>(FName("Regular"),FPaths::ProjectContentDir()/TEXT("UI/Fonts")/Face,EFontHinting::Default,EFontLoadingPolicy::LazyLoad));
    return SNew(STextBlock).Text(FText::FromString(Copy))
        .Font(FSlateFontInfo(Fonts[Face],Size+(bTyped||Size>=30?0:4)))
        .ColorAndOpacity(bMuted ? CLPaper::Carbon : CLPaper::Ink).AutoWrapText(true);
}

TSharedRef<SWidget> SCLCountyBook::Button(const FString& Copy, TFunction<void()> Action, bool bEnabled) const
{
    static const FButtonStyle Style=FButtonStyle()
        .SetNormal(FSlateColorBrush(CLPaper::Dust))
        .SetHovered(FSlateColorBrush(FLinearColor(0.76f,0.66f,0.44f)))
        .SetPressed(FSlateColorBrush(FLinearColor(0.64f,0.53f,0.33f)))
        .SetDisabled(FSlateColorBrush(FLinearColor(0.68f,0.64f,0.54f)))
        .SetNormalForeground(CLPaper::Ink).SetHoveredForeground(CLPaper::Ink).SetPressedForeground(CLPaper::Ink).SetDisabledForeground(CLPaper::Carbon);
    TSharedRef<SWidget> Result=SNew(SCLBookButton).ButtonStyle(&Style).IsEnabled(bEnabled).ContentPadding(FMargin(16,10))
        .ButtonColorAndOpacity(FLinearColor::White).ForegroundColor(CLPaper::Ink)
        .OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); })[Text(Copy,19)];
    return Control(Result,Action,bEnabled);
}

TSharedRef<SWidget> SCLCountyBook::Control(TSharedRef<SWidget> Widget,TFunction<void()> Action,bool bEnabled) const
{
    if(bEnabled)
    {
        Controls.Add(Widget);
        ControlActions.Add(MoveTemp(Action));
        if(!FirstButton.IsValid()) FirstButton=Widget;
    }
    TWeakPtr<SWidget> WeakWidget=Widget;
    return SNew(SBorder).Padding(3).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor_Lambda([WeakWidget]{auto P=WeakWidget.Pin();return P.IsValid() && P->HasKeyboardFocus()?FLinearColor(0.25f,0.15f,0.045f):FLinearColor::Transparent;})
        [Widget];
}

int32 SCLCountyBook::FocusedIndex() const
{
    for(int32 I=0;I<Controls.Num();++I)
        if(Controls[I]->HasKeyboardFocus()) return I;
    return 0;
}

void SCLCountyBook::MoveFocus(int32 Direction)
{
    if(Controls.IsEmpty()) return;
    const int32 Index=(FocusedIndex()+Direction+Controls.Num())%Controls.Num();
    FSlateApplication::Get().SetAllUserFocus(Controls[Index],EFocusCause::Navigation);
    if(Scroll.IsValid()) Scroll->ScrollDescendantIntoView(Controls[Index],false,EDescendantScrollDestination::IntoView);
}

void SCLCountyBook::FocusFirst()
{
    if(Owner.IsValid() && Owner->IsBookOpen() && !Controls.IsEmpty())
    {
        const auto Target=Controls[FMath::Clamp(RestoreFocusIndex,0,Controls.Num()-1)];
        FSlateApplication::Get().SetAllUserFocus(Target,EFocusCause::SetDirectly);
        if(Scroll.IsValid()) Scroll->ScrollDescendantIntoView(Target,false,EDescendantScrollDestination::IntoView);
    }
}

void SCLCountyBook::Line(const FString& Copy, int32 Size, bool bMuted)
{
    Page->AddSlot().AutoHeight().Padding(0,0,0,16)[Text(Copy,Size,bMuted)];
}

void SCLCountyBook::Construct(const FArguments& Args)
{
    Owner = Args._Owner;
    bCover = Args._ReportCover;
    bPause = Args._Pause;
    bConversation = Args._Conversation;
    ChildSlot
    [
        SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.015f,0.012f,0.008f,0.70f)).Padding(28)
        [
            SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
            [
                SNew(SBox).WidthOverride(1160).HeightOverride(800)
                [
                    SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(CLPaper::Cream).Padding(FMargin(44,32))
                    [ SAssignNew(Page,SVerticalBox) ]
                ]
            ]
        ]
    ];
    Rebuild();
}

void SCLCountyBook::Rebuild()
{
    if (!Owner.IsValid() || !Owner->Case()) return;
    UCLCaseState* State=Owner->Case();
    auto& Report=State->Report;
    RestoreFocusIndex=FocusedIndex();
    FirstButton.Reset();
    Controls.Reset();
    ControlActions.Reset();
    Scroll.Reset();
    Page->ClearChildren();
    if(bConversation)
    {
        Line(TEXT("JAIL OFFICE  /  DEPUTY PRUITT"),18,true);
        Line(TEXT("Before the ink dries"),42);
        if(Report.Status!=ECLReportStatus::Draft)
        {
            Line(Report.Status==ECLReportStatus::Signed?TEXT("PRUITT\nYour signature is on it, Sheriff. The carbon stays with the case."):TEXT("PRUITT\nHeld for inquiry, then. We'll need more than what's on that sheet."),26);
            Line(State->IsCurrentStateSaved()?TEXT("The date is written. The book will keep it."):TEXT("Write the date at the desk before you go."),24,true);
        }
        else if(ConversationStep==0)
        {
            Line(TEXT("PRUITT\nSheriff. The Bend Lateral report is on your desk. Salazar found the man. There's a bottle mentioned in the account."),26);
            Line(TEXT("REED\nAnd that's all we have?"),24,true);
            Page->AddSlot().AutoHeight().Padding(0,16)[Button(TEXT("HEAR HIM OUT"),[this]{ConversationStep=1;Owner->Case()->Report.bBriefedByPruitt=true;Rebuild();})];
        }
        else
        {
            Line(TEXT("PRUITT\nThat's what's on paper. It doesn't tell us how the man died. You can sign the facts we have, or hold the report for inquiry."),26);
            Line(TEXT("REED\nI'll read it before I put my name to it."),24,true);
            Line(TEXT("PRUITT\nThe cream sheet on the desk. Enter it in the County Book, then write the date when you're done."),24);
        }
        Page->AddSlot().FillHeight(1)[SNew(SSpacer)];
        const bool bFinished=ConversationStep>0 || Report.Status!=ECLReportStatus::Draft;
        Page->AddSlot().AutoHeight()[Button(bFinished?TEXT("RETURN TO THE OFFICE"):TEXT("I'LL COME BACK"),[this]{Owner->CloseBook();})];
        Line(TEXT("D-pad / left stick: choose     A / Enter: select     B / Escape: leave"),17,true);
        FocusFirst();
        return;
    }
    if(bPause)
    {
        TSharedPtr<SVerticalBox> Frame=Page;
        TSharedRef<SVerticalBox> Menu=SNew(SVerticalBox);
        Frame->AddSlot().FillHeight(1)[SAssignNew(Scroll,SScrollBox)+SScrollBox::Slot()[Menu]];
        Page=Menu;
        Line(TEXT("THE COUNTY LINE"),18,true);
        Line(TEXT("Jail office"),40);
        Line(TEXT("Pecos Bend  /  October 1926\nStandalone playable study"),22,true);
        if(!Notice.IsEmpty()) Line(Notice,18);
        Page->AddSlot().AutoHeight().Padding(0,10)[Button(TEXT("BACK TO IT"),[this]{Owner->CloseBook();})];
        Page->AddSlot().AutoHeight().Padding(0,10)[Button(TEXT("COUNTY BOOK"),[this]{bPause=false; ActivePage=0; Rebuild();})];
        Page->AddSlot().AutoHeight().Padding(0,10)[Button(TEXT("START AGAIN  (UNSAVED)"),[this]{Owner->Case()->Report=FCLReportState{}; Notice=TEXT("A fresh report waits on the desk. Your saved date remains until you write a new one.");Rebuild();})];
        Page->AddSlot().AutoHeight().Padding(0,10)[Button(Owner->IsAtDesk()?TEXT("WRITE THE DATE"):TEXT("WRITE THE DATE  (return to the desk)"),[this]{Notice=Owner->Case()->WriteDate()?TEXT("Date written. This report will be here when you return."):TEXT("The date could not be written. Try again."); Rebuild();},Owner->IsAtDesk())];
        Page->AddSlot().AutoHeight().Padding(0,10)[Button(TEXT("QUIT TO DESK"),[this]{Owner->ConsoleCommand(TEXT("quit"));})];
        Line(TEXT("WASD / left stick: walk     Mouse / right stick: look\nE / A: interact     Tab / View: County Book\nD-pad / left stick: choose     A: select     B: back\nLB / RB: book pages     Menu: pause\nSave at the desk before leaving."),20,true);
        Page=Frame;
        FocusFirst();
        return;
    }
    if(bCover)
    {
        Line(TEXT("RIVAS COUNTY  /  ACTING SHERIFF S. REED"),18,true);
        Line(TEXT("A report on the desk."),42);
        Line(TEXT("26-001     BEND LATERAL     /     UNSIGNED"),22,true);
        Line(TEXT("A man was found at Bend Lateral.\nSalazar found him. A bottle was reported at the scene.\n\nThat is what the paper says. It is not yet your signature."),26);
        Line(TEXT("Open the County Book to decide what goes on record.\nOnly known facts may be included. Omissions are kept with the case."),22,true);
        Page->AddSlot().FillHeight(1)[SNew(SSpacer)];
        Page->AddSlot().AutoHeight()[Button(TEXT("ENTER IN COUNTY BOOK"),[this]{Owner->Case()->Report.bRead=true;bCover=false;ActivePage=2;Rebuild();})];
        Page->AddSlot().AutoHeight().Padding(0,12,0,0)[Button(TEXT("LEAVE IT ON THE DESK"),[this]{Owner->CloseBook();})];
        FocusFirst();
        return;
    }
    Page->AddSlot().AutoHeight()
    [SNew(SHorizontalBox)
        +SHorizontalBox::Slot().FillWidth(1)[Text(TEXT("The County Book"),38)]
        +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("CLOSE  /  B"),[this]{Owner->CloseBook();})]];
    Line(TEXT("S. REED     /     PECOS BEND     /     OCTOBER 1926"),17,true);
    const TCHAR* Names[]={TEXT("LEDGER"),TEXT("MAP"),TEXT("CASES"),TEXT("PEOPLE"),TEXT("PAYROLL")};
    TSharedRef<SHorizontalBox> Tabs=SNew(SHorizontalBox);
    for(int32 I=0;I<5;++I)
        Tabs->AddSlot().FillWidth(1).Padding(0,0,6,18)[Button(FString::Printf(TEXT("%s%s"),ActivePage==I?TEXT("— "):TEXT(""),Names[I]),[this,I]{ActivePage=I; Notice.Empty(); Rebuild();})];
    Page->AddSlot().AutoHeight()[Tabs];
    // A scrolling page keeps all choices reachable at small viewport sizes.
    TSharedPtr<SVerticalBox> Outer=Page;
    TSharedRef<SVerticalBox> Body=SNew(SVerticalBox);
    Outer->AddSlot().FillHeight(1)[SAssignNew(Scroll,SScrollBox)+SScrollBox::Slot()[Body]];
    Page=Body;
    if(ActivePage==2)
    {
        Line(TEXT("26-001  /  Bend Lateral"),30);
        Line(UCLCaseState::StatusText(Report.Status),19,true);
        if(!Report.bRead)
        {
            Line(TEXT("The unsigned report is still on the jail-office desk.\nApproach it and press E / A to read it."),24);
        }
        else
        {
            const bool bEditable=Report.Status==ECLReportStatus::Draft && Owner->IsAtDesk();
            Line(TEXT("KNOWN FACTS  /  unchecked facts are recorded as omitted"),18,true);
            const TCHAR* Facts[]={TEXT("Salazar found the man at Bend Lateral."),TEXT("A bottle was reported at the scene.")};
            for(int32 I=0;I<2;++I)
            {
                Page->AddSlot().AutoHeight().Padding(0,0,0,12)
                [Control(SNew(SCLBookCheckBox).HAlign(HAlign_Fill).IsEnabled(bEditable)
                    .IsChecked_Lambda([this,I]{const auto& R=Owner->Case()->Report;return (I==0?R.bIncludeFinder:R.bIncludeBottle)?ECheckBoxState::Checked:ECheckBoxState::Unchecked;})
                    .OnCheckStateChanged_Lambda([this,I](ECheckBoxState Value){auto& R=Owner->Case()->Report;(I==0?R.bIncludeFinder:R.bIncludeBottle)=Value==ECheckBoxState::Checked;Notice.Empty();Rebuild();})
                    [SNew(SBox).WidthOverride(990)[Text(Facts[I],22)]],
                    [this,I]{auto& R=Owner->Case()->Report;bool& Fact=I==0?R.bIncludeFinder:R.bIncludeBottle;Fact=!Fact;Notice.Empty();Rebuild();},bEditable)];
            }
            Line(TEXT("CLOSING LINE  /  choose one"),18,true);
            for(int32 I=0;I<3;++I)
                Page->AddSlot().AutoHeight().Padding(0,0,0,7)[Button(FString(Report.ClosingLine==I?TEXT("[x]  "):TEXT("[ ]  "))+UCLCaseState::ClosingLines[I],[this,I]{Owner->Case()->Report.ClosingLine=I;Notice.Empty();Rebuild();},bEditable)];
            Page->AddSlot().AutoHeight().Padding(0,14)
            [SNew(SHorizontalBox)
                +SHorizontalBox::Slot().AutoWidth().Padding(0,0,12,0)[Button(TEXT("SIGN"),[this]{if(Owner->IsAtDesk() && Owner->Case()->Report.Submit(ECLReportStatus::Signed)) Notice=TEXT("Signed. A carbon stays with the case. Write the date to save.");Rebuild();},bEditable)]
                +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("HOLD"),[this]{if(Owner->IsAtDesk() && Owner->Case()->Report.Submit(ECLReportStatus::Held)) Notice=TEXT("Held for inquiry. The clerk will notice. Write the date to save.");Rebuild();},bEditable)]];
            if(Report.Status!=ECLReportStatus::Draft)
                Line(FString::Printf(TEXT("Carbon copy: %d fact(s) included, %d omitted.\nAmendment awaits further evidence."),Report.IncludedFacts.Num(),Report.OmittedFacts.Num()),19,true);
            else if(!Owner->IsAtDesk()) Line(TEXT("Return to the desk to change or submit this report."),20,true);
        }
    }
    else if(ActivePage==0)
    {
        Line(TEXT("What the county remembers"),30);
        Line(TEXT("COURTHOUSE"),19,true);
        Line(Report.Status==ECLReportStatus::Signed?TEXT("The clerk has my signature. The report can go upstairs."):Report.Status==ECLReportStatus::Held?TEXT("I held the report. They wanted it closed."):TEXT("An unsigned report waits on the desk."));
        Line(TEXT("STREET"),19,true); Line(TEXT("Salazar found him. I have not heard him out."));
        Line(TEXT("CAPITAL"),19,true); Line(TEXT("No entry yet."));
        Line(TEXT("HOME"),19,true); Line(TEXT("A room at Lang's. The rest can wait."));
    }
    else if(ActivePage==1)
    {
        Line(TEXT("Rivas County"),30);
        Line(TEXT("County Clerk's Office  /  1927"),20,true);
        Line(TEXT("The county map is not yet mounted in this playable study.\n\nJail office — Pecos Bend.\nBend Lateral — a name on the report."),24);
    }
    else if(ActivePage==3)
    {
        Line(TEXT("People in the book"),30);
        Line(TEXT("SAM REED  /  Acting Sheriff\nThe name on the door is mine, for now."));
        Line(TEXT("PRUITT  /  Deputy\nWaiting in the jail office. He brought the report to my attention."));
        Line(TEXT("INEZ PADILLA  /  County Clerk\nShe will see the carbon before Helm does."));
        Line(TEXT("SALAZAR  /  Commissioner\nRiver Road. He found the man at the lateral."));
    }
    else
    {
        Line(TEXT("County payroll"),30);
        Line(TEXT("ACTING SHERIFF S. REED"),22,true);
        Line(TEXT("Appropriation pending entry.\n\nPayroll, deputies, and county property are not simulated in this office study."),24);
    }
    Page=Outer;
    if(!Notice.IsEmpty()) Line(Notice,18);
    Page->AddSlot().AutoHeight().Padding(0,16,0,0)
    [SNew(SHorizontalBox)
        +SHorizontalBox::Slot().FillWidth(1)
        [Control(SNew(SCLBookCheckBox).HAlign(HAlign_Fill).IsChecked(State->bTypedCopy?ECheckBoxState::Checked:ECheckBoxState::Unchecked)
            .OnCheckStateChanged_Lambda([this](ECheckBoxState V){Owner->Case()->bTypedCopy=V==ECheckBoxState::Checked;Notice.Empty();Rebuild();})[Text(TEXT("Clerk's typed copy"),18)],
            [this]{Owner->Case()->bTypedCopy=!Owner->Case()->bTypedCopy;Notice.Empty();Rebuild();})]
        +SHorizontalBox::Slot().AutoWidth()
        [Button(Owner->IsAtDesk()?TEXT("WRITE THE DATE"):TEXT("WRITE THE DATE  (at desk only)"),[this]{Notice=Owner->Case()->WriteDate()?TEXT("Date written. Your report and copy preference are saved."):TEXT("Could not write the date.");Rebuild();},Owner->IsAtDesk())]];
    Line(TEXT("D-pad / left stick: choose    A / Enter: select    B: close    LB / RB: pages"),16,true);
    FocusFirst();
}

FReply SCLCountyBook::OnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    const FKey Key=Event.GetKey();
    if(Key==EKeys::Tab || Key==EKeys::Escape || Key==EKeys::Gamepad_FaceButton_Right || Key==EKeys::Gamepad_Special_Left || Key==EKeys::Gamepad_Special_Right)
    { Owner->CloseBook(); return FReply::Handled(); }
    if(Key==EKeys::Gamepad_DPad_Down || Key==EKeys::Gamepad_DPad_Right || Key==EKeys::Down || Key==EKeys::Right)
    {MoveFocus(1);return FReply::Handled();}
    if(Key==EKeys::Gamepad_DPad_Up || Key==EKeys::Gamepad_DPad_Left || Key==EKeys::Up || Key==EKeys::Left)
    {MoveFocus(-1);return FReply::Handled();}
    if(Key==EKeys::Gamepad_FaceButton_Bottom || Key==EKeys::Enter || Key==EKeys::SpaceBar)
    {
        const int32 Index=FocusedIndex();
        if(!Event.IsRepeat() && ControlActions.IsValidIndex(Index))
        {
            // Copy before invoking: the action may rebuild and replace the entire control list.
            TFunction<void()> Action=ControlActions[Index];
            Action();
        }
        return FReply::Handled();
    }
    if(!bCover && !bPause && !bConversation && (Key==EKeys::Gamepad_LeftShoulder || Key==EKeys::Gamepad_RightShoulder))
    {ActivePage=(ActivePage+(Key==EKeys::Gamepad_LeftShoulder?4:1))%5;Rebuild();return FReply::Handled();}
    return FReply::Unhandled();
}

FReply SCLCountyBook::OnAnalogValueChanged(const FGeometry&,const FAnalogInputEvent& Event)
{
    const bool bVertical=Event.GetKey()==EKeys::Gamepad_LeftY;
    if(!bVertical && Event.GetKey()!=EKeys::Gamepad_LeftX) return FReply::Unhandled();
    double& NextNavigation=bVertical?NextAnalogNavigation:NextHorizontalNavigation;
    const float Value=Event.GetAnalogValue();
    if(FMath::Abs(Value)<0.3f) {NextNavigation=0;return FReply::Handled();}
    const double Now=FSlateApplication::Get().GetCurrentTime();
    if(FMath::Abs(Value)>0.55f && Now>=NextNavigation)
    {
        MoveFocus((Value>0?1:-1)*(bVertical?-1:1));
        NextNavigation=Now+0.22;
    }
    return FReply::Handled();
}
