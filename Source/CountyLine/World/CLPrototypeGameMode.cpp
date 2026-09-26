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
#include "Components/TextRenderComponent.h"
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
#include "Engine/GameViewportClient.h"
#include "Widgets/SViewport.h"
#include "Layout/WidgetPath.h"

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
    // Run with -CLMobilePreview -faketouches and a rendered viewport. Check actual
    // hit paths: visible full-screen wrappers can silently intercept both sticks.
    if(FParse::Param(FCommandLine::Get(),TEXT("CLMobilePreview")))
    {
        const auto Viewport=GetWorld()->GetGameViewport()->GetGameViewportWidget();
        const FGeometry Geometry=Viewport->GetCachedGeometry();
        auto Hits=[&Geometry](FVector2D Fraction,const TCHAR* Type)
        {
            auto& Slate=FSlateApplication::Get();
            const auto Path=Slate.LocateWindowUnderMouse(Geometry.LocalToAbsolute(Geometry.GetLocalSize()*Fraction),Slate.GetInteractiveTopLevelWindows());
            for(int32 I=0;I<Path.Widgets.Num();++I)
                if(Path.Widgets[I].Widget->GetTypeAsString()==Type) return true;
            return false;
        };
        Check(Hits(FVector2D(.15f,.85f),TEXT("SVirtualJoystick")),TEXT("Mobile left-stick touch reaches virtual joystick"));
        Check(Hits(FVector2D(.85f,.85f),TEXT("SVirtualJoystick")),TEXT("Mobile right-stick touch reaches virtual joystick"));
        Check(Hits(FVector2D(.93f,.065f),TEXT("SButton")),TEXT("Mobile action button remains touchable"));
    }
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
        if(TActorIterator<ACLJailOffice> It(GetWorld());It) {Office=*It;}
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
        if(TActorIterator<ACLBendLateral> It(GetWorld());It) {Bend=*It;}
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
        WalkRoute(FVector(-850,-180,92));WalkRoute(FVector(-850,-800,92));WalkRoute(FVector(-5850,-800,92));
        WalkRoute(FVector(-5850,3300,92));WalkRoute(FVector(-5650,4300,92));WalkRoute(FVector(-7250,4300,92));
        WalkRoute(FVector(-7250,-800,92));WalkRoute(FVector(-2200,-800,92));
        Check(bRouteClear,TEXT("Western shop fronts and rear service lane connect to both cross streets"));
        WalkRoute(FVector(-2200,4300,92));WalkRoute(FVector(-1780,3500,92));WalkRoute(FVector(-1780,800,92));
        WalkRoute(FVector(-2200,800,92));WalkRoute(FVector(-2200,-800,92));
        WalkRoute(FVector(-850,-800,92));WalkRoute(FVector(-850,-180,92));WalkRoute(FVector(-350,-180,92));
        Check(bRouteClear && PC->Case()->World.LastSafeLocation==TEXT("JailOffice"),TEXT("Eastern storefront walk connects North Lane and the jail without crossing a shop"));
        ACLPecosBend* Town=nullptr;
        if(TActorIterator<ACLPecosBend> It(GetWorld());It) {Town=*It;}
        auto Part=[Town](const FString& Name)->UStaticMeshComponent*
        {
            return Town?FindObjectFast<UStaticMeshComponent>(Town,FName(*Name)):nullptr;
        };
        FCollisionQueryParams LayoutParams(SCENE_QUERY_STAT(TownLayout),false,Reed);
        bool bShopApproaches=Town!=nullptr;
        for(const TCHAR* Name:{TEXT("DryGoods"),TEXT("Grocer"),TEXT("ClosedShop"),TEXT("PostOffice"),TEXT("Drugs")})
        {
            const auto* Door=Part(FString(Name)+TEXT("Door"));
            if(!Door) {bShopApproaches=false;continue;}
            const FVector Facing=Door->GetComponentQuat().RotateVector(FVector(0,-1,0));
            const float ExpectedX=(FString(Name)==TEXT("PostOffice") || FString(Name)==TEXT("Drugs"))?-1.f:1.f;
            FHitResult Obstruction;
            const FVector Start=Door->GetComponentLocation()+Facing*60;
            bShopApproaches &= Facing.X*ExpectedX>.99f && !GetWorld()->LineTraceSingleByChannel(Obstruction,Start,Start+Facing*850,ECC_Visibility,LayoutParams);
        }
        Check(bShopApproaches,TEXT("Every shop faces its public street with an unobstructed approach"));
        bool bSeatViews=Town!=nullptr;
        for(int32 Side:{-1,1}) for(int32 End:{-1,1})
        {
            const auto* Seat=Part(FString::Printf(TEXT("SquareSeat%d_%d"),Side,End));
            const auto* Back=Part(FString::Printf(TEXT("SquareSeatBack%d_%d"),Side,End));
            if(!Seat || !Back) {bSeatViews=false;continue;}
            const FVector Facing(0,End,0);
            const FVector Start=Seat->GetComponentLocation()+Facing*80+FVector(0,0,65);
            FHitResult Obstruction;
            bSeatViews &= FVector::DotProduct(Back->GetComponentLocation()-Seat->GetComponentLocation(),Facing)<-20.f && !GetWorld()->LineTraceSingleByChannel(Obstruction,Start,Start+Facing*600,ECC_Visibility,LayoutParams);
        }
        Check(bSeatViews,TEXT("Public benches face open space with their backs toward the courthouse"));
        WalkRoute(FVector(-850,-180,92));WalkRoute(FVector(-850,-800,92));WalkRoute(FVector(-2200,-800,92));
        WalkRoute(FVector(-2200,4300,92));WalkRoute(FVector(-5350,4300,92));WalkRoute(FVector(-5350,4730,92));
        Check(bRouteClear,TEXT("Resident approach connects the jail and North Lane without crossing a building"));
        auto LookAtResident=[&]
        {
            PC->SetControlRotation(FRotator(-5,90,0));
            Reed->CameraArm->TickComponent(1.f,LEVELTICK_All,nullptr);
            PC->PlayerCameraManager->UpdateCamera(1.f);
        };
        LookAtResident();
        Check(Town && Town->ResidentMesh->GetSkeletalMeshAsset() && Town->ResidentMesh->GetSingleNodeInstance() && Town->ResidentMesh->GetComponentQuat().RotateVector(FVector(0,1,0)).Y<-.99f,TEXT("Resident has period art idle animation and faces the public approach"));
        Check(PC->ReachableFieldAction()==6,TEXT("Resident is available through the normal distance view and occlusion gate"));
        PC->Interact();Press(EKeys::Gamepad_FaceButton_Right);
        Check(!PC->Case()->Report.FieldNotes.Contains(TEXT("ResidentAccount")) && !PC->IsBookOpen(),TEXT("Leaving the resident greeting adds no evidence"));
        PC->Interact();Press(EKeys::Gamepad_FaceButton_Bottom);Press(EKeys::Gamepad_FaceButton_Right);
        Check(!PC->Case()->Report.FieldNotes.Contains(TEXT("ResidentAccount")),TEXT("Hearing but not recording the resident account adds no evidence"));
        PC->Interact();Press(EKeys::Gamepad_FaceButton_Bottom);Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(PC->Case()->Report.FieldNotes.Contains(TEXT("ResidentAccount")) && !PC->IsBookOpen() && !PC->IsMoveInputIgnored(),TEXT("Controller asks records and returns from the resident conversation"));
        const int32 NotesAfterResident=PC->Case()->Report.FieldNotes.Num();
        PC->Interact();Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(PC->Case()->Report.FieldNotes.Num()==NotesAfterResident && PC->Case()->Report.IncludedFacts==Carbon,TEXT("Repeat conversation adds no duplicate or change to the signed carbon"));
        Check(PC->ObjectiveText().Contains(TEXT("write the date")) && !PC->Case()->IsCurrentStateSaved(),TEXT("Resident note prompts a return to the manual save desk"));
        UCLPrototypeSave* ResidentSave=NewObject<UCLPrototypeSave>();ResidentSave->Report=PC->Case()->Report;ResidentSave->World=PC->Case()->World;
        TArray<uint8> ResidentBytes;FCLReportState ResidentReport;FCLWorldState ResidentWorld;bool bResidentTyped=false;
        const bool bResidentSerialized=UGameplayStatics::SaveGameToMemory(ResidentSave,ResidentBytes);
        USaveGame* ResidentLoaded=bResidentSerialized?UGameplayStatics::LoadGameFromMemory(ResidentBytes):nullptr;
        Check(CLSaveValidation::CopyIfValid(ResidentLoaded,ResidentReport,bResidentTyped,ResidentWorld) && ResidentReport.FieldNotes.Contains(TEXT("ResidentAccount")) && ResidentReport.IncludedFacts==Carbon,TEXT("Resident note survives production validation and memory save without rewriting the carbon"));
        PC->SetControlRotation(FRotator(0,-90,0));Reed->CameraArm->TickComponent(1.f,LEVELTICK_All,nullptr);PC->PlayerCameraManager->UpdateCamera(1.f);
        Check(!PC->CanReachResident(),TEXT("Resident cannot be addressed while looking away"));
        WalkRoute(FVector(-5350,4300,92));LookAtResident();
        Check(!PC->CanReachResident(),TEXT("Resident cannot be addressed from the street outside interaction range"));
        WalkRoute(FVector(-2200,4300,92));WalkRoute(FVector(-2200,-800,92));WalkRoute(FVector(-850,-800,92));WalkRoute(FVector(-850,-180,92));WalkRoute(FVector(-350,-180,92));
        WalkRoute(FVector(-100,-180,92));WalkRoute(FVector(-20,-110,92));
        Check(bRouteClear && PC->IsAtDesk() && PC->Case()->Report.FieldNotes.Contains(TEXT("ResidentAccount")),TEXT("The resident investigation loop returns to the jail desk with its note intact"));
        const FCLReportState BeforePruitt=PC->Case()->Report;
        for(const ECLReportStatus Disposition:{ECLReportStatus::Draft,ECLReportStatus::Signed,ECLReportStatus::Held})
        {
            FCLReportState Fixture;Fixture.bRead=true;Fixture.FieldNotes.Add(TEXT("ResidentAccount"));
            if(Disposition!=ECLReportStatus::Draft) Fixture.Submit(Disposition);
            PC->Case()->Report=Fixture;
            PC->ShowBook(false,false,true);
            Check(PC->IsBookOpen(),TEXT("Pruitt opens the recorded resident discussion"));
            Press(EKeys::Gamepad_FaceButton_Right);
            Check(!PC->IsBookOpen() && !PC->IsMoveInputIgnored() && FCLReportState::StaticStruct()->CompareScriptStruct(&PC->Case()->Report,&Fixture,0),TEXT("Controller leaves Pruitt's resident response without changing any report field"));
            PC->ShowBook(false,false,true);Press(EKeys::Gamepad_FaceButton_Bottom);
            Check(PC->IsBookOpen() && FCLReportState::StaticStruct()->CompareScriptStruct(&PC->Case()->Report,&Fixture,0),TEXT("Other county business remains accessible without altering the resident account"));
            Press(EKeys::Escape);
        }
        PC->Case()->Report=BeforePruitt;
        WalkRoute(FVector(-350,-180,92));WalkRoute(FVector(-850,-180,92));WalkRoute(FVector(-850,-800,92));WalkRoute(FVector(-3900,-800,92));
        WalkRoute(FVector(-3900,100,92));WalkRoute(FVector(-3900,700,92));WalkRoute(FVector(-3900,1200,92));
        Check(bRouteClear,TEXT("Courthouse doorway and lobby provide a grounded collision-clear route to the clerk"));
        // The swept route advances positions without frames; settle the shoulder camera before aiming.
        const bool bClerkCameraLag=Reed->CameraArm->bEnableCameraLag;Reed->CameraArm->bEnableCameraLag=false;
        PC->SetControlRotation(FRotator(-5,90,0));Reed->CameraArm->TickComponent(1.f,LEVELTICK_All,nullptr);PC->PlayerCameraManager->UpdateCamera(1.f);
        Reed->CameraArm->bEnableCameraLag=bClerkCameraLag;
        Check(PC->ReachableFieldAction()==7,TEXT("Inez is reachable through the normal interaction gate across the counter"));
        for(const ECLReportStatus Disposition:{ECLReportStatus::Draft,ECLReportStatus::Signed,ECLReportStatus::Held})
        {
            FCLReportState Fixture;Fixture.bRead=true;Fixture.FieldNotes.Add(TEXT("ResidentAccount"));
            if(Disposition!=ECLReportStatus::Draft) Fixture.Submit(Disposition);
            PC->Case()->Report=Fixture;PC->Interact();
            Check(PC->IsBookOpen() && PC->IsMoveInputIgnored(),TEXT("Clerk interaction opens a focused report review"));
            if(Disposition!=ECLReportStatus::Draft) Press(EKeys::Gamepad_FaceButton_Bottom);
            Check(FCLReportState::StaticStruct()->CompareScriptStruct(&PC->Case()->Report,&Fixture,0),TEXT("Presenting a report to Inez preserves all evidence and the original carbon"));
            Press(EKeys::Gamepad_FaceButton_Right);
            Check(!PC->IsBookOpen() && !PC->IsMoveInputIgnored(),TEXT("Controller leaves the clerk counter and restores walking"));
        }
        PC->Case()->Report=BeforePruitt;
        PC->SetControlRotation(FRotator(0,-90,0));Reed->CameraArm->TickComponent(1.f,LEVELTICK_All,nullptr);PC->PlayerCameraManager->UpdateCamera(1.f);
        Check(!PC->CanReachClerk(),TEXT("Clerk cannot be addressed while looking away"));
        WalkRoute(FVector(-3900,700,92));
        Check(!PC->CanReachClerk(),TEXT("Clerk rejects interaction beyond counter range"));
        WalkRoute(FVector(-3900,100,92));WalkRoute(FVector(-3900,-800,92));WalkRoute(FVector(-850,-800,92));WalkRoute(FVector(-850,-180,92));WalkRoute(FVector(-100,-180,92));WalkRoute(FVector(-20,-110,92));
        Check(bRouteClear && PC->IsAtDesk() && FCLReportState::StaticStruct()->CompareScriptStruct(&PC->Case()->Report,&BeforePruitt,0),TEXT("Clerk visit returns to the jail desk with the full report unchanged"));
        WalkRoute(FVector(-350,-180,92));WalkRoute(FVector(-850,-180,92));WalkRoute(FVector(-850,-800,92));WalkRoute(FVector(-2650,-800,92));WalkRoute(FVector(-2650,-1700,92));WalkRoute(FVector(-2650,-2320,92));
        Check(bRouteClear,TEXT("Enterprise doorway and desk connect to the square without obstruction"));
        const bool bNewsLag=Reed->CameraArm->bEnableCameraLag;Reed->CameraArm->bEnableCameraLag=false;
        PC->SetControlRotation(FRotator(-5,-90,0));Reed->CameraArm->TickComponent(1.f,LEVELTICK_All,nullptr);PC->PlayerCameraManager->UpdateCamera(1.f);Reed->CameraArm->bEnableCameraLag=bNewsLag;
        Check(PC->ReachableFieldAction()==8,TEXT("Mara is reachable at her desk through the normal interaction gate"));
        for(const ECLReportStatus Status:{ECLReportStatus::Draft,ECLReportStatus::Signed,ECLReportStatus::Held})
        {
            FCLReportState Fixture;Fixture.bRead=true;if(Status!=ECLReportStatus::Draft) Fixture.Submit(Status);PC->Case()->Report=Fixture;
            PC->Interact();Press(EKeys::Gamepad_FaceButton_Right);
            Check(!PC->Case()->Report.bEnterpriseReviewed,TEXT("Leaving Mara without showing the carbon changes nothing"));
            PC->Interact();Press(EKeys::Gamepad_FaceButton_Bottom);
            Check(PC->Case()->Report.bEnterpriseReviewed==(Status!=ECLReportStatus::Draft),TEXT("Controller handoff posts only a submitted report"));
            if(PC->IsBookOpen()) Press(EKeys::Gamepad_FaceButton_Right);
            Town->RefreshEnterpriseNotice(PC->Case()->Report);
            const FString Printed=Town->NewsLettering->Text.ToString();
            Check(Printed.Contains(Status==ECLReportStatus::Draft?TEXT("COPY AWAITED"):Status==ECLReportStatus::Signed?TEXT("REED SIGNS"):TEXT("STORY HELD")),TEXT("Physical Enterprise board reflects the submitted disposition"));
            const bool bShared=PC->Case()->Report.bEnterpriseReviewed;PC->Case()->Report.bEnterpriseReviewed=false;
            Check(FCLReportState::StaticStruct()->CompareScriptStruct(&PC->Case()->Report,&Fixture,0),TEXT("Newspaper interaction changes no original report field"));PC->Case()->Report.bEnterpriseReviewed=bShared;
        }
        WalkRoute(FVector(-2650,-1700,92));WalkRoute(FVector(-2360,-1710,92));
        Reed->CameraArm->bEnableCameraLag=false;PC->SetControlRotation(FRotator(-5,-90,0));Reed->CameraArm->TickComponent(1.f,LEVELTICK_All,nullptr);PC->PlayerCameraManager->UpdateCamera(1.f);Reed->CameraArm->bEnableCameraLag=bNewsLag;
        Check(PC->ReachableFieldAction()==9,TEXT("Posted notice is readable from the public pavement"));
        PC->Interact();Check(PC->IsBookOpen(),TEXT("Enterprise notice opens for reading"));Press(EKeys::Gamepad_FaceButton_Right);
        Check(!PC->IsMoveInputIgnored(),TEXT("Leaving the newspaper notice restores walking"));
        WalkRoute(FVector(-2650,-1700,92));WalkRoute(FVector(-2650,-800,92));WalkRoute(FVector(-850,-800,92));WalkRoute(FVector(-850,-180,92));WalkRoute(FVector(-100,-180,92));WalkRoute(FVector(-20,-110,92));
        Check(bRouteClear && PC->IsAtDesk(),TEXT("Enterprise visit returns to the manual save desk"));
        PC->Case()->Report=BeforePruitt;




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
    const FVector Positions[]={FVector(-11000,-8500,9500),FVector(-1600,-480,260),FVector(-4340,-1820,190),FVector(-7600,2800,2500),FVector(-4400,-1400,450),FVector(-6250,4250,220),FVector(-5350,-150,210),FVector(-2350,550,210),FVector(-4400,-900,180),FVector(-5350,4570,180),FVector(-1900,1500,155),FVector(-5430,2250,155),FVector(-2460,1200,145),FVector(-2750,-1300,180),FVector(-2290,1090,180)};
    const FVector Targets[]={FVector(-3300,1700,250),FVector(-560,0,260),FVector(-4000,-2330,110),FVector(-2700,5650,150),FVector(-3900,1200,650),FVector(-5500,5550,230),FVector(-6550,1400,210),FVector(-950,2400,240),FVector(-4700,-150,85),FVector(-5350,4930,130),FVector(-1220,1500,155),FVector(-6320,2300,155),FVector(-2100,1200,105),FVector(-2300,-950,0),FVector(-2100,1200,158)};
    const TCHAR* Names[]={TEXT("TownOverview.png"),TEXT("JailFrontage.png"),TEXT("LangLobby.png"),TEXT("ResidentialLane.png"),TEXT("CourthouseDetail.png"),TEXT("HomeDetail.png"),TEXT("WestMarketStreet.png"),TEXT("CourtStreetShops.png"),TEXT("SquareSeating.png"),TEXT("NorthLaneResident.png"),TEXT("DrugstoreDisplay.png"),TEXT("DryGoodsDisplay.png"),TEXT("ReedAppearance.png"),TEXT("GroundTransition.png"),TEXT("ReedPortrait.png")};
    const int32 View=TownReviewStep/2;
    if(View>=UE_ARRAY_COUNT(Positions)+(FParse::Param(FCommandLine::Get(),TEXT("CLStreetReview"))?0:17)) {GetWorldTimerManager().ClearTimer(TownReviewTimer);FPlatformMisc::RequestExitWithStatus(false,0);return;}
    if(View>=UE_ARRAY_COUNT(Positions))
    {
        // Capture the actual Slate conversation and Book in the isolated fixture.
        auto* PC=Cast<ACLPlayerController>(UGameplayStatics::GetPlayerController(this,0));
        const int32 Page=View-UE_ARRAY_COUNT(Positions);
        if(TownReviewStep%2==0 && PC)
        {
            auto Press=[](FKey Key)
            {
                FKeyEvent Event(Key,FModifierKeysState(),0,false,0,0);
                FSlateApplication::Get().ProcessKeyDownEvent(Event);
                FSlateApplication::Get().ProcessKeyUpEvent(Event);
            };
            if(Page==0)
            {
                PC->Case()->Report.FieldNotes.Remove(TEXT("ResidentAccount"));
                PC->ShowBook(false,false,false,6);
            }
            else if(Page==1) Press(EKeys::Gamepad_FaceButton_Bottom);
            else if(Page==2)
            {
                PC->CloseBook();PC->Case()->bTypedCopy=false;PC->ShowBook(false,false,false,6);
                Press(EKeys::Gamepad_FaceButton_Bottom);
            }
            else if(Page==3) {Press(EKeys::Gamepad_FaceButton_Bottom);PC->Case()->bTypedCopy=true;PC->ShowBook();}
            else if(Page==4) Press(EKeys::Gamepad_RightShoulder);
            else if(Page<8)
            {
                PC->CloseBook();FCLReportState Fixture;Fixture.bRead=true;Fixture.FieldNotes.Add(TEXT("ResidentAccount"));
                if(Page>5) Fixture.Submit(Page==6?ECLReportStatus::Signed:ECLReportStatus::Held);
                PC->Case()->Report=Fixture;PC->ShowBook(false,false,true);
            }
            else if(Page<12)
            {
                PC->CloseBook();
                TownReviewCamera->SetActorLocationAndRotation(FVector(-4100,770,185),(FVector(-3900,1400,145)-FVector(-4100,770,185)).Rotation());
                if(Page>8)
                {
                    FCLReportState Fixture;Fixture.bRead=true;Fixture.FieldNotes.Add(TEXT("ResidentAccount"));
                    if(Page>9) Fixture.Submit(Page==10?ECLReportStatus::Signed:ECLReportStatus::Held);
                    PC->Case()->Report=Fixture;PC->ShowBook(false,false,false,7);
                    if(Page>9) Press(EKeys::Gamepad_FaceButton_Bottom);
                }
            }
            else
            {
                PC->CloseBook();
                const FVector Camera=Page==12?FVector(-2650,-1030,220):FVector(-2480,-2070,185);
                const FVector Target=Page==12?FVector(-2650,-1950,220):FVector(-2650,-2550,148);
                TownReviewCamera->SetActorLocationAndRotation(Camera,(Target-Camera).Rotation());
                FCLReportState Fixture;Fixture.bRead=true;Fixture.FieldNotes={TEXT("SalazarStatement"),TEXT("BottleObserved"),TEXT("BankExamined"),TEXT("ResidentAccount")};
                if(Page>=14) {Fixture.Submit(Page==16?ECLReportStatus::Held:ECLReportStatus::Signed);Fixture.ShareWithEnterprise();}
                PC->Case()->Report=Fixture;
                if(TActorIterator<ACLPecosBend> TownIt(GetWorld());TownIt) {TownIt->RefreshEnterpriseNotice(Fixture);}
                if(Page>=14) {PC->ShowBook(false,false,false,9);if(Page==15) Press(EKeys::Gamepad_FaceButton_Bottom);}
            }
            PC->SetPause(false); // Let the QA capture timer advance; normal dialogue stays paused.
        }
        else
        {
            const TCHAR* Pages[]={TEXT("ResidentGreeting.png"),TEXT("ResidentAccount.png"),TEXT("ResidentHandwritten.png"),TEXT("ResidentBook.png"),TEXT("ResidentPeople.png"),TEXT("PruittResidentDraft.png"),TEXT("PruittResidentSigned.png"),TEXT("PruittResidentHeld.png"),TEXT("ClerkLobby.png"),TEXT("ClerkDraft.png"),TEXT("ClerkSigned.png"),TEXT("ClerkHeld.png"),TEXT("EnterpriseFront.png"),TEXT("EnterpriseOffice.png"),TEXT("EnterpriseCopy.png"),TEXT("EnterpriseCopyContinued.png"),TEXT("EnterpriseHeld.png")};
            FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Screenshots/TownReview"),Pages[Page]),Page!=8 && Page!=12 && Page!=13,false);
        }
        ++TownReviewStep;return;
    }
    if(TownReviewStep%2==0)
    {
        if(View==12)
            if(auto* Pawn=UGameplayStatics::GetPlayerPawn(this,0))
            {
                Pawn->SetActorHiddenInGame(false);
                Pawn->SetActorLocationAndRotation(FVector(-2100,1200,96),FRotator(0,180,0));
            }
        TownReviewCamera->SetActorLocationAndRotation(Positions[View],(Targets[View]-Positions[View]).Rotation());
        TownReviewCamera->GetCameraComponent()->SetFieldOfView(View==2?85.f:70.f);
    }
    else FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Screenshots/TownReview"),Names[View]),FParse::Param(FCommandLine::Get(),TEXT("CLMobilePreview")),false);
    ++TownReviewStep;
}
