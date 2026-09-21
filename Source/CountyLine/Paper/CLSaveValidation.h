#pragma once

#include "CoreMinimal.h"

class USaveGame;
class UCLPrototypeSave;
struct FCLReportState;
struct FCLWorldState;

enum class ECLSaveRejection : uint8
{
    None,
    NullSave,           // Missing, or not a UCLPrototypeSave.
    UnsupportedVersion,
    InvalidClosingLine,
    InvalidStatus,
    InvalidFollowup,
    InvalidWorldState,
    InvalidEnterprise
};

// Version 1 base-report acceptance remains unchanged (a signed report with no
// facts is still accepted). New tagged follow-up fields default to empty in old
// saves, and are checked for valid enum values and a coherent frozen supplement.
namespace CLSaveValidation
{
    constexpr int32 SupportedVersion = 1;

    // Returns None when the save may be loaded, otherwise the first rule it breaks.
    COUNTYLINE_API ECLSaveRejection Validate(const USaveGame* Save);

    // Validates, then copies the whole report struct and copy preference.
    // On rejection neither destination is written.
    COUNTYLINE_API bool CopyIfValid(const USaveGame* Save, FCLReportState& OutReport, bool& OutTypedCopy, ECLSaveRejection* OutRejection = nullptr);

    // Same rules, and also copies the world state. Older saves carry no world
    // state, so the destination receives the empty default. On rejection no
    // destination is written.
    COUNTYLINE_API bool CopyIfValid(const USaveGame* Save, FCLReportState& OutReport, bool& OutTypedCopy, FCLWorldState& OutWorld, ECLSaveRejection* OutRejection = nullptr);

    COUNTYLINE_API const TCHAR* RejectionText(ECLSaveRejection Rejection);
}
