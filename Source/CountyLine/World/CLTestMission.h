#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CLTestMission.generated.h"

// Disposable design exercise. Never serialized into the campaign.
enum class ECLTrialStage : uint8 { Search, Chase, Confrontation, Complete, Escaped };
enum class ECLTrialChoice : uint8 { None, Recover, Verify, Release };
struct COUNTYLINE_API FCLTrialState
{
    ECLTrialStage Stage=ECLTrialStage::Search;
    ECLTrialChoice Choice=ECLTrialChoice::None;
    bool bManifest=false;
    bool bGateOpen=false;
    float ChaseSeconds=0;
    float Distance=0;
    bool StartChase();
    bool Catch();
    bool Resolve(ECLTrialChoice InChoice);
    void Advance(float Seconds,float RouteLength);
};

UCLASS()
class COUNTYLINE_API ACLTestMission : public AActor
{
    GENERATED_BODY()
public:
    ACLTestMission();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    static bool IsEnabled();
    FCLTrialState State;
    FVector StartPosition() const { return GetActorLocation()+FVector(0,-800,92); }
    FVector RunnerPosition() const;
    int32 ReachableAction(class ACLPlayerController* PC) const;
    FString Prompt(class ACLPlayerController* PC) const;
    FString Objective() const;
    FString Summary() const;
    void Interact(class ACLPlayerController* PC);
    void Resolve(ECLTrialChoice Choice);
    void Restart();
    void RunChecks();
private:
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Crate;
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Manifest;
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Gate;
    UPROPERTY() TObjectPtr<class USceneComponent> Runner;
    UPROPERTY() TObjectPtr<class USkeletalMeshComponent> RunnerMesh;
    UPROPERTY() TObjectPtr<class UTextRenderComponent> RunnerLabel;
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Cargo;
    TArray<FVector> Route;
    float RouteLength=0;
    float MessageSeconds=0;
    FString Message;
    bool bPositioned=false;
    FVector AlongRoute(float Distance) const;
    void UpdateRunner();
    void ReviewFrame();
    int32 ReviewIndex=0;
    FTimerHandle ReviewTimer;
};
