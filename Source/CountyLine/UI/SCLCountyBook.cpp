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
        return (Event.GetKey()==EKeys::Gamepad_LeftY || Event.GetKey()==EKeys::Gamepad_LeftX || Event.GetKey()==EKeys::Gamepad_RightX || Event.GetKey()==EKeys::Gamepad_RightY)?FReply::Unhandled():SButton::OnAnalogValueChanged(Geometry,Event);
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
    FieldAction = Args._FieldAction;
    if(Owner.IsValid() && Owner->IsInspecting())
    {
        ChildSlot
        [SNew(SOverlay)
            +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(24)
            [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
                [SNew(SBox).WidthOverride(450).HeightOverride(800)
                    [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .BorderBackgroundColor(CLPaper::Cream).Padding(26)
                        [SAssignNew(Page,SVerticalBox)]]]]];
        Rebuild();return;
    }
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

void SCLCountyBook::Rebuild(int32 FocusOverride)
{
    if (!Owner.IsValid() || !Owner->Case()) return;
    UCLCaseState* State=Owner->Case();
    auto& Report=State->Report;
    RestoreFocusIndex=FocusOverride>=0?FocusOverride:FocusedIndex();
    FirstButton.Reset();
    Controls.Reset();
    ControlActions.Reset();
    Scroll.Reset();
    Page->ClearChildren();
    if(FieldAction>=0)
    {
        const bool bInspect=Owner->IsInspecting();
        Line(FieldAction==6?TEXT("PECOS BEND / NORTH LANE"):FieldAction==5?TEXT("PECOS BEND / LANG'S BOARDINGHOUSE"):TEXT("BEND LATERAL / FIELD STUDY"),18,true);
        const TCHAR* Titles[]={TEXT("Back to Pecos Bend"),TEXT("Salazar's account"),TEXT("A bottle by the bank"),TEXT("The ditch bank"),TEXT("The road to Bend Lateral"),TEXT("Rooms and board"),TEXT("A River Road account")};
        Line(Titles[FieldAction],bInspect?32:40);
        if(FieldAction==6)
        {
            if(Report.FieldNotes.Contains(TEXT("ResidentAccount")))
            {
                Line(TEXT("RESIDENT\nYou have what I can tell you, Sheriff. I use that path. I did not see what happened to the man."),24);
                Line(TEXT("The account is in Cases and People. Write the date at the jail desk to save it."),20,true);
            }
            else if(ConversationStep==0)
            {
                Line(TEXT("RESIDENT\nYou're asking about the lateral? I come in from River Road. We use the bank path to reach the fields."),24);
                Page->AddSlot().AutoHeight()[Button(TEXT("ASK: DID YOU SEE WHAT HAPPENED?"),[this]{ConversationStep=1;Rebuild(0);})];
                Line(TEXT("Nothing is entered until you choose to record the account."),18,true);
            }
            else
            {
                Line(TEXT("REED\nDid you see the man, or anyone with him?"),23);
                Line(TEXT("RESIDENT\nNo. I heard about him here in town. That path isn't just for the lease men. Families use it too. I can't tell you who was there that morning."),24);
                Line(TEXT("A reported use of the path, not an eyewitness account of the death."),20,true);
                Page->AddSlot().AutoHeight()[Button(TEXT("RECORD THE RESIDENT'S ACCOUNT"),[this]{Owner->Case()->Report.FieldNotes.AddUnique(TEXT("ResidentAccount"));Owner->CloseBook();})];
            }
        }
        else if(FieldAction==5)
        {
            Line(TEXT("HOUSE NOTICE\nRooms upstairs. Meals downstairs. Leave messages with Mrs. Lang."),26);
            Line(TEXT("Reed's room is here, across Court Street from county business. The courthouse stands beyond the square; the jail is east along the road."),24);
            Line(TEXT("The lobby is open for this town study. Rooms, meals and conversations are still to come. Write the date at the jail desk to save your discoveries."),20,true);
            Page->AddSlot().AutoHeight()[Button(TEXT("BACK TO THE LOBBY"),[this]{Owner->CloseBook();})];
        }
        else if(FieldAction==0 || FieldAction==4)
        {
            Line(TEXT("Walk through the open office doorway, turn south to the dirt road, then follow it east to Bend Lateral. The same road leads home."),24);
            Line(TEXT("Field notes stay in your book. Write the date at the office desk to save them."),22,true);
            Page->AddSlot().AutoHeight()[Button(TEXT("WALK THE ROAD"),[this]{Owner->CloseBook();})];
        }
        else
        {
            const bool bFollowup=Report.CanCompleteFollowup(FieldAction);
            const TCHAR* Copy[]={TEXT(""),TEXT("SALAZAR\nI found him at the lateral. I didn't see him go into the water. Finding a man isn't the same as knowing what happened to him."),TEXT("Reed sees a bottle beside the bank. Its presence does not establish who drank from it, or how the man died."),TEXT("The irrigation channel runs beside the road. From this bank, Reed cannot establish how the man entered the water.")};
            const FName Keys[]={NAME_None,TEXT("SalazarStatement"),TEXT("BottleObserved"),TEXT("BankExamined")};
            Line(bFollowup?FCLReportState::FollowupFinding(Report.FollowupLead):FString(Copy[FieldAction]),bInspect?20:26);
            const FName Key=Keys[FieldAction];
            Line(bFollowup?TEXT("Follow-up observation. Cause of death remains unestablished."):Report.FieldNotes.Contains(Key)?TEXT("Already entered in field notes."):TEXT("An observation, not a finding of cause."),bInspect?18:22,true);
            Page->AddSlot().AutoHeight()[Button(bFollowup?TEXT("RECORD FOLLOW-UP"):TEXT("ENTER FIELD NOTE"),[this,Key,bFollowup]{auto& R=Owner->Case()->Report;if(bFollowup) R.CompleteFollowup(FieldAction);else R.FieldNotes.AddUnique(Key);Owner->CloseBook();})];
            if(bInspect)
            {
                Line(TEXT("Q / E or LB / RB: orbit\nR / F or LT / RT: zoom\nRight stick: adjust view"),16,true);
                Page->AddSlot().AutoHeight()[SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().FillWidth(1)[Button(TEXT("< ORBIT"),[this]{Owner->AdjustInspection(-8,0);})]
                    +SHorizontalBox::Slot().FillWidth(1)[Button(TEXT("ORBIT >"),[this]{Owner->AdjustInspection(8,0);})]];
                Page->AddSlot().AutoHeight()[SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().FillWidth(1)[Button(TEXT("CLOSER"),[this]{Owner->AdjustInspection(0,-.1f);})]
                    +SHorizontalBox::Slot().FillWidth(1)[Button(TEXT("FARTHER"),[this]{Owner->AdjustInspection(0,.1f);})]];
            }
        }
        Page->AddSlot().FillHeight(1)[SNew(SSpacer)];
        Page->AddSlot().AutoHeight()[Button(TEXT("LEAVE / B"),[this]{Owner->CloseBook();})];
        FocusFirst();return;
    }
    if(bConversation)
    {
        Line(TEXT("JAIL OFFICE  /  DEPUTY PRUITT"),18,true);
        Line(TEXT("Before the ink dries"),42);
        if(Report.FollowupOutcome!=ECLFollowupOutcome::None)
        {
            Line(Report.FollowupOutcome==ECLFollowupOutcome::RequestInquiry?TEXT("PRUITT\nI'll carry the inquiry, Sheriff. We'll keep what you observed separate from what anyone supposes. We still need a cause of death."):TEXT("PRUITT\nThe supplement is with the clerk. No new inquiry order, then. Those questions will stay on the paper unless we take them further."),26);
            Line(State->IsCurrentStateSaved()?TEXT("The date is written. The original carbon and the follow-up are both kept."):TEXT("Write the date before you leave. The new entry is not saved yet."),22,true);
        }
        else if(Report.Status!=ECLReportStatus::Draft)
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
        Line(TEXT("WASD / left stick: walk     Mouse / right stick: look\nHold Shift / LB while moving: jog\nE / A: interact     Tab / View: County Book\nD-pad / left stick: choose     A: select     B: back\nLB / RB: book pages     Menu: pause\nSave at the desk before leaving."),20,true);
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
        Line(TEXT("FIELD NOTES"),18,true);
        if(Report.FieldNotes.IsEmpty()) Line(TEXT("No first-hand notes yet. Take the road from the office door."),20,true);
        if(Report.FieldNotes.Contains(TEXT("SalazarStatement"))) Line(TEXT("Salazar: found the man; did not witness him entering the water."),20);
        if(Report.FieldNotes.Contains(TEXT("BottleObserved"))) Line(TEXT("Bottle: observed beside the bank; ownership and use unestablished."),20);
        if(Report.FieldNotes.Contains(TEXT("BankExamined"))) Line(TEXT("Bank: irrigation channel inspected; means of entry unestablished."),20);
        if(Report.FieldNotes.Contains(TEXT("ResidentAccount"))) Line(TEXT("River Road resident, heard on North Lane: families use the bank path to reach fields. Did not see the man or witness the death; cannot say who was there that morning."),20);
        else Line(TEXT("TOWN LEAD / A River Road resident is at the west house on North Lane, beyond the market shops."),20,true);
        if(Report.Status!=ECLReportStatus::Draft) Line(TEXT("New field notes do not change the submitted carbon."),18,true);
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
            if(!Report.FieldNotes.IsEmpty())
                Page->AddSlot().AutoHeight()[Button(Report.bIncludeFieldNotes?TEXT("[x] Include available field notes"):TEXT("[ ] Omit available field notes"),[this]{auto& R=Owner->Case()->Report;R.bIncludeFieldNotes=!R.bIncludeFieldNotes;Rebuild();},bEditable)];
            Line(TEXT("CLOSING LINE  /  choose one"),18,true);
            for(int32 I=0;I<3;++I)
                Page->AddSlot().AutoHeight().Padding(0,0,0,7)[Button(FString(Report.ClosingLine==I?TEXT("[x]  "):TEXT("[ ]  "))+Report.ClosingText(I),[this,I]{Owner->Case()->Report.ClosingLine=I;Notice.Empty();Rebuild();},bEditable)];
            Page->AddSlot().AutoHeight().Padding(0,14)
            [SNew(SHorizontalBox)
                +SHorizontalBox::Slot().AutoWidth().Padding(0,0,12,0)[Button(TEXT("SIGN"),[this]{if(Owner->IsAtDesk() && Owner->Case()->Report.Submit(ECLReportStatus::Signed)) Notice=TEXT("Signed. A carbon stays with the case. Write the date to save.");Rebuild();},bEditable)]
                +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("HOLD"),[this]{if(Owner->IsAtDesk() && Owner->Case()->Report.Submit(ECLReportStatus::Held)) Notice=TEXT("Held for inquiry. The clerk will notice. Write the date to save.");Rebuild();},bEditable)]];
            if(Report.Status!=ECLReportStatus::Draft)
                Line(FString::Printf(TEXT("Carbon copy: %d fact(s) included, %d omitted.\nThe original carbon remains unchanged. Follow-up is entered separately below."),Report.IncludedFacts.Num(),Report.OmittedFacts.Num()),19,true);
            else if(!Owner->IsAtDesk()) Line(TEXT("Return to the desk to change or submit this report."),20,true);
        }
        Line(TEXT("FOLLOW-UP / BEND LATERAL"),24);
        for(ECLFollowupLead Lead:{ECLFollowupLead::Bottle,ECLFollowupLead::Bank,ECLFollowupLead::Finder})
        {
            const bool bKnown=Report.FollowupFacts.Contains(FCLReportState::FollowupFact(Lead));
            if(bKnown) Line(FCLReportState::FollowupFinding(Lead),20);
            else if(Report.FollowupOutcome==ECLFollowupOutcome::None)
            {
                const bool bAvailable=Report.CanPursue(Lead);
                const FString Prefix=Report.FollowupLead==Lead?TEXT("[PURSUING] "):bAvailable?TEXT("FOLLOW UP: "):!Report.bRead?TEXT("[READ REPORT FIRST] "):TEXT("[NEEDS FIELD NOTE] ");
                Page->AddSlot().AutoHeight()[Button(Prefix+FCLReportState::FollowupTitle(Lead),[this,Lead]{Owner->Case()->Report.Pursue(Lead);Notice=TEXT("Lead entered. Return to the matching evidence or witness at Bend Lateral.");Rebuild();},bAvailable && Report.FollowupLead!=Lead)];
            }
        }
        if(Report.FollowupLead!=ECLFollowupLead::None)
        {
            Line(Report.FollowupObjective(),20,true);
            Page->AddSlot().AutoHeight()[Button(TEXT("SET THIS LEAD ASIDE"),[this]{Owner->Case()->Report.FollowupLead=ECLFollowupLead::None;Notice=TEXT("Lead set aside. Completed observations remain in the Book.");Rebuild();})];
        }
        if(Report.FollowupOutcome!=ECLFollowupOutcome::None)
        {
            Line(Report.FollowupOutcome==ECLFollowupOutcome::RequestInquiry?TEXT("FILED / FURTHER INQUIRY REQUESTED"):TEXT("FILED / SUPPLEMENT ATTACHED"),22);
            Line(Report.FollowupConsequence(),20);
            Line(TEXT("This supplemental record is locked. Write the date to save it with the original carbon."),18,true);
        }
        else if(!Report.FollowupFacts.IsEmpty())
        {
            const bool bCanFile=Owner->IsAtDesk() && Report.Status!=ECLReportStatus::Draft && Report.FollowupLead==ECLFollowupLead::None;
            Line(TEXT("Attach the observations and leave the report's disposition standing, or request further inquiry and give Pruitt the unfinished questions. Either choice files all completed follow-up observations; the original carbon stays intact."),20);
            if(!bCanFile) Line(TEXT("Submit the original report and return to the desk. Finish or set aside the active lead before filing."),18,true);
            if(PendingFollowup==0)
            {
                Page->AddSlot().AutoHeight()[Button(TEXT("ATTACH A SUPPLEMENT"),[this]{PendingFollowup=1;Notice.Empty();Rebuild();},bCanFile)];
                Page->AddSlot().AutoHeight()[Button(TEXT("REQUEST FURTHER INQUIRY"),[this]{PendingFollowup=2;Notice.Empty();Rebuild();},bCanFile)];
            }
            else
            {
                Line(PendingFollowup==1?TEXT("The clerk will attach the observations. Pruitt receives no new inquiry order. This decision locks the follow-up record."):TEXT("The clerk will enter a request for further inquiry. Pruitt takes responsibility for the unanswered questions. This decision locks the follow-up record."),20,true);
                Page->AddSlot().AutoHeight()[Button(TEXT("CONFIRM FOLLOW-UP"),[this]{if(Owner->FileFollowup(static_cast<ECLFollowupOutcome>(PendingFollowup))) Notice=TEXT("Follow-up filed. Write the date to save; read the Ledger for the consequence.");PendingFollowup=0;Rebuild(0);},bCanFile)];
                Page->AddSlot().AutoHeight()[Button(TEXT("KEEP CONSIDERING"),[this]{PendingFollowup=0;Rebuild();})];
            }
        }
    }
    else if(ActivePage==0)
    {
        Line(TEXT("What the county remembers"),30);
        Line(TEXT("COURTHOUSE"),19,true);
        if(Report.FollowupOutcome!=ECLFollowupOutcome::None) Line(Report.FollowupConsequence(),22);
        Line(Report.Status==ECLReportStatus::Signed?TEXT("The clerk has my signature. The report can go upstairs."):Report.Status==ECLReportStatus::Held?TEXT("I held the report. They wanted it closed."):TEXT("An unsigned report waits on the desk."));
        Line(TEXT("STREET"),19,true); Line(Report.FieldNotes.Contains(TEXT("SalazarStatement"))?TEXT("I heard Salazar. He did not see the man enter the water."):TEXT("Salazar found him. I have not heard him out."));
        if(Report.FollowupOutcome!=ECLFollowupOutcome::None) Line(Report.FollowupOutcome==ECLFollowupOutcome::RequestInquiry?TEXT("I gave Pruitt the questions still unanswered. Salazar's account will have to be heard with care."):TEXT("I put the observations on record but gave no new inquiry order. A filed paper is not an answer for the people at the lateral."),22);
        Line(TEXT("CAPITAL"),19,true); Line(TEXT("No entry yet."));
        Line(TEXT("HOME"),19,true); Line(TEXT("A room at Lang's. The rest can wait."));
    }
    else if(ActivePage==1)
    {
        Line(TEXT("Rivas County"),30);
        Line(TEXT("County Clerk's Office  /  October 1926"),20,true);
        Line(TEXT("Leave the jail doorway and turn south to the road. West leads to Court Street: the courthouse is north of the square and Lang's is south. East leads to Bend Lateral."),24);
        Line(TEXT("PLACES ENTERED IN THE BOOK"),20,true);
        for(FName Id : {FName(TEXT("JailOffice")),FName(TEXT("CourtStreet")),FName(TEXT("LangHouse")),FName(TEXT("CountyRoad")),FName(TEXT("BendLateral"))})
            if(State->World.IsLocationDiscovered(Id)) Line(Id==TEXT("JailOffice")?TEXT("Jail office / Pecos Bend"):Id==TEXT("CourtStreet")?TEXT("Court Street / courthouse square"):Id==TEXT("LangHouse")?TEXT("Lang's / rooms and board"):Id==TEXT("CountyRoad")?TEXT("Road to Bend Lateral"):TEXT("Bend Lateral / irrigation bank"),22);
        Line(TEXT("North Lane runs behind the courthouse and shops. The resident is at the west house, north of the market row."),20);
        Line(TEXT("Discoveries are kept when you write the date at the desk. This first route is a compact study of the county."),18,true);
    }
    else if(ActivePage==3)
    {
        Line(TEXT("People in the book"),30);
        if(Report.FieldNotes.Contains(TEXT("ResidentAccount"))) Line(TEXT("RIVER ROAD RESIDENT / Met on North Lane\nSays families use the bank path to reach fields. Did not witness the death. Account entered with the Bend Lateral notes."));
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
    if(Owner->IsInspecting())
    {
        if(Key==EKeys::Q || Key==EKeys::Gamepad_LeftShoulder) {Owner->AdjustInspection(-8,0);return FReply::Handled();}
        if(Key==EKeys::E || Key==EKeys::Gamepad_RightShoulder) {Owner->AdjustInspection(8,0);return FReply::Handled();}
        if(Key==EKeys::R || Key==EKeys::Gamepad_LeftTrigger) {Owner->AdjustInspection(0,-.1f);return FReply::Handled();}
        if(Key==EKeys::F || Key==EKeys::Gamepad_RightTrigger) {Owner->AdjustInspection(0,.1f);return FReply::Handled();}
    }
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
    if(!bCover && !bPause && !bConversation && FieldAction<0 && (Key==EKeys::Gamepad_LeftShoulder || Key==EKeys::Gamepad_RightShoulder))
    {ActivePage=(ActivePage+(Key==EKeys::Gamepad_LeftShoulder?4:1))%5;Rebuild();return FReply::Handled();}
    return FReply::Unhandled();
}

FReply SCLCountyBook::OnAnalogValueChanged(const FGeometry&,const FAnalogInputEvent& Event)
{
    if(Owner->IsInspecting() && (Event.GetKey()==EKeys::Gamepad_RightX || Event.GetKey()==EKeys::Gamepad_RightY))
    {
        const float Value=Event.GetAnalogValue();
        if(FMath::Abs(Value)>.2f)
        {
            const float Step=FMath::Min(FSlateApplication::Get().GetDeltaTime(),.05f)*Value;
            Owner->AdjustInspection(Event.GetKey()==EKeys::Gamepad_RightX?Step*40:0,Event.GetKey()==EKeys::Gamepad_RightY?-Step*.4f:0);
        }
        return FReply::Handled();
    }
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
