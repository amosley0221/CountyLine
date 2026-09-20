#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "World/CLWorldState.h"
#include "CLCaseState.generated.h"

UENUM()
enum class ECLReportStatus : uint8 { Draft, Signed, Held };

UENUM()
enum class ECLFollowupLead : uint8 { None, Bottle, Bank, Finder };

UENUM()
enum class ECLFollowupOutcome : uint8 { None, FileSupplement, RequestInquiry };

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
    // Version 1 tagged saves default these fields when absent. The original
    // carbon and courthouse delta never change when this later paper is filed.
    UPROPERTY() ECLFollowupLead FollowupLead = ECLFollowupLead::None;
    UPROPERTY() TArray<FName> FollowupFacts;
    UPROPERTY() ECLFollowupOutcome FollowupOutcome = ECLFollowupOutcome::None;
    UPROPERTY() TArray<FName> SupplementFacts;
    bool CanPursue(ECLFollowupLead Lead) const;
    bool Pursue(ECLFollowupLead Lead);
    bool CanCompleteFollowup(int32 FieldAction) const;
    bool CompleteFollowup(int32 FieldAction);
    bool FileFollowup(ECLFollowupOutcome Outcome);
    static FName FollowupFact(ECLFollowupLead Lead);
    static FString FollowupTitle(ECLFollowupLead Lead);
    static FString FollowupFinding(ECLFollowupLead Lead);
    FString FollowupObjective() const;
    FString FollowupConsequence() const;
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
    // Absent in saves written before world state existed; Unreal leaves it at
    // its empty default, which is a valid world state.
    UPROPERTY() FCLWorldState World;
};

UCLASS()
class COUNTYLINE_API UCLCaseState : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    UPROPERTY() FCLReportState Report;
    // Persistent world state. Loaded and saved with the report; nothing reads it
    // for spawning or travel yet.
    UPROPERTY() FCLWorldState World;
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
