#pragma once

#include "CoreMinimal.h"

class USaveGame;
class UCLPrototypeSave;
struct FCLReportState;

enum class ECLSaveRejection : uint8
{
    None,
    NullSave,           // Missing, or not a UCLPrototypeSave.
    UnsupportedVersion,
    InvalidClosingLine,
    InvalidStatus
};

// Acceptance rules for the jail prototype save slot. These mirror the rules that
// UCLCaseState::Initialize has applied since version 1; they deliberately do not
// judge narrative consistency (e.g. a signed report with no facts is still accepted).
namespace CLSaveValidation
{
    constexpr int32 SupportedVersion = 1;

    // Returns None when the save may be loaded, otherwise the first rule it breaks.
    COUNTYLINE_API ECLSaveRejection Validate(const USaveGame* Save);

    // Validates, then copies the whole report struct and copy preference.
    // On rejection neither destination is written.
    COUNTYLINE_API bool CopyIfValid(const USaveGame* Save, FCLReportState& OutReport, bool& OutTypedCopy, ECLSaveRejection* OutRejection = nullptr);

    COUNTYLINE_API const TCHAR* RejectionText(ECLSaveRejection Rejection);
}
