#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "CLCaseState.generated.h"

UENUM()
enum class ECLReportStatus : uint8 { Draft, Signed, Held };

// Small, serializable case state for the jail prototype; all UI uses this authority.
USTRUCT()
struct FCLReportState
{
    GENERATED_BODY()
    UPROPERTY() ECLReportStatus Status = ECLReportStatus::Draft;
    UPROPERTY() bool bRead = false;
    UPROPERTY() bool bBriefedByPruitt = false;
    UPROPERTY() bool bIncludeFinder = true;
    UPROPERTY() bool bIncludeBottle = true;
    UPROPERTY() int32 ClosingLine = 0;
    UPROPERTY() TArray<FName> IncludedFacts;
    UPROPERTY() TArray<FName> OmittedFacts;
    UPROPERTY() int32 CourthouseDelta = 0;
    UPROPERTY() TArray<FName> FieldNotes;
    UPROPERTY() bool bIncludeFieldNotes = true;
    UPROPERTY() FString CarbonClosingLine;
    FString ClosingText(int32 Index) const;
    bool Submit(ECLReportStatus NewStatus);
};

UCLASS()
class COUNTYLINE_API UCLPrototypeSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() int32 Version = 1;
    UPROPERTY() FCLReportState Report;
    UPROPERTY() bool bTypedCopy = true;
};

UCLASS()
class COUNTYLINE_API UCLCaseState : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    UPROPERTY() FCLReportState Report;
    bool bTypedCopy = true;
    bool bHasWrittenDate = false;
    bool WriteDate();
    bool IsCurrentStateSaved() const;
    static FString StatusText(ECLReportStatus Status);
    static const TCHAR* ClosingLines[3];
private:
    FCLReportState SavedReport;
    bool bSavedTypedCopy = true;
};
