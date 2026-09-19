#include "World/CLPrototypeGameMode.h"
#include "World/CLJailOffice.h"
#include "World/CLBendLateral.h"
#include "Player/CLReedCharacter.h"
#include "Player/CLPlayerController.h"
#include "Paper/CLCaseState.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "Animation/AnimSingleNodeInstance.h"

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
        const USkeletalMesh* ReedArt=Reed->GetMesh()->GetSkeletalMeshAsset();
        Check(ReedArt && ReedArt->GetName()==TEXT("SK_Reed_Period"),TEXT("Reed period character mesh loaded"));
        Check(ReedArt && ReedArt->GetBounds().BoxExtent.Z>75.f && ReedArt->GetBounds().BoxExtent.Z<110.f,TEXT("Imported Reed mesh retains human scale"));
        bool bReedMaterials=Reed->GetMesh()->GetNumMaterials()>0;
        for(int32 I=0;I<Reed->GetMesh()->GetNumMaterials();++I) bReedMaterials &= Reed->GetMesh()->GetMaterial(I)!=nullptr;
        Check(bReedMaterials,TEXT("Reed has all authored garment and skin materials"));
        Check(Reed->GetMesh()->GetSingleNodeInstance()!=nullptr,TEXT("Locomotion animation instance loaded"));
        // Evaluate bones even in -nullrhi, where visibility-based ticking can
        // leave sockets at their reference pose and conceal a bad animation scale.
        Reed->GetMesh()->TickAnimation(0.f,false);
        Reed->GetMesh()->RefreshBoneTransforms();
        const float PosedHeight=Reed->GetMesh()->GetSocketLocation(TEXT("head")).Z-Reed->GetActorLocation().Z;
        Check(PosedHeight>55.f && PosedHeight<110.f,TEXT("Animated Reed pose remains at human scale"));
        Check(Reed->GetFootstepCount()>0 && Reed->Footsteps->Sound!=nullptr,TEXT("Walking schedules surface footsteps"));
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
        ACLJailOffice* Office=nullptr;
        for(TActorIterator<ACLJailOffice> It(GetWorld());It;++It) {Office=*It;break;}
        Check(Office && Office->DeputyMesh->GetSkeletalMeshAsset() && Office->DeputyMesh->GetSingleNodeInstance(),TEXT("Pruitt has a mesh and idle animation"));
        auto Press=[](FKey Key)
        {
            const FKeyEvent Event(Key,FModifierKeysState(),0,false,0,0);
            FSlateApplication::Get().ProcessKeyDownEvent(Event);
            FSlateApplication::Get().ProcessKeyUpEvent(Event);
        };
        PC->Case()->Report=FCLReportState{};
        PC->ShowBook(false,false,true);
        Press(EKeys::Gamepad_FaceButton_Right);
        Check(!PC->IsBookOpen() && !PC->Case()->Report.bBriefedByPruitt,TEXT("B cancels dialogue without advancing the introduction"));
        PC->ShowBook(false,false,true);
        Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(PC->Case()->Report.bBriefedByPruitt && !PC->Case()->Report.bRead,TEXT("A advances Pruitt dialogue without reading the report remotely"));
        Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(!PC->IsBookOpen(),TEXT("A finishes dialogue and restores gameplay"));
        Reed->SetActorLocation(FVector(-20,-110,92));
        PC->ShowBook(true);
        Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(PC->Case()->Report.bRead,TEXT("A enters report into County Book"));
        for(int32 I=0;I<6;++I) Press(EKeys::Gamepad_DPad_Down);
        Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(!PC->Case()->Report.bIncludeFinder,TEXT("D-pad and A toggle a known fact"));
        const auto BeforeStick=FSlateApplication::Get().GetKeyboardFocusedWidget();
        FSlateApplication::Get().ProcessAnalogInputEvent(FAnalogInputEvent(EKeys::Gamepad_LeftY,FModifierKeysState(),0,false,0,0,-1.f));
        Check(FSlateApplication::Get().GetKeyboardFocusedWidget()!=BeforeStick,TEXT("Left stick moves book focus"));
        Press(EKeys::Gamepad_DPad_Down);
        Press(EKeys::Gamepad_DPad_Down);
        Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(PC->Case()->Report.ClosingLine==1,TEXT("Controller selects a closing line and preserves focus"));
        for(int32 I=0;I<3;++I) Press(EKeys::Gamepad_DPad_Down);
        Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(PC->Case()->Report.Status==ECLReportStatus::Held && PC->Case()->Report.OmittedFacts.Num()==1,TEXT("Controller submits HOLD with chosen facts"));
        Press(EKeys::Gamepad_LeftShoulder);
        Press(EKeys::Gamepad_RightShoulder);
        Check(PC->IsBookOpen(),TEXT("Shoulder page changes keep the book open"));
        Press(EKeys::Gamepad_FaceButton_Right);
        Check(!PC->IsBookOpen() && !PC->IsMoveInputIgnored(),TEXT("Controller B returns to gameplay"));
        Check(!PC->Case()->IsCurrentStateSaved(),TEXT("Submitting a report never implies it was saved"));
        const TArray<FName> Carbon=PC->Case()->Report.IncludedFacts;
        PC->ShowBook(false,false,false,4);
        Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(PC->IsInField() && !PC->IsBookOpen() && !PC->IsMoveInputIgnored(),TEXT("Controller travel enters Bend Lateral and restores movement"));
        ACLBendLateral* Bend=nullptr;
        for(TActorIterator<ACLBendLateral> It(GetWorld());It;++It) {Bend=*It;break;}
        Check(Bend && Bend->Markers.Num()==4 && Bend->Salazar->GetSingleNodeInstance(),TEXT("Bend Lateral has interaction targets and animated witness"));
        if(Bend)
        {
            const USkeletalMesh* WitnessArt=Bend->Salazar->GetSkeletalMeshAsset();
            Check(WitnessArt && WitnessArt->GetName()==TEXT("SK_Salazar_Period"),TEXT("Salazar period character mesh loaded"));
            TArray<UStaticMeshComponent*> Scenery;Bend->GetComponents(Scenery);
            int32 LoadedArt=0;
            bool bWaterAligned=false;
            for(const UStaticMeshComponent* Mesh:Scenery)
            {
                if(Mesh->GetName().StartsWith(TEXT("SM_Bend")) && Mesh->GetStaticMesh()) ++LoadedArt;
                if(Mesh->GetName()==TEXT("SM_BendWater") && Mesh->GetStaticMesh()) bWaterAligned=Mesh->GetStaticMesh()->GetBounds().Origin.Y>400.f;
            }
            Check(LoadedArt==6,TEXT("All six authored landscape meshes are available"));
            Check(bWaterAligned,TEXT("Imported channel aligns with the gameplay bank"));
            const USoundWave* Wind=Cast<USoundWave>(Bend->WindAudio->Sound);
            const USoundWave* Water=Cast<USoundWave>(Bend->WaterAudio->Sound);
            const USoundWave* Birds=Cast<USoundWave>(Bend->BirdsAudio->Sound);
            Check(Bend->IsFieldAudioActive() && Wind && Wind->bLooping && Water && Water->bLooping && Birds && Birds->bLooping,TEXT("Field travel enables three authored ambience loops"));
            Check(Bend->WaterAudio->bOverrideAttenuation && Bend->WaterAudio->AttenuationOverrides.bSpatialize,TEXT("Water sound is spatialized along the channel"));
        }
        PC->ShowBook(false,false,false,1);
        Check(PC->IsInspecting() && !UGameplayStatics::IsGamePaused(this) && Bend && Bend->Salazar->GetSingleNodeInstance()->GetCurrentAsset()->GetName()==TEXT("A_SalazarSpeaking"),TEXT("Witness conversation keeps scene live and plays gesture motion"));
        Press(EKeys::Gamepad_FaceButton_Right);
        Check(PC->Case()->Report.FieldNotes.IsEmpty(),TEXT("Canceling witness interaction awards no evidence"));
        PC->ShowBook(false,false,false,2);
        const FVector BeforeOrbit=PC->GetViewTarget()->GetActorLocation();
        Press(EKeys::Gamepad_RightShoulder);
        Check(PC->IsInspecting() && PC->IsMoveInputIgnored() && Reed->IsHidden() && !BeforeOrbit.Equals(PC->GetViewTarget()->GetActorLocation()),TEXT("Evidence close-up accepts controller orbit with movement locked"));
        const FVector BeforeZoom=PC->GetViewTarget()->GetActorLocation();
        Press(EKeys::Gamepad_LeftTrigger);
        Check(!BeforeZoom.Equals(PC->GetViewTarget()->GetActorLocation()),TEXT("Controller trigger adjusts evidence zoom"));
        Press(EKeys::Gamepad_FaceButton_Right);
        Check(!PC->IsInspecting() && PC->GetViewTarget()==Reed && !Reed->IsHidden() && !PC->IsMoveInputIgnored() && PC->Case()->Report.FieldNotes.IsEmpty(),TEXT("Cancel restores third person without awarding evidence"));
        for(int32 I=1;I<=3;++I)
        {
            PC->ShowBook(false,false,false,I);Press(EKeys::Gamepad_FaceButton_Bottom);
            PC->ShowBook(false,false,false,I);Press(EKeys::Gamepad_FaceButton_Bottom);
        }
        Check(PC->Case()->Report.FieldNotes.Num()==3,TEXT("Controller records three unique field notes without duplicates"));
        Check(PC->Case()->Report.IncludedFacts==Carbon,TEXT("Investigation cannot rewrite an existing carbon"));
        Check(!PC->IsAtDesk(),TEXT("Field investigation cannot access the office save station"));
        PC->ShowBook(false,false,false,0);Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(!PC->IsInField() && PC->Case()->Report.FieldNotes.Num()==3 && !PC->IsBookOpen(),TEXT("Return to office preserves all field notes"));
        Check(Bend && !Bend->IsFieldAudioActive(),TEXT("Return to office disables outdoor ambience"));
        Reed->SetActorLocation(FVector(-20,-110,92));
        PC->ShowBook();
        for(int32 I=0;I<6;++I) Press(EKeys::Gamepad_DPad_Down);
        Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(PC->Case()->Report.FollowupLead==ECLFollowupLead::Bottle,TEXT("Controller selects an unlocked follow-up in Cases"));
        Press(EKeys::Gamepad_FaceButton_Right);
        PC->TravelToBend(true);
        PC->ShowBook(false,false,false,2);Press(EKeys::Gamepad_FaceButton_Right);
        Check(PC->Case()->Report.FollowupFacts.IsEmpty() && PC->Case()->Report.FollowupLead==ECLFollowupLead::Bottle,TEXT("Cancel leaves the lead pending without awarding a finding"));
        PC->ShowBook(false,false,false,2);Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(PC->Case()->Report.FollowupFacts.Contains(TEXT("BottleSealed")) && !PC->IsInspecting(),TEXT("Field inspection records the selected follow-up finding"));
        Check(!PC->FileFollowup(ECLFollowupOutcome::RequestInquiry),TEXT("Follow-up filing is rejected away from the desk"));
        PC->TravelToBend(false);Reed->SetActorLocation(FVector(-20,-110,92));
        PC->ShowBook();
        for(int32 I=0;I<9;++I) Press(EKeys::Gamepad_DPad_Down);
        Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(PC->Case()->Report.FollowupOutcome==ECLFollowupOutcome::None,TEXT("Consequence preview does not file the decision"));
        Press(EKeys::Gamepad_DPad_Up);Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(PC->Case()->Report.FollowupOutcome==ECLFollowupOutcome::RequestInquiry && PC->Case()->Report.SupplementFacts.Contains(TEXT("BottleSealed")),TEXT("Controller confirms a frozen follow-up record at the desk"));
        Check(PC->Case()->Report.IncludedFacts==Carbon && !PC->Case()->IsCurrentStateSaved(),TEXT("Follow-up preserves carbon and remains unsaved until the date is written"));
        Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(!PC->IsBookOpen(),TEXT("Filing returns focus to Close rather than the save button"));
    }
    UE_LOG(LogTemp,Display,TEXT("CL_SMOKE_RESULT=%s"),Passed?TEXT("PASS"):TEXT("FAIL"));
    // Optional visual review fixture. CL smoke runs already bypass the player's
    // save slot and block WriteDate; keeping this one open cannot overwrite it.
    if(Passed && FParse::Param(FCommandLine::Get(),TEXT("CLSmokeKeepOpen")))
    {
        bSmoke=false;PC->ShowBook();return;
    }
    FPlatformMisc::RequestExitWithStatus(false,Passed?0:1);
}
