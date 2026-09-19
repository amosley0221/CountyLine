#include "Paper/CLCaseState.h"
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
    Status = NewStatus;
    CourthouseDelta = Status == ECLReportStatus::Signed ? 2 : -3;
    return true;
}

void UCLCaseState::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Test runs must never load or overwrite a player's slot.
    if (FParse::Param(FCommandLine::Get(), TEXT("CLSmokeTest"))) return;
    if (!UGameplayStatics::DoesSaveGameExist(TEXT("CountyLine_JailPrototype"), 0)) return;
    const UCLPrototypeSave* Save = Cast<UCLPrototypeSave>(UGameplayStatics::LoadGameFromSlot(TEXT("CountyLine_JailPrototype"), 0));
    if (Save && Save->Version == 1 && Save->Report.ClosingLine >= 0 && Save->Report.ClosingLine < 3 &&
        static_cast<uint8>(Save->Report.Status) <= static_cast<uint8>(ECLReportStatus::Held))
    {
        Report = Save->Report;
        bTypedCopy = Save->bTypedCopy;
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
