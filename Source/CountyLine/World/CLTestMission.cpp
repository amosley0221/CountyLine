#include "World/CLTestMission.h"
#include "Player/CLPlayerController.h"
#include "Player/CLReedCharacter.h"
#include "Paper/CLCaseState.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"

bool FCLTrialState::StartChase()
{
    if(Stage!=ECLTrialStage::Search) return false;
    Stage=ECLTrialStage::Chase;return true;
}
bool FCLTrialState::Catch()
{
    if(Stage!=ECLTrialStage::Chase) return false;
    Stage=ECLTrialStage::Confrontation;return true;
}
bool FCLTrialState::Resolve(ECLTrialChoice InChoice)
{
    if(Stage!=ECLTrialStage::Confrontation ||
       (InChoice!=ECLTrialChoice::Recover && InChoice!=ECLTrialChoice::Verify && InChoice!=ECLTrialChoice::Release) ||
       (InChoice==ECLTrialChoice::Verify && !bManifest)) return false;
    Choice=InChoice;Stage=ECLTrialStage::Complete;return true;
}
void FCLTrialState::Advance(float Seconds,float Length)
{
    if(Stage!=ECLTrialStage::Chase || !FMath::IsFinite(Seconds) || Seconds<=0 || !FMath::IsFinite(Length) || Length<=0) return;
    ChaseSeconds+=Seconds;Distance=FMath::Min(Length,Distance+260.f*Seconds);
    if(Distance>=Length) Stage=ECLTrialStage::Escaped;
}
bool ACLTestMission::IsEnabled() {return FParse::Param(FCommandLine::Get(),TEXT("CLTestMission"));}

ACLTestMission::ACLTestMission()
{
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("TrialRoot"));
    auto* Sky=CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("TrialSky"));Sky->SetupAttachment(RootComponent);
    auto* Sun=CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("TrialSun"));Sun->SetupAttachment(RootComponent);
    Sun->SetMobility(EComponentMobility::Movable);Sun->SetRelativeRotation(FRotator(-38,125,0));
    Sun->SetIntensity(3);Sun->SetLightColor(FLinearColor(1.f,.90f,.74f));Sun->bAtmosphereSunLight=true;Sun->LightSourceAngle=2;
    auto* Ambient=CreateDefaultSubobject<USkyLightComponent>(TEXT("TrialSkyFill"));Ambient->SetupAttachment(RootComponent);
    Ambient->SetMobility(EComponentMobility::Movable);Ambient->SetIntensity(1.25f);Ambient->SetRealTimeCaptureEnabled(true);
    auto Box=[this](const TCHAR* Name,FVector Pos,FVector Size,const TCHAR* Material,bool Collision=true)
    {
        auto* C=CreateDefaultSubobject<UStaticMeshComponent>(Name);C->SetupAttachment(RootComponent);
        C->SetRelativeLocation(Pos);C->SetRelativeScale3D(Size/100.f);
        C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        C->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Art/Materials/M_CL_%s.M_CL_%s"),Material,Material)));
        C->SetCollisionProfileName(Collision?TEXT("BlockAll"):TEXT("NoCollision"));return C;
    };
    auto Label=[this](const TCHAR* Name,const TCHAR* Copy,FVector Pos,float Size=35.f)
    {
        auto* C=CreateDefaultSubobject<UTextRenderComponent>(Name);C->SetupAttachment(RootComponent);
        C->SetRelativeLocation(Pos);C->SetRelativeRotation(FRotator(0,-90,0));C->SetWorldSize(Size);
        C->SetHorizontalAlignment(EHTA_Center);C->SetTextRenderColor(FColor(245,223,163));C->SetText(FText::FromString(Copy));return C;
    };
    Box(TEXT("YardFloor"),FVector(3400,0,-30),FVector(10000,6000,60),TEXT("RoadDust"));
    Box(TEXT("TrialHorizon"),FVector(3400,0,-120),FVector(600000,600000,20),TEXT("Soil"),false);
    Box(TEXT("NorthFence"),FVector(3400,3000,100),FVector(10000,30,200),TEXT("Timber"));
    Box(TEXT("SouthFence"),FVector(3400,-3000,100),FVector(10000,30,200),TEXT("Timber"));
    Box(TEXT("WestFence"),FVector(-1600,0,100),FVector(30,6000,200),TEXT("Timber"));
    Box(TEXT("EastFence"),FVector(8400,0,100),FVector(30,6000,200),TEXT("Timber"));
    Box(TEXT("Warehouse"),FVector(3300,900,260),FVector(2200,1200,520),TEXT("Timber"));
    Box(TEXT("WarehouseRoof"),FVector(3300,900,535),FVector(2300,1300,30),TEXT("Timber"));
    Label(TEXT("WarehouseSign"),TEXT("TEST FREIGHT SHED"),FVector(3300,285,370),52);
    // A long fenced route remains open to the north; the gate offers a direct intercept.
    Box(TEXT("GateFenceSouth"),FVector(2100,-1660,90),FVector(30,2680,180),TEXT("Timber"));
    Gate=Box(TEXT("ShortcutGate"),FVector(2100,0,90),FVector(30,640,180),TEXT("Timber"));
    Box(TEXT("GateFenceNorth"),FVector(2100,900,90),FVector(30,1160,180),TEXT("Timber"));
    Label(TEXT("GateSign"),TEXT("YARD CUT-THROUGH\nUNLATCH GATE"),FVector(2050,-350,210),28)->SetRelativeRotation(FRotator(0,180,0));
    Crate=Box(TEXT("BrokenShipment"),FVector(500,0,50),FVector(140,110,100),TEXT("Timber"));
    Box(TEXT("BrokenLid"),FVector(600,-90,4),FVector(120,70,8),TEXT("Timber"),false);
    Label(TEXT("CrateLabel"),TEXT("BROKEN SHIPMENT\nINSPECT"),FVector(500,-65,145),22);
    Box(TEXT("DispatchTable"),FVector(850,-650,45),FVector(180,100,90),TEXT("Timber"));
    Manifest=Box(TEXT("DispatchSheet"),FVector(850,-650,96),FVector(65,42,2),TEXT("PaperLabel"),false);
    Label(TEXT("ManifestLabel"),TEXT("DISPATCH DESK\nOPTIONAL CLUE"),FVector(850,-710,175),23);
    Label(TEXT("StartSign"),TEXT("NON-CANON PLAYTEST\nTHE MISSING SHIPMENT\nInspect the broken crate. Follow the courier."),FVector(0,150,230),32);
    Label(TEXT("ExitSign"),TEXT("EAST EXIT"),FVector(7600,160,260),50);
    for(int I=0;I<8;++I)
    {
        Box(*FString::Printf(TEXT("FreightStack%d"),I),FVector(2800+I*650,-1400+(I%2)*350,85),FVector(180,180,170),TEXT("Timber"));
        Box(*FString::Printf(TEXT("NorthPost%d"),I),FVector(1000+I*800,2500,120),FVector(24,24,240),TEXT("Timber"));
    }
    // Deliberately reuse a marked test stand-in, with no named story character.
    Runner=CreateDefaultSubobject<USceneComponent>(TEXT("CourierRoot"));Runner->SetupAttachment(RootComponent);
    RunnerMesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CourierStandIn"));RunnerMesh->SetupAttachment(Runner);
    RunnerMesh->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Art/Characters/SK_Reed_Period.SK_Reed_Period")));
    RunnerMesh->SetRelativeRotation(FRotator(0,-90,0));RunnerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Cargo=Box(TEXT("CourierCargo"),FVector::ZeroVector,FVector(60,50,45),TEXT("Timber"),false);
    Cargo->SetupAttachment(Runner);Cargo->SetRelativeLocation(FVector(38,0,105));
    RunnerLabel=Label(TEXT("CourierLabel"),TEXT("TEST COURIER"),FVector::ZeroVector,25);
    RunnerLabel->SetupAttachment(Runner);RunnerLabel->SetRelativeLocation(FVector(0,0,215));
    Route={FVector(1000,0,0),FVector(1600,0,0),FVector(1600,1800,0),FVector(5000,1800,0),FVector(5000,0,0),FVector(7600,0,0)};
    for(int I=1;I<Route.Num();++I) RouteLength+=FVector::Distance(Route[I-1],Route[I]);
    UpdateRunner();
}
void ACLTestMission::BeginPlay()
{
    Super::BeginPlay();
    if(auto* Blend=LoadObject<UBlendSpace>(nullptr,TEXT("/Game/Art/Animations/BS_Reed_FieldLocomotion.BS_Reed_FieldLocomotion"))) RunnerMesh->PlayAnimation(Blend,true);
    if(FParse::Param(FCommandLine::Get(),TEXT("CLTrialSmoke")))
    {
        FTimerHandle T;GetWorldTimerManager().SetTimer(T,this,&ACLTestMission::RunChecks,3.f,false);
    }
}
FVector ACLTestMission::AlongRoute(float Distance) const
{
    for(int I=1;I<Route.Num();++I)
    {
        const float Length=FVector::Distance(Route[I-1],Route[I]);
        if(Distance<=Length) return FMath::Lerp(Route[I-1],Route[I],FMath::Clamp(Distance/Length,0.f,1.f));
        Distance-=Length;
    }
    return Route.Last();
}
FVector ACLTestMission::RunnerPosition() const {return Runner->GetComponentLocation()+FVector(0,0,100);}
void ACLTestMission::UpdateRunner()
{
    const FVector P=AlongRoute(State.Distance);Runner->SetRelativeLocation(P);
    const FVector Direction=AlongRoute(FMath::Min(RouteLength,State.Distance+10))-AlongRoute(FMath::Max(0.f,State.Distance-10));
    Runner->SetRelativeRotation(Direction.Rotation());
    const bool bDeparting=State.Stage==ECLTrialStage::Complete && State.Choice!=ECLTrialChoice::Recover;
    Runner->SetVisibility(State.Stage!=ECLTrialStage::Escaped && !(bDeparting && State.Distance>=RouteLength),true);
    if(auto* Anim=RunnerMesh->GetSingleNodeInstance()) Anim->SetBlendSpacePosition(FVector((State.Stage==ECLTrialStage::Chase || bDeparting)?260:0,0,0));
}
void ACLTestMission::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(ReviewIndex>0) return;
    auto* PC=Cast<ACLPlayerController>(UGameplayStatics::GetPlayerController(this,0));
    if(!PC || !PC->GetPawn()) return;
    if(!bPositioned) {Restart();bPositioned=true;if(!FParse::Param(FCommandLine::Get(),TEXT("CLTrialSmoke"))) PC->ShowBook();}
    if(PC->IsBookOpen()) return;
    const FVector Local=PC->GetPawn()->GetActorLocation()-GetActorLocation();
    if(Local.Z < -150 || Local.X < -1800 || Local.X>8600 || FMath::Abs(Local.Y)>3200) {Restart();Message=TEXT("Returned to the test entrance. Open the briefing with Tab / View.");MessageSeconds=6;}
    MessageSeconds=FMath::Max(0.f,MessageSeconds-DeltaSeconds);
    const auto Before=State.Stage;State.Advance(DeltaSeconds,RouteLength);
    if(State.Stage==ECLTrialStage::Complete && State.Choice!=ECLTrialChoice::Recover) State.Distance=FMath::Min(RouteLength,State.Distance+260.f*DeltaSeconds);
    UpdateRunner();
    RunnerLabel->SetWorldRotation((PC->PlayerCameraManager->GetCameraLocation()-RunnerLabel->GetComponentLocation()).Rotation());
    if(Before==ECLTrialStage::Chase && State.Stage==ECLTrialStage::Escaped) PC->ShowBook();
}
int32 ACLTestMission::ReachableAction(ACLPlayerController* PC) const
{
    if(!PC || !PC->GetPawn()) return -1;
    FVector Eye;FRotator View;PC->GetPlayerViewPoint(Eye,View);
    const FVector Targets[]={Crate->GetComponentLocation()+FVector(0,0,55),Manifest->GetComponentLocation()+FVector(0,0,5),Gate->GetComponentLocation(),RunnerPosition()};
    for(int I=3;I>=0;--I)
    {
        if((I==0 && State.Stage!=ECLTrialStage::Search) || (I==1 && (State.bManifest || (State.Stage!=ECLTrialStage::Search && State.Stage!=ECLTrialStage::Chase))) || (I==2 && (State.bGateOpen || State.Stage==ECLTrialStage::Complete)) || (I==3 && State.Stage!=ECLTrialStage::Chase && State.Stage!=ECLTrialStage::Confrontation)) continue;
        if(!ACLPlayerController::WithinInteractionGate(PC->GetPawn()->GetActorLocation(),Eye,View.Vector(),Targets[I])) continue;
        FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(TrialInteract),false,PC->GetPawn());
        if(!GetWorld()->LineTraceSingleByChannel(Hit,Eye,Targets[I],ECC_Visibility,Params) || (I==0 && Hit.GetComponent()==Crate) || (I==2 && Hit.GetComponent()==Gate)) return I;
    }
    return -1;
}
FString ACLTestMission::Prompt(ACLPlayerController* PC) const
{
    const int I=ReachableAction(PC);
    const TCHAR* Names[]={TEXT("Inspect the broken shipment"),TEXT("Read the dispatch slip"),TEXT("Unlatch the shortcut gate"),TEXT("Call the courier to a stop")};
    return I<0?FString():FString(TEXT("[ E / A ]  "))+Names[I];
}
void ACLTestMission::Interact(ACLPlayerController* PC)
{
    switch(ReachableAction(PC))
    {
    case 0:State.StartChase();Message=TEXT("The crate is empty. The courier bolts with a parcel! Hold Shift / LB to jog.");break;
    case 1:State.bManifest=true;Message=TEXT("Dispatch slip: clinic supplies. Paid collection, but the release signature is missing. You can now challenge the courier with this evidence.");break;
    case 2:State.bGateOpen=true;Gate->SetCollisionEnabled(ECollisionEnabled::NoCollision);Gate->SetVisibility(false);Message=TEXT("Gate open. Cut south of the shed to get ahead of the courier.");break;
    case 3:State.Catch();UpdateRunner();PC->ShowBook();break;
    default:return;
    }
    MessageSeconds=7;
}
FString ACLTestMission::Objective() const
{
    FString Copy=TEXT("NON-CANON PLAYTEST / THE MISSING SHIPMENT\n");
    switch(State.Stage)
    {
    case ECLTrialStage::Search:Copy+=TEXT("Inspect the broken crate. Optional: check the dispatch desk first.");break;
    case ECLTrialStage::Chase:
        Copy+=FString::Printf(TEXT("Stop the courier before the east exit!  %.0f s remaining\nHold Shift / LB to jog. Gate = shortcut. Get close, face him, press E / A."),(RouteLength-State.Distance)/260.f);break;
    case ECLTrialStage::Confrontation:Copy+=TEXT("Courier stopped. Press Tab / View to decide what happens to the shipment.");break;
    case ECLTrialStage::Complete:Copy+=TEXT("Test complete. Tab / View: outcome and replay.");break;
    case ECLTrialStage::Escaped:Copy+=TEXT("Courier escaped. Tab / View: replay.");break;
    }
    if(MessageSeconds>0) Copy+=TEXT("\n\n")+Message;
    return Copy;
}
FString ACLTestMission::Summary() const
{
    if(State.Stage==ECLTrialStage::Escaped) return TEXT("The courier reached the east exit with the shipment. You did not identify its destination.\n\nTry reading the dispatch slip before disturbing the crate, then use the gate south of the shed to intercept him. Jogging matters; the long route costs time.");
    FString Outcome;
    switch(State.Choice)
    {
    case ECLTrialChoice::Recover:Outcome=TEXT("You recover the shipment and hold it at the yard. The cargo is secured, but the clinic receives nothing today. The courier's claim remains unresolved.");break;
    case ECLTrialChoice::Verify:Outcome=TEXT("You match the paid collection slip to the parcel and require a signed handover. The supplies can reach the clinic with a record of who took them. The courier cooperates once you show the evidence.");break;
    case ECLTrialChoice::Release:Outcome=TEXT("You let the courier leave with the parcel on his word. If his claim is true, the clinic gets its supplies quickly. You have no signed handover and cannot confirm the delivery.");break;
    default:break;
    }
    return Outcome+FString::Printf(TEXT("\n\nPursuit: %.1f seconds. Dispatch slip: %s. Shortcut gate: %s.\n\nThis outcome belongs only to this disposable playtest."),State.ChaseSeconds,State.bManifest?TEXT("found"):TEXT("missed"),State.bGateOpen?TEXT("opened"):TEXT("unused"));
}
void ACLTestMission::Resolve(ECLTrialChoice Choice)
{
    if(!State.Resolve(Choice)) return;
    if(Choice==ECLTrialChoice::Recover) {Cargo->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);Cargo->SetWorldLocation(Runner->GetComponentLocation()+FVector(85,0,24));}
    RunnerLabel->SetText(FText::FromString(Choice==ECLTrialChoice::Recover?TEXT("SHIPMENT RECOVERED"):Choice==ECLTrialChoice::Verify?TEXT("HANDOVER VERIFIED"):TEXT("COURIER RELEASED")));
}
void ACLTestMission::Restart()
{
    State=FCLTrialState();Message.Empty();MessageSeconds=0;
    Gate->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Gate->SetVisibility(true);
    Cargo->AttachToComponent(Runner,FAttachmentTransformRules::KeepRelativeTransform);Cargo->SetRelativeLocation(FVector(38,0,105));Cargo->SetRelativeRotation(FRotator::ZeroRotator);
    RunnerLabel->SetText(FText::FromString(TEXT("TEST COURIER")));UpdateRunner();
    if(auto* PC=Cast<ACLPlayerController>(UGameplayStatics::GetPlayerController(this,0)))
    {
        if(auto* Pawn=Cast<ACLReedCharacter>(PC->GetPawn())) {Pawn->SetActorLocation(StartPosition());Pawn->SetActorRotation(FRotator(0,55,0));Pawn->GetCharacterMovement()->StopMovementImmediately();Pawn->Jog(0);}
        PC->SetControlRotation(FRotator(-12,55,0));
    }
}

void ACLTestMission::RunChecks()
{
    bool bPass=true;
    auto Check=[&](bool Value,const TCHAR* What){UE_LOG(LogTemp,Display,TEXT("CL_TRIAL %s: %s"),Value?TEXT("PASS"):TEXT("FAIL"),What);bPass &= Value;};
    auto* PC=Cast<ACLPlayerController>(UGameplayStatics::GetPlayerController(this,0));
    auto* Pawn=PC?Cast<ACLReedCharacter>(PC->GetPawn()):nullptr;
    Check(PC && Pawn && PC->TestMission()==this,TEXT("Isolated playtest actor and controller are active"));
    if(PC && Pawn)
    {
        PC->CloseBook();Restart();
        const FCLReportState ReportBefore=PC->Case()->Report;
        const FCLWorldState WorldBefore=PC->Case()->World;
        Check(!PC->Case()->bHasWrittenDate && PC->Case()->Report.Status==ECLReportStatus::Draft,TEXT("Campaign save was not loaded"));
        Check(!PC->Case()->WriteDate(),TEXT("Campaign saving is refused in playtest"));
        auto Aim=[&](FVector Local,FVector Target)
        {
            Pawn->SetActorLocation(GetActorLocation()+Local);Pawn->GetCharacterMovement()->StopMovementImmediately();
            PC->SetControlRotation((Target-Pawn->GetActorLocation()).Rotation());
            const bool Lag=Pawn->CameraArm->bEnableCameraLag;Pawn->CameraArm->bEnableCameraLag=false;
            Pawn->CameraArm->TickComponent(.016f,LEVELTICK_All,nullptr);PC->PlayerCameraManager->UpdateCamera(.016f);Pawn->CameraArm->bEnableCameraLag=Lag;
        };
        auto Press=[](FKey Key){FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));};
        PC->ShowBook();Check(PC->IsBookOpen() && PC->IsPaused(),TEXT("Briefing opens and pauses the pursuit clock"));
        Press(EKeys::Gamepad_FaceButton_Right);Check(!PC->IsBookOpen() && !PC->IsMoveInputIgnored(),TEXT("Controller B closes briefing and restores walking"));
        Aim(FVector(850,-850,92),Manifest->GetComponentLocation());
        Check(ReachableAction(PC)==1,TEXT("Dispatch evidence is reachable through real view and collision gate"));PC->Interact();
        Check(State.bManifest && State.Stage==ECLTrialStage::Search,TEXT("Exploration clue is optional and does not start the chase"));
        Aim(FVector(500,-230,92),Crate->GetComponentLocation()+FVector(0,0,55));
        Check(ReachableAction(PC)==0,TEXT("Broken crate is reachable"));PC->Interact();
        Check(State.Stage==ECLTrialStage::Chase,TEXT("Inspecting shipment starts live pursuit"));
        Aim(FVector(1880,0,92),Gate->GetComponentLocation());
        Check(ReachableAction(PC)==2,TEXT("Gate latch is reachable from west approach"));
        FHitResult Block;Pawn->SetActorLocation(GetActorLocation()+FVector(2300,0,92),true,&Block);
        Check(Block.bBlockingHit && Block.GetComponent()==Gate,TEXT("Closed gate physically blocks shortcut"));
        Aim(FVector(1880,0,92),Gate->GetComponentLocation());PC->Interact();
        Check(State.bGateOpen && Gate->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("Using latch opens physical shortcut"));
        bool Clear=true;
        auto Walk=[&](FVector Destination)
        {
            const FVector Start=Pawn->GetActorLocation();Destination+=GetActorLocation();
            const float Distance=FVector::Distance(Start,Destination);const int Steps=FMath::CeilToInt(Distance/25);
            for(int I=1;I<=Steps;++I)
            {
                FHitResult Hit,Floor;const FVector Next=FMath::Lerp(Start,Destination,float(I)/Steps);
                Pawn->SetActorLocation(Next,true,&Hit);FCollisionQueryParams Params(SCENE_QUERY_STAT(TrialRoute),false,Pawn);
                Clear &= !Hit.bBlockingHit && GetWorld()->LineTraceSingleByChannel(Floor,Next,Next-FVector(0,0,130),ECC_Visibility,Params) && Floor.ImpactNormal.Z>.7f;
                State.Advance(Distance/Steps/360.f,RouteLength);UpdateRunner();
            }
        };
        // Full accessible player route from entrance via clues and opened gate.
        State.Distance=0;State.ChaseSeconds=0;
        Pawn->SetActorLocation(StartPosition());State.Stage=ECLTrialStage::Search;
        Walk(FVector(500,-230,92));State.StartChase();
        Walk(FVector(1600,-230,92));Walk(FVector(1880,0,92));Walk(FVector(4900,0,92));
        Check(Clear,TEXT("Continuous capsule route reaches intercept through open gate"));
        // Courier's entire fixed route must remain clear, not pass through scenery.
        bool RunnerClear=true;
        // Use real world scenery including this actor's components; ignore only Reed.
        FCollisionQueryParams RunnerParams(SCENE_QUERY_STAT(TrialRunnerRoute),false,Pawn);
        for(int I=1;I<Route.Num();++I)
        {
            FHitResult Hit;
            RunnerClear &= !GetWorld()->SweepSingleByChannel(Hit,GetActorLocation()+Route[I-1]+FVector(0,0,92),GetActorLocation()+Route[I]+FVector(0,0,92),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(32,90),RunnerParams);
        }
        Check(RunnerClear,TEXT("Courier path clears warehouse, fences and ground"));
        Check(State.Stage==ECLTrialStage::Chase && State.Distance<7600,TEXT("Jogging shortcut reaches crossing before courier"));
        State.Distance=7500;UpdateRunner();
        Aim(FVector(4770,100,92),RunnerPosition());
        Check(ReachableAction(PC)==3,TEXT("Courier can be stopped at intercept"));PC->Interact();
        Check(State.Stage==ECLTrialStage::Confrontation && PC->IsBookOpen(),TEXT("Catch opens confrontation"));
        Press(EKeys::Gamepad_FaceButton_Right);
        Check(State.Stage==ECLTrialStage::Confrontation && State.Choice==ECLTrialChoice::None,TEXT("Leaving confrontation does not silently choose outcome"));
        for(auto Choice:{ECLTrialChoice::Recover,ECLTrialChoice::Verify,ECLTrialChoice::Release})
        {
            State.Stage=ECLTrialStage::Confrontation;State.Choice=ECLTrialChoice::None;State.bManifest=true;
            PC->ShowBook();
            const int Steps=Choice==ECLTrialChoice::Recover?0:Choice==ECLTrialChoice::Verify?1:2;
            for(int I=0;I<Steps;++I) Press(EKeys::Gamepad_DPad_Down);
            Press(EKeys::Gamepad_FaceButton_Bottom);
            Check(State.Stage==ECLTrialStage::Complete && State.Choice==Choice,TEXT("Controller selects requested confrontation outcome"));
            PC->CloseBook();
        }
        State.Stage=ECLTrialStage::Confrontation;State.Choice=ECLTrialChoice::None;State.bManifest=false;
        Resolve(ECLTrialChoice::Verify);Check(State.Stage==ECLTrialStage::Confrontation,TEXT("Evidence-gated resolution cannot bypass clue"));
        State.Stage=ECLTrialStage::Chase;State.Advance(100,RouteLength);UpdateRunner();
        Check(State.Stage==ECLTrialStage::Escaped && !RunnerMesh->IsVisible(),TEXT("Missed interception ends with visible escape"));
        PC->ShowBook();Press(EKeys::Gamepad_FaceButton_Bottom);
        Check(State.Stage==ECLTrialStage::Search && !State.bManifest && !State.bGateOpen && !PC->IsBookOpen(),TEXT("Controller replay resets mission and physical gate"));
        Check(FCLReportState::StaticStruct()->CompareScriptStruct(&ReportBefore,&PC->Case()->Report,0) && FCLWorldState::StaticStruct()->CompareScriptStruct(&WorldBefore,&PC->Case()->World,0),TEXT("All trial branches leave campaign report and world untouched"));
        Restart();
    }
    UE_LOG(LogTemp,Display,TEXT("CL_TRIAL_RESULT=%s"),bPass?TEXT("PASS"):TEXT("FAIL"));
    if(bPass && FParse::Param(FCommandLine::Get(),TEXT("CLTrialReview")))
    {
        ReviewFrame();GetWorldTimerManager().SetTimer(ReviewTimer,this,&ACLTestMission::ReviewFrame,3.f,true);
    }
    else FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
}

void ACLTestMission::ReviewFrame()
{
    auto* PC=Cast<ACLPlayerController>(UGameplayStatics::GetPlayerController(this,0));
    if(!PC) return;
    PC->CloseBook();
    if(ReviewIndex>=6) {GetWorldTimerManager().ClearTimer(ReviewTimer);FPlatformMisc::RequestExit(false);return;}
    const int Frame=ReviewIndex++;
    if(Frame==0 || Frame==1)
    {
        auto* Camera=GetWorld()->SpawnActor<ACameraActor>();
        const FVector P=GetActorLocation()+(Frame==0?FVector(-1000,-5500,5200):FVector(300,-850,230));
        const FVector Target=GetActorLocation()+(Frame==0?FVector(3000,0,0):FVector(1200,0,130));
        Camera->SetActorLocation(P);Camera->SetActorRotation((Target-P).Rotation());PC->SetViewTarget(Camera);
        if(Frame==1) {State.StartChase();State.Distance=300;UpdateRunner();}
    }
    else
    {
        State.Stage=ECLTrialStage::Confrontation;State.bManifest=true;
        if(Frame==3) Resolve(ECLTrialChoice::Verify);
        if(Frame==4) State.Stage=ECLTrialStage::Escaped;
        if(Frame==5) State=FCLTrialState();
        PC->ShowBook();PC->SetPause(false);
    }
    const TCHAR* Names[]={TEXT("YardOverview.png"),TEXT("Pursuit.png"),TEXT("Confrontation.png"),TEXT("VerifiedOutcome.png"),TEXT("Escape.png"),TEXT("Briefing.png")};
    FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Screenshots/FreightTrial"),Names[Frame]),Frame!=0,false);
}
