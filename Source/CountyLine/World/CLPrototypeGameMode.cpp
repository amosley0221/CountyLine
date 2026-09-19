#include "World/CLPrototypeGameMode.h"
#include "World/CLJailOffice.h"
#include "Player/CLReedCharacter.h"
#include "Player/CLPlayerController.h"
#include "Paper/CLCaseState.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

ACLPrototypeGameMode::ACLPrototypeGameMode()
{
    DefaultPawnClass=ACLReedCharacter::StaticClass();
    PlayerControllerClass=ACLPlayerController::StaticClass();
    PrimaryActorTick.bCanEverTick=true;
}

void ACLPrototypeGameMode::BeginPlay()
{
    Super::BeginPlay();
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("CLSmokeTest")))
    {
        bSmoke=true;
        FTimerHandle Handle;
        GetWorldTimerManager().SetTimer(Handle,this,&ACLPrototypeGameMode::RunSmokeTest,3.f,false);
    }
#endif
}

void ACLPrototypeGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(!bSmoke) return;
    if(APawn* Pawn=UGameplayStatics::GetPlayerPawn(this,0))
    {
        if(SmokeTime==0) SmokeStart=Pawn->GetActorLocation();
        SmokeTime+=DeltaSeconds;
        if(SmokeTime>0.5f && SmokeTime<1.5f) Pawn->AddMovementInput(FVector::ForwardVector,1.f);
    }
}

void ACLPrototypeGameMode::RunSmokeTest()
{
    bool Passed=true;
    auto Check=[&Passed](bool Condition,const TCHAR* Name)
    {
        UE_LOG(LogTemp,Display,TEXT("CL_SMOKE %s: %s"),Condition?TEXT("PASS"):TEXT("FAIL"),Name);
        Passed &= Condition;
    };
    ACLPlayerController* PC=Cast<ACLPlayerController>(UGameplayStatics::GetPlayerController(this,0));
    ACLReedCharacter* Reed=PC?Cast<ACLReedCharacter>(PC->GetPawn()):nullptr;
    Check(PC && Reed,TEXT("Game mode creates Reed and County Line controller"));
    if(Reed)
    {
        Check(Reed->GetActorLocation().X-SmokeStart.X>80,TEXT("Movement input advances character across floor"));
        Check(Reed->GetMesh()->GetSkeletalMeshAsset()!=nullptr,TEXT("Placeholder skeletal mesh loaded"));
        Check(Reed->GetMesh()->GetSingleNodeInstance()!=nullptr,TEXT("Locomotion animation instance loaded"));
        Check(Reed->GetCharacterMovement()->IsMovingOnGround(),TEXT("Character grounded on office floor"));
        const FVector Initial=SmokeStart;
        FHitResult Hit;
        Reed->SetActorLocation(FVector(Initial.X,-650,Initial.Z),true,&Hit);
        Check(Hit.bBlockingHit && Reed->GetActorLocation().Y > -460,TEXT("Capsule cannot cross office wall"));
        Reed->SetActorLocation(FVector(-20,-110,92));
        Check(PC->IsAtDesk(),TEXT("Desk range accepts near pawn"));
        PC->Case()->Report.bRead=true;
        PC->ShowBook();
        Check(PC->IsBookOpen() && PC->IsMoveInputIgnored() && UGameplayStatics::IsGamePaused(this),TEXT("Book owns input and pauses world"));
        PC->CloseBook();
        Check(!PC->IsBookOpen() && !PC->IsMoveInputIgnored() && !UGameplayStatics::IsGamePaused(this),TEXT("Closing book restores gameplay"));
        Reed->SetActorLocation(Initial);
        Check(!PC->IsAtDesk(),TEXT("Save station rejects distant pawn"));
    }
    UE_LOG(LogTemp,Display,TEXT("CL_SMOKE_RESULT=%s"),Passed?TEXT("PASS"):TEXT("FAIL"));
    FPlatformMisc::RequestExitWithStatus(false,Passed?0:1);
}
