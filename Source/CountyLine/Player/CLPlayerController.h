#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CLPlayerController.generated.h"

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
    FString ObjectiveText() const;
    bool IsAtDesk() const;
    void Interact();
    void ToggleBook();
    void ShowBook(bool bReportCover = false, bool bPause = false, bool bConversation = false);
    void CloseBook();
    bool IsBookOpen() const { return Book.IsValid(); }
    class UCLCaseState* Case() const;
    static bool WithinInteractionGate(FVector PawnPosition, FVector Eye, FVector Forward, FVector Target);
private:
    void PauseMenu();
    TWeakObjectPtr<class ACLJailOffice> Office;
    TSharedPtr<class SWidget> HUD;
    TSharedPtr<class SCLCountyBook> Book;
    bool bPromptAvailable = false;
    bool bDeputyAvailable = false;
};
