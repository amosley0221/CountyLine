#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CLPrototypeGameMode.generated.h"

UCLASS()
class COUNTYLINE_API ACLPrototypeGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ACLPrototypeGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    void RunSmokeTest();
    bool bSmoke = false;
    float SmokeTime = 0;
    FVector SmokeStart = FVector::ZeroVector;
};
