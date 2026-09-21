#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CLPlayerController.generated.h"

enum class ECLFollowupOutcome : uint8;

UCLASS()
class COUNTYLINE_API ACLPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaSeconds) override;
    bool CanReachReport() const;
    bool CanReachDeputy() const;
    bool CanReachResident() const;
    bool CanReachClerk() const;
    FString ObjectiveText() const;
    bool IsAtDesk() const;
    void Interact();
    void ToggleBook();
    void ShowBook(bool bReportCover = false, bool bPause = false, bool bConversation = false, int32 FieldAction = -1);
    int32 ReachableFieldAction() const;
    void TravelToBend(bool bOutbound);
    bool IsInField() const;
    void UpdateWorldProgress();
    bool RestoreSafePosition();
    bool IsInspecting() const { return InspectionAction>=1 && InspectionAction<=3; }
    void AdjustInspection(float Orbit, float Zoom);
    bool FileFollowup(ECLFollowupOutcome Outcome);
    void CloseBook();
    bool IsBookOpen() const { return Book.IsValid(); }
    class UCLCaseState* Case() const;
    static bool WithinInteractionGate(FVector PawnPosition, FVector Eye, FVector Forward, FVector Target);
private:
    void BeginInspection(int32 Action);
    void EndInspection();
    void UpdateInspectionCamera();
    UPROPERTY() TObjectPtr<class ACameraActor> InspectionCamera;
    int32 InspectionAction = -1;
    float InspectionOrbit = 0;
    float InspectionZoom = 1;
    bool bPawnWasHidden = false;
    void PauseMenu();
    TWeakObjectPtr<class ACLJailOffice> Office;
    TWeakObjectPtr<class ACLBendLateral> Bend;
    TWeakObjectPtr<class ACLPecosBend> Town;
    TSharedPtr<class SWidget> HUD;
    TSharedPtr<class SCLCountyBook> Book;
    bool bPromptAvailable = false;
    bool bDeputyAvailable = false;
    bool bWorldInitialized = false;
};
