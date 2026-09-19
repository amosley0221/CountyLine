#include "Paper/CLSaveValidation.h"
#include "Paper/CLCaseState.h"

ECLSaveRejection CLSaveValidation::Validate(const USaveGame* Save)
{
    const UCLPrototypeSave* Prototype = Cast<UCLPrototypeSave>(Save);
    if (!Prototype) return ECLSaveRejection::NullSave;
    if (Prototype->Version != SupportedVersion) return ECLSaveRejection::UnsupportedVersion;
    if (Prototype->Report.ClosingLine < 0 || Prototype->Report.ClosingLine >= static_cast<int32>(UE_ARRAY_COUNT(UCLCaseState::ClosingLines))) return ECLSaveRejection::InvalidClosingLine;
    if (static_cast<uint8>(Prototype->Report.Status) > static_cast<uint8>(ECLReportStatus::Held)) return ECLSaveRejection::InvalidStatus;
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

const TCHAR* CLSaveValidation::RejectionText(ECLSaveRejection Rejection)
{
    switch (Rejection)
    {
    case ECLSaveRejection::None: return TEXT("accepted");
    case ECLSaveRejection::NullSave: return TEXT("no prototype save");
    case ECLSaveRejection::UnsupportedVersion: return TEXT("unsupported version");
    case ECLSaveRejection::InvalidClosingLine: return TEXT("invalid closing line");
    case ECLSaveRejection::InvalidStatus: return TEXT("invalid report status");
    default: return TEXT("unknown");
    }
}
