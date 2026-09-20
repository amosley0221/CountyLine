#include "World/CLPrototypeGameMode.h"
#include "World/CLJailOffice.h"
#include "World/CLBendLateral.h"
#include "World/CLCountyRoad.h"
#include "World/CLPecosBend.h"
#include "Paper/CLSaveValidation.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
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
#include "Animation/BlendSpace.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"

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
        TArray<FInputAxisKeyMapping> JogKeys;
        GetDefault<UInputSettings>()->GetAxisMappingByName(TEXT("Jog"),JogKeys);
        auto HasJogKey=[&JogKeys](FKey Key) {return JogKeys.ContainsByPredicate([Key](const FInputAxisKeyMapping& M){return M.Key==Key && M.Scale==1.f;});};
        Check(HasJogKey(EKeys::LeftShift) && HasJogKey(EKeys::Gamepad_LeftShoulder),TEXT("Jog maps keyboard Shift and controller LB"));
        auto JogInput=[Reed](float Value)
        {
            for(FInputAxisBinding& Binding:Reed->InputComponent->AxisBindings)
                if(Binding.AxisName==TEXT("Jog")) Binding.AxisDelegate.Execute(Value);
        };
        JogInput(1.f);
        Check(Reed->GetCharacterMovement()->MaxWalkSpeed==360.f,TEXT("Jog input doubles walking speed"));
        JogInput(2.f);
        Check(Reed->GetCharacterMovement()->MaxWalkSpeed==360.f,TEXT("Combined jog controls cannot stack speed"));
        JogInput(0.f);
        Check(Reed->GetCharacterMovement()->MaxWalkSpeed==180.f,TEXT("Releasing jog restores walking speed"));
        const UBlendSpace* JogBlend=LoadObject<UBlendSpace>(nullptr,TEXT("/Game/Art/Animations/BS_Reed_FieldLocomotion.BS_Reed_FieldLocomotion"));
        Check(JogBlend && JogBlend->GetBlendParameter(0).Max>=360.f,TEXT("Locomotion blend supports jogging speed"));
        const FVector Initial=SmokeStart;
        FHitResult Hit;
        Reed->SetActorLocation(FVector(Initial.X,-650,Initial.Z),true,&Hit);
        Check(Hit.bBlockingHit && Reed->GetActorLocation().Y > -460,TEXT("Capsule cannot cross office wall"));
        Reed->SetActorLocation(FVector(-20,-110,92));
        Check(PC->IsAtDesk(),TEXT("Desk range accepts near pawn"));
        PC->Case()->Report.bRead=true;
        JogInput(1.f);
        PC->ShowBook();
        Check(PC->IsBookOpen() && PC->IsMoveInputIgnored() && UGameplayStatics::IsGamePaused(this),TEXT("Book owns input and pauses world"));
        JogInput(1.f);
        Check(Reed->GetCharacterMovement()->MaxWalkSpeed==180.f && Reed->GetVelocity().IsNearlyZero(),TEXT("Book stops jogging and rejects jog while reading"));
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
        Check(!PC->IsInField() && !PC->IsBookOpen() && !PC->IsMoveInputIgnored(),TEXT("Road directions restore movement without teleporting"));
        // Sweep the same pawn continuously through the doorway and along the
        // authored road. Every step must have ground and remain unobstructed.
        Reed->SetActorLocation(FVector(-350,-180,92));
        PC->UpdateWorldProgress();
        bool bRouteClear=true;
        auto WalkRoute=[&](const FVector& Destination)
        {
            const FVector Start=Reed->GetActorLocation();
            const int32 Steps=FMath::CeilToInt(FVector::Distance(Start,Destination)/25.f);
            for(int32 I=1;I<=Steps;++I)
            {
                const FVector Next=FMath::Lerp(Start,Destination,float(I)/Steps);
                FHitResult Wall,Ground;
                Reed->SetActorLocation(Next,true,&Wall);
                FCollisionQueryParams Params(SCENE_QUERY_STAT(RoadSmoke),false,Reed);
                const bool bFloor=GetWorld()->LineTraceSingleByChannel(Ground,Next,Next-FVector(0,0,130),ECC_Visibility,Params);
                if(bRouteClear && (Wall.bBlockingHit || !bFloor || Ground.ImpactNormal.Z<=.7f))
                    UE_LOG(LogTemp,Warning,TEXT("CL_ROUTE obstruction=%s floor=%d at=%s actual=%s"),*GetNameSafe(Wall.GetComponent()),bFloor,*Next.ToString(),*Reed->GetActorLocation().ToString());
                bRouteClear &= !Wall.bBlockingHit && bFloor && Ground.ImpactNormal.Z>.7f;
                PC->UpdateWorldProgress();
            }
        };
        WalkRoute(FVector(-850,-180,92));WalkRoute(FVector(-850,-800,92));WalkRoute(FVector(8800,-800,92));
        Check(bRouteClear && PC->IsInField(),TEXT("Continuous capsule route connects the open office door to Bend Lateral"));
        Check(PC->Case()->World.IsLocationDiscovered(TEXT("JailOffice")) && PC->Case()->World.IsLocationDiscovered(TEXT("CountyRoad")) && PC->Case()->World.IsLocationDiscovered(TEXT("BendLateral")),TEXT("Walking discovers all three authored locations"));
        Check(PC->Case()->World.LastSafeLocation==TEXT("BendLateral"),TEXT("Entering Bend records its safe checkpoint"));
        const auto WorldBeforeRestore=PC->Case()->World;
        Reed->SetActorLocation(FVector(0,0,-250));
        Check(PC->RestoreSafePosition() && PC->IsInField(),TEXT("Recovery returns Reed to the last safe authored location"));
        Check(PC->Case()->World.DiscoveredLocations==WorldBeforeRestore.DiscoveredLocations && PC->Case()->Report.IncludedFacts==Carbon,TEXT("Recovery preserves discoveries and the original report"));
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
        Check(PC->IsInField(),TEXT("Return directions do not fast travel"));
        WalkRoute(FVector(8800,-800,92));WalkRoute(FVector(-850,-800,92));WalkRoute(FVector(-850,-180,92));WalkRoute(FVector(-350,-180,92));
        Check(bRouteClear,TEXT("The same road and doorway support the return walk"));
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
        PC->Case()->World.SetLastSafePosition(TEXT("UnknownLocation"),FTransform(FVector(0,0,-5000)));
        Check(PC->RestoreSafePosition() && PC->Case()->World.LastSafeLocation==TEXT("JailOffice") && PC->IsAtDesk()==false,TEXT("Unknown safe location falls back to the office entrance"));
        PC->Case()->World.SetLastSafePosition(TEXT("BendLateral"),FTransform(FVector(0,0,-5000)));
        Check(PC->RestoreSafePosition() && !PC->IsInField(),TEXT("Known location with unsafe coordinates falls back to the office"));
        // Teleport recovery starts above the floor. This synchronous route
        // fixture must enter walking mode before simulating grounded steps.
        Reed->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        WalkRoute(FVector(-850,-180,92));WalkRoute(FVector(-850,-800,92));WalkRoute(FVector(-2200,-800,92));
        WalkRoute(FVector(-3900,-250,92));
        Check(bRouteClear && PC->Case()->World.IsLocationDiscovered(TEXT("CourtStreet")),TEXT("Walk from jail reaches Court Street and the courthouse square"));
        WalkRoute(FVector(-3900,-800,92));WalkRoute(FVector(-4200,-800,92));WalkRoute(FVector(-4200,-1920,92));
        Check(bRouteClear && PC->Case()->World.IsLocationDiscovered(TEXT("LangHouse")),TEXT("Lang's open doorway admits Reed and records discovery"));
        Check(PC->Case()->World.LastSafeLocation==TEXT("LangHouse") && PC->RestoreSafePosition(),TEXT("Lang's has a reachable safe checkpoint"));
        Reed->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        WalkRoute(FVector(-3900,-1970,92));
        PC->SetControlRotation(FRotator(-15,-90,0));
        Reed->CameraArm->TickComponent(1.f,LEVELTICK_All,nullptr);
        PC->PlayerCameraManager->UpdateCamera(1.f);
        Check(PC->ReachableFieldAction()==5,TEXT("Lang register can be reached through the normal interaction gate"));
        PC->Interact();
        Check(PC->IsBookOpen() && !PC->IsInspecting(),TEXT("Guest register opens its own readable page"));
        Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(!PC->IsBookOpen() && !PC->IsMoveInputIgnored() && PC->Case()->Report.IncludedFacts==Carbon,TEXT("Controller exits Lang's notice without changing case evidence"));
        UCLPrototypeSave* TownSave=NewObject<UCLPrototypeSave>();TownSave->Report=PC->Case()->Report;TownSave->World=PC->Case()->World;
        TArray<uint8> TownBytes;FCLReportState TownReport;FCLWorldState TownWorld;bool bTownTyped=false;
        const bool bTownSerialized=UGameplayStatics::SaveGameToMemory(TownSave,TownBytes);
        USaveGame* TownLoaded=bTownSerialized?UGameplayStatics::LoadGameFromMemory(TownBytes):nullptr;
        Check(CLSaveValidation::CopyIfValid(TownLoaded,TownReport,bTownTyped,TownWorld) && TownWorld.IsLocationDiscovered(TEXT("LangHouse")) && TownWorld.IsLocationDiscovered(TEXT("CourtStreet")) && TownWorld.LastSafeLocation==TEXT("LangHouse") && TownReport.IncludedFacts==Carbon,TEXT("Town discoveries and checkpoint survive a memory-only save with the original carbon"));
        WalkRoute(FVector(-4200,-1920,92));WalkRoute(FVector(-4200,-800,92));WalkRoute(FVector(-850,-800,92));WalkRoute(FVector(-850,-180,92));WalkRoute(FVector(-350,-180,92));
        Check(bRouteClear && PC->Case()->World.LastSafeLocation==TEXT("JailOffice"),TEXT("Return walk from Lang's preserves an open route to the jail"));
        WalkRoute(FVector(-850,-180,92));WalkRoute(FVector(-850,-800,92));WalkRoute(FVector(-2200,-800,92));
        WalkRoute(FVector(-2200,4300,92));WalkRoute(FVector(-5500,4300,92));WalkRoute(FVector(-5500,4750,92));
        Check(bRouteClear && PC->Case()->World.LastSafeLocation==TEXT("CourtStreet"),TEXT("Residential lane connects to Court Street and retains its safe checkpoint"));
        WalkRoute(FVector(-5500,4300,92));WalkRoute(FVector(400,4300,92));WalkRoute(FVector(400,4800,92));
        WalkRoute(FVector(400,4300,92));WalkRoute(FVector(-2650,4300,92));WalkRoute(FVector(-2650,7000,92));
        WalkRoute(FVector(-5500,7000,92));WalkRoute(FVector(400,7000,92));WalkRoute(FVector(-2650,7000,92));
        Check(bRouteClear,TEXT("Residential fronts and rear yard access have grounded collision-clear routes"));
        WalkRoute(FVector(-2650,4300,92));WalkRoute(FVector(-2200,4300,92));WalkRoute(FVector(-2200,-800,92));
        WalkRoute(FVector(-850,-800,92));WalkRoute(FVector(-850,-180,92));WalkRoute(FVector(-350,-180,92));
        Check(bRouteClear && PC->Case()->World.LastSafeLocation==TEXT("JailOffice") && PC->Case()->Report.IncludedFacts==Carbon,TEXT("Residential return preserves the jail route and original carbon"));
    }
    UE_LOG(LogTemp,Display,TEXT("CL_SMOKE_RESULT=%s"),Passed?TEXT("PASS"):TEXT("FAIL"));
    if(Passed && FParse::Param(FCommandLine::Get(),TEXT("CLTownReview")))
    {
        bSmoke=false;PC->CloseBook();Reed->SetActorHiddenInGame(true);
        TownReviewCamera=GetWorld()->SpawnActor<ACameraActor>();
        PC->SetViewTarget(TownReviewCamera);
        CaptureTownReview();
        GetWorldTimerManager().SetTimer(TownReviewTimer,this,&ACLPrototypeGameMode::CaptureTownReview,2.f,true);
        return;
    }
    // Optional visual review fixture. CL smoke runs already bypass the player's
    // save slot and block WriteDate; keeping this one open cannot overwrite it.
    if(Passed && FParse::Param(FCommandLine::Get(),TEXT("CLSmokeKeepOpen")))
    {
        bSmoke=false;PC->ShowBook();return;
    }
    FPlatformMisc::RequestExitWithStatus(false,Passed?0:1);
}

void ACLPrototypeGameMode::CaptureTownReview()
{
    // Engine-rendered QA artifacts, isolated by the required CLSmokeTest run.
    // These cameras do not alter the player's saved location or normal view.
    const FVector Positions[]={FVector(-10000,-7600,8500),FVector(-1600,-480,260),FVector(-4340,-1820,190),FVector(-7600,2800,2500),FVector(-4400,-1400,450),FVector(-6250,4250,220)};
    const FVector Targets[]={FVector(-2900,1700,250),FVector(-560,0,260),FVector(-4000,-2330,110),FVector(-2700,5650,150),FVector(-3900,1200,650),FVector(-5500,5550,230)};
    const TCHAR* Names[]={TEXT("TownOverview.png"),TEXT("JailFrontage.png"),TEXT("LangLobby.png"),TEXT("ResidentialLane.png"),TEXT("CourthouseDetail.png"),TEXT("HomeDetail.png")};
    const int32 View=TownReviewStep/2;
    if(View>=6) {GetWorldTimerManager().ClearTimer(TownReviewTimer);FPlatformMisc::RequestExitWithStatus(false,0);return;}
    if(TownReviewStep%2==0)
    {
        TownReviewCamera->SetActorLocationAndRotation(Positions[View],(Targets[View]-Positions[View]).Rotation());
        TownReviewCamera->GetCameraComponent()->SetFieldOfView(View==2?85.f:70.f);
    }
    else FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Screenshots/TownReview"),Names[View]),false,false);
    ++TownReviewStep;
}
