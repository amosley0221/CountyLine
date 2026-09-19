#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CLBendLateral.generated.h"

// A bounded outdoor study, reached through the office door.
UCLASS()
class COUNTYLINE_API ACLBendLateral : public AActor
{
    GENERATED_BODY()
public:
    ACLBendLateral();
    virtual void BeginPlay() override;
    FVector Target(int32 Index) const;
    void SetFieldActive(bool bActive);
    void SetWitnessSpeaking(bool bSpeaking);
    bool IsFieldAudioActive() const { return bFieldActive; }
    UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Markers;
    UPROPERTY() TObjectPtr<class USkeletalMeshComponent> Salazar;
    UPROPERTY() TObjectPtr<class UAudioComponent> WindAudio;
    UPROPERTY() TObjectPtr<class UAudioComponent> WaterAudio;
    UPROPERTY() TObjectPtr<class UAudioComponent> BirdsAudio;
private:
    bool bFieldActive = false;
};
