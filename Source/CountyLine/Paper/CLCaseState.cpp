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

void UCLCaseState::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Test runs must never load or overwrite a player's slot.
    if (FParse::Param(FCommandLine::Get(), TEXT("CLSmokeTest"))) return;
    if (!UGameplayStatics::DoesSaveGameExist(TEXT("CountyLine_JailPrototype"), 0)) return;
    const USaveGame* Save = UGameplayStatics::LoadGameFromSlot(TEXT("CountyLine_JailPrototype"), 0);
    if (CLSaveValidation::CopyIfValid(Save, Report, bTypedCopy))
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
