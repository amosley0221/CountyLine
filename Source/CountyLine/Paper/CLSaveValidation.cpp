#include "Paper/CLSaveValidation.h"
#include "Paper/CLCaseState.h"

ECLSaveRejection CLSaveValidation::Validate(const USaveGame* Save)
{
    const UCLPrototypeSave* Prototype = Cast<UCLPrototypeSave>(Save);
    if (!Prototype) return ECLSaveRejection::NullSave;
    if (Prototype->Version != SupportedVersion) return ECLSaveRejection::UnsupportedVersion;
    if (Prototype->Report.ClosingLine < 0 || Prototype->Report.ClosingLine >= static_cast<int32>(UE_ARRAY_COUNT(UCLCaseState::ClosingLines))) return ECLSaveRejection::InvalidClosingLine;
    if (static_cast<uint8>(Prototype->Report.Status) > static_cast<uint8>(ECLReportStatus::Held)) return ECLSaveRejection::InvalidStatus;
    const FCLReportState& R=Prototype->Report;
    if(static_cast<uint8>(R.FollowupLead)>static_cast<uint8>(ECLFollowupLead::Finder) ||
       static_cast<uint8>(R.FollowupOutcome)>static_cast<uint8>(ECLFollowupOutcome::RequestInquiry)) return ECLSaveRejection::InvalidFollowup;
    TSet<FName> Seen;
    for(FName Fact:R.FollowupFacts)
    {
        if((Fact!=TEXT("BottleSealed") && Fact!=TEXT("PrintsAboveWater") && Fact!=TEXT("NoWetClothes")) || Seen.Contains(Fact)) return ECLSaveRejection::InvalidFollowup;
        Seen.Add(Fact);
    }
    if(R.FollowupLead!=ECLFollowupLead::None && !R.CanPursue(R.FollowupLead)) return ECLSaveRejection::InvalidFollowup;
    if(R.FollowupOutcome==ECLFollowupOutcome::None)
    {
        if(!R.SupplementFacts.IsEmpty()) return ECLSaveRejection::InvalidFollowup;
    }
    else if(!R.bRead || R.Status==ECLReportStatus::Draft || R.FollowupLead!=ECLFollowupLead::None || R.SupplementFacts.IsEmpty() || R.SupplementFacts!=R.FollowupFacts) return ECLSaveRejection::InvalidFollowup;
    // Empty world state, the default for saves written before it existed, is valid.
    if (!Prototype->World.IsValidState()) return ECLSaveRejection::InvalidWorldState;
    return ECLSaveRejection::None;
}

bool CLSaveValidation::CopyIfValid(const USaveGame* Save, FCLReportState& OutReport, bool& OutTypedCopy, ECLSaveRejection* OutRejection)
{
    const ECLSaveRejection Rejection = Validate(Save);
    if (OutRejection) *OutRejection = Rejection;
    if (Rejection != ECLSaveRejection::None) return false;
    const UCLPrototypeSave* Prototype = CastChecked<UCLPrototypeSave>(Save);
    // Whole-struct assignment, so fields added to FCLReportState later are carried too.
    OutReport = Prototype->Report;
    OutTypedCopy = Prototype->bTypedCopy;
    return true;
}

bool CLSaveValidation::CopyIfValid(const USaveGame* Save, FCLReportState& OutReport, bool& OutTypedCopy, FCLWorldState& OutWorld, ECLSaveRejection* OutRejection)
{
    const ECLSaveRejection Rejection = Validate(Save);
    if (OutRejection) *OutRejection = Rejection;
    if (Rejection != ECLSaveRejection::None) return false;
    const UCLPrototypeSave* Prototype = CastChecked<UCLPrototypeSave>(Save);
    OutReport = Prototype->Report;
    OutTypedCopy = Prototype->bTypedCopy;
    // Whole-struct assignment again, so later world-state fields come along.
    OutWorld = Prototype->World;
    return true;
}

const TCHAR* CLSaveValidation::RejectionText(ECLSaveRejection Rejection)
{
    switch (Rejection)
    {
    case ECLSaveRejection::None: return TEXT("accepted");
    case ECLSaveRejection::NullSave: return TEXT("no prototype save");
    case ECLSaveRejection::UnsupportedVersion: return TEXT("unsupported version");
    case ECLSaveRejection::InvalidClosingLine: return TEXT("invalid closing line");
    case ECLSaveRejection::InvalidStatus: return TEXT("invalid report status");
    case ECLSaveRejection::InvalidFollowup: return TEXT("invalid follow-up record");
    case ECLSaveRejection::InvalidWorldState: return TEXT("invalid world state");
    default: return TEXT("unknown");
    }
}
