#include "Paper/CLCaseState.h"
#include "Paper/CLSaveValidation.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// Modes that run against an isolated in-memory slot: automated tests, the freight
// fixture and the opt-in performance benchmark. Each one both skips loading the
// player's save and refuses to write one.
static bool IsSlotIsolated()
{
    return FParse::Param(FCommandLine::Get(), TEXT("CLSmokeTest")) ||
        FParse::Param(FCommandLine::Get(), TEXT("CLTestMission")) ||
        FParse::Param(FCommandLine::Get(), TEXT("CLPerfBenchmark"));
}

const TCHAR* UCLCaseState::ClosingLines[3] = {
    TEXT("The facts presently known are entered above."),
    TEXT("Further inquiry at Bend Lateral is required."),
    TEXT("The county should hear the finder before closing this matter.")
};

bool FCLReportState::Submit(ECLReportStatus NewStatus)
{
    if (!bRead || Status != ECLReportStatus::Draft ||
        (NewStatus != ECLReportStatus::Signed && NewStatus != ECLReportStatus::Held) ||
        ClosingLine < 0 || ClosingLine >= 3) return false;
    IncludedFacts.Reset();
    OmittedFacts.Reset();
    (bIncludeFinder ? IncludedFacts : OmittedFacts).Add(TEXT("SalazarFoundBody"));
    (bIncludeBottle ? IncludedFacts : OmittedFacts).Add(TEXT("BottleReported"));
    for(FName Note:FieldNotes) (bIncludeFieldNotes?IncludedFacts:OmittedFacts).AddUnique(Note);
    for(FName Fact:FollowupFacts) (bIncludeFieldNotes?IncludedFacts:OmittedFacts).AddUnique(Fact);
    CarbonClosingLine=ClosingText(ClosingLine);
    Status = NewStatus;
    CourthouseDelta = Status == ECLReportStatus::Signed ? 2 : -3;
    return true;
}

FString FCLReportState::ClosingText(int32 Index) const
{
    if(Index<0 || Index>=3) return FString();
    if(Status!=ECLReportStatus::Draft) return Index==ClosingLine && !CarbonClosingLine.IsEmpty()?CarbonClosingLine:FString(UCLCaseState::ClosingLines[Index]);
    if(Index==2 && FieldNotes.Contains(TEXT("SalazarStatement"))) return TEXT("The finder was heard; the cause of death remains unestablished.");
    return UCLCaseState::ClosingLines[Index];
}

FName FCLReportState::FollowupFact(ECLFollowupLead Lead)
{
    switch(Lead)
    {
    case ECLFollowupLead::Bottle:return TEXT("BottleSealed");
    case ECLFollowupLead::Bank:return TEXT("PrintsAboveWater");
    case ECLFollowupLead::Finder:return TEXT("NoWetClothes");
    default:return NAME_None;
    }
}

FString FCLReportState::FollowupTitle(ECLFollowupLead Lead)
{
    switch(Lead)
    {
    case ECLFollowupLead::Bottle:return TEXT("Check the bottle's seal");
    case ECLFollowupLead::Bank:return TEXT("Study the prints above the waterline");
    case ECLFollowupLead::Finder:return TEXT("Ask Salazar about the clothing");
    default:return TEXT("Choose a follow-up in the County Book");
    }
}

FString FCLReportState::FollowupFinding(ECLFollowupLead Lead)
{
    switch(Lead)
    {
    case ECLFollowupLead::Bottle:return TEXT("The bottle is full and its seal is intact. This bottle does not establish that the man had been drinking.");
    case ECLFollowupLead::Bank:return TEXT("Boot prints lie above the waterline. Reed cannot identify whose they are, or establish a route into the water.");
    case ECLFollowupLead::Finder:return TEXT("SALAZAR\nHis clothes were dry when I found him. I did not see how he came to be there.\n\nReed records the finder's account, not a medical finding.");
    default:return FString();
    }
}

bool FCLReportState::CanPursue(ECLFollowupLead Lead) const
{
    if(!bRead || FollowupOutcome!=ECLFollowupOutcome::None || FollowupFact(Lead).IsNone() || FollowupFacts.Contains(FollowupFact(Lead))) return false;
    const FName Required=Lead==ECLFollowupLead::Bottle?FName(TEXT("BottleObserved")):Lead==ECLFollowupLead::Bank?FName(TEXT("BankExamined")):FName(TEXT("SalazarStatement"));
    return FieldNotes.Contains(Required);
}

bool FCLReportState::Pursue(ECLFollowupLead Lead)
{
    if(!CanPursue(Lead) || FollowupLead==Lead) return false;
    FollowupLead=Lead;return true;
}

bool FCLReportState::CanCompleteFollowup(int32 FieldAction) const
{
    const int32 Target=FollowupLead==ECLFollowupLead::Bottle?2:FollowupLead==ECLFollowupLead::Bank?3:FollowupLead==ECLFollowupLead::Finder?1:-1;
    return CanPursue(FollowupLead) && Target==FieldAction;
}

bool FCLReportState::CompleteFollowup(int32 FieldAction)
{
    if(!CanCompleteFollowup(FieldAction)) return false;
    FollowupFacts.AddUnique(FollowupFact(FollowupLead));
    FollowupLead=ECLFollowupLead::None;return true;
}

bool FCLReportState::FileFollowup(ECLFollowupOutcome Outcome)
{
    if(Status==ECLReportStatus::Draft || !bRead || FollowupFacts.IsEmpty() || FollowupLead!=ECLFollowupLead::None || FollowupOutcome!=ECLFollowupOutcome::None ||
        (Outcome!=ECLFollowupOutcome::FileSupplement && Outcome!=ECLFollowupOutcome::RequestInquiry)) return false;
    SupplementFacts=FollowupFacts;FollowupOutcome=Outcome;return true;
}

bool FCLReportState::ShareWithEnterprise()
{
    if(bEnterpriseReviewed || !bRead || (Status!=ECLReportStatus::Signed && Status!=ECLReportStatus::Held)) return false;
    bEnterpriseReviewed=true;return true;
}

FString FCLReportState::EnterpriseHeadline() const
{
    if(!bEnterpriseReviewed) return TEXT("BEND LATERAL / COPY AWAITED");
    return Status==ECLReportStatus::Held?TEXT("BEND LATERAL / STORY HELD"):TEXT("BEND LATERAL / REED SIGNS REPORT");
}

FString FCLReportState::EnterpriseCopy() const
{
    if(!bEnterpriseReviewed) return TEXT("No carbon has been shown to The Enterprise. Speak with Mara Holt inside.");
    if(Status==ECLReportStatus::Held) return TEXT("MARA HOLT\nHeld for inquiry. Then I hold the story. I won't turn your unanswered questions into a settled account. Bring me a signed finding when there is one.\n\nNo account of the death has been posted. Your original held report remains unchanged.");
    FString Copy=TEXT("THE ENTERPRISE / FROM THE SIGNED CARBON\nActing Sheriff S. Reed has signed a report concerning the man found at Bend Lateral. The report does not establish a cause of death.");
    if(IncludedFacts.Contains(TEXT("SalazarFoundBody"))) Copy+=TEXT("\nThe report names Salazar as the finder.");
    if(IncludedFacts.Contains(TEXT("BottleReported"))) Copy+=TEXT("\nA bottle was reported at the scene; the carbon does not establish its owner or use.");
    if(IncludedFacts.Contains(TEXT("SalazarStatement"))) Copy+=TEXT("\nSalazar said he did not see the man enter the water.");
    if(IncludedFacts.Contains(TEXT("BottleObserved"))) Copy+=TEXT("\nReed observed a bottle beside the bank.");
    if(IncludedFacts.Contains(TEXT("BankExamined"))) Copy+=TEXT("\nReed inspected the bank; the means of entry remained unestablished.");
    if(IncludedFacts.Contains(TEXT("ResidentAccount"))) Copy+=TEXT("\nA resident said families use the bank path. The resident did not witness the death.");
    if(IncludedFacts.Contains(TEXT("BottleSealed"))) Copy+=TEXT("\nReed recorded a full bottle with its seal intact.");
    if(IncludedFacts.Contains(TEXT("PrintsAboveWater"))) Copy+=TEXT("\nBoot prints were observed above the waterline; their owner was not identified.");
    if(IncludedFacts.Contains(TEXT("NoWetClothes"))) Copy+=TEXT("\nSalazar described dry clothing. This is his account, not a medical finding.");
    return Copy;
}

FString FCLReportState::ClerkResponse() const
{
    if(Status==ECLReportStatus::Draft)
        return TEXT("INEZ PADILLA\nThis report is unsigned, Sheriff. Read it at the jail desk, choose what belongs in it, then sign it or hold it for inquiry. I need your disposition before I can review the carbon.");
    FString Reply=Status==ECLReportStatus::Signed?
        TEXT("INEZ PADILLA\nSigned by S. Reed. That is the disposition on this carbon. A signature records what you put forward; it does not settle the cause of death."):
        TEXT("INEZ PADILLA\nHeld for inquiry. The carbon records that you left this matter open. The unanswered questions belong with it, not under a different closing line.");
    Reply+=FString::Printf(TEXT("\n\nThis copy has %d included fact(s) and %d omitted. The original wording stays as submitted."),IncludedFacts.Num(),OmittedFacts.Num());
    if(FieldNotes.Contains(TEXT("ResidentAccount")))
        Reply+=IncludedFacts.Contains(TEXT("ResidentAccount"))?TEXT(" The resident's account is included."):OmittedFacts.Contains(TEXT("ResidentAccount"))?TEXT(" The resident's account was omitted; your Book still keeps it."):TEXT(" The resident's later account is in your Book, not on this carbon.");
    if(FollowupOutcome!=ECLFollowupOutcome::None) Reply+=TEXT(" Your separate follow-up entry leaves this original carbon intact.");
    return Reply;
}

FString FCLReportState::PruittResidentResponse() const
{
    const FName Account(TEXT("ResidentAccount"));
    if(!FieldNotes.Contains(Account)) return FString();
    FString Reply=TEXT("PRUITT\nFamilies use the bank path, then. That gives us people to ask, Sheriff. It doesn't place anyone there that morning or tell us how the man died.");
    if(Status==ECLReportStatus::Draft)
        Reply+=TEXT("\n\nThe report is still unsigned. Read it at the desk and decide whether to include that account with your field notes.");
    else
    {
        Reply+=Status==ECLReportStatus::Signed?TEXT("\n\nYou've signed the report. Your original carbon stands."):TEXT("\n\nYou've held the report for inquiry. That account gives us a question to pursue, not an answer. The held carbon stands.");
        if(IncludedFacts.Contains(Account)) Reply+=TEXT(" The resident's account is already included in it.");
        else if(OmittedFacts.Contains(Account)) Reply+=TEXT(" You left that account out of the report. It remains in your Book.");
        else Reply+=TEXT(" This later account stays in your Book; talking to me does not add it to the old report.");
    }
    return Reply;
}

FString FCLReportState::FollowupObjective() const
{
    if(FollowupOutcome!=ECLFollowupOutcome::None) return TEXT("Follow-up filed. Write the date at the desk to save.");
    if(FollowupLead!=ECLFollowupLead::None) return FString(TEXT("Bend Lateral: "))+FollowupTitle(FollowupLead)+TEXT(".");
    if(!FollowupFacts.IsEmpty()) return Status==ECLReportStatus::Draft?TEXT("Return to the desk: submit the report, then decide the follow-up."):TEXT("Review the follow-up at the desk, or choose another lead in Cases.");
    return FString();
}

FString FCLReportState::FollowupConsequence() const
{
    if(FollowupOutcome==ECLFollowupOutcome::FileSupplement)
        return TEXT("The clerk attaches the observations to the existing report. Its original disposition stands. Pruitt receives no new inquiry order; the unanswered questions remain on paper.");
    if(FollowupOutcome==ECLFollowupOutcome::RequestInquiry)
        return TEXT("The clerk enters a request for further inquiry beside the original report. Pruitt is assigned to carry the questions forward. The courthouse must account for a case Reed would not leave settled.");
    return TEXT("No follow-up has been filed.");
}

void UCLCaseState::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Test, fixture and benchmark runs must never load or overwrite a player's slot.
    if (IsSlotIsolated()) return;
    if (!UGameplayStatics::DoesSaveGameExist(TEXT("CountyLine_JailPrototype"), 0)) return;
    const USaveGame* Save = UGameplayStatics::LoadGameFromSlot(TEXT("CountyLine_JailPrototype"), 0);
    if (CLSaveValidation::CopyIfValid(Save, Report, bTypedCopy, World))
    {
        bHasWrittenDate = true;
        SavedReport = Report;
        SavedWorld = World;
        bSavedTypedCopy = bTypedCopy;
    }
}

bool UCLCaseState::WriteDate()
{
    if (IsSlotIsolated()) return false;
    UCLPrototypeSave* Save = Cast<UCLPrototypeSave>(UGameplayStatics::CreateSaveGameObject(UCLPrototypeSave::StaticClass()));
    Save->Report = Report;
    Save->bTypedCopy = bTypedCopy;
    Save->World = World;
    bHasWrittenDate = UGameplayStatics::SaveGameToSlot(Save, TEXT("CountyLine_JailPrototype"), 0);
    if (bHasWrittenDate)
    {
        SavedReport = Report;
        SavedWorld = World;
        bSavedTypedCopy = bTypedCopy;
    }
    return bHasWrittenDate;
}

bool UCLCaseState::IsCurrentStateSaved() const
{
    return bHasWrittenDate && bTypedCopy == bSavedTypedCopy &&
        FCLReportState::StaticStruct()->CompareScriptStruct(&Report, &SavedReport, 0) &&
        FCLWorldState::StaticStruct()->CompareScriptStruct(&World, &SavedWorld, 0);
}

FString UCLCaseState::StatusText(ECLReportStatus Status)
{
    switch (Status)
    {
    case ECLReportStatus::Signed: return TEXT("SIGNED / S. REED");
    case ECLReportStatus::Held: return TEXT("HELD FOR INQUIRY");
    default: return TEXT("UNSIGNED");
    }
}
