#include "Paper/CLCaseState.h"
#include "Paper/CLSaveValidation.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

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
    // Test runs must never load or overwrite a player's slot.
    if (FParse::Param(FCommandLine::Get(), TEXT("CLSmokeTest"))) return;
    if (!UGameplayStatics::DoesSaveGameExist(TEXT("CountyLine_JailPrototype"), 0)) return;
    const USaveGame* Save = UGameplayStatics::LoadGameFromSlot(TEXT("CountyLine_JailPrototype"), 0);
    if (CLSaveValidation::CopyIfValid(Save, Report, bTypedCopy, World))
    {
        bHasWrittenDate = true;
        SavedReport = Report;
        bSavedTypedCopy = bTypedCopy;
    }
}

bool UCLCaseState::WriteDate()
{
    if (FParse::Param(FCommandLine::Get(), TEXT("CLSmokeTest"))) return false;
    UCLPrototypeSave* Save = Cast<UCLPrototypeSave>(UGameplayStatics::CreateSaveGameObject(UCLPrototypeSave::StaticClass()));
    Save->Report = Report;
    Save->bTypedCopy = bTypedCopy;
    Save->World = World;
    bHasWrittenDate = UGameplayStatics::SaveGameToSlot(Save, TEXT("CountyLine_JailPrototype"), 0);
    if (bHasWrittenDate)
    {
        SavedReport = Report;
        bSavedTypedCopy = bTypedCopy;
    }
    return bHasWrittenDate;
}

bool UCLCaseState::IsCurrentStateSaved() const
{
    return bHasWrittenDate && bTypedCopy == bSavedTypedCopy &&
        FCLReportState::StaticStruct()->CompareScriptStruct(&Report, &SavedReport, 0);
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
