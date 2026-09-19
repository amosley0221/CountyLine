#include "Player/CLPlayerController.h"
#include "World/CLJailOffice.h"
#include "World/CLBendLateral.h"
#include "Paper/CLCaseState.h"
#include "UI/SCLCountyBook.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"

void ACLPlayerController::BeginPlay()
{
    Super::BeginPlay();
    for(TActorIterator<ACLJailOffice> It(GetWorld());It;++It) {Office=*It;break;}
    Bend=GetWorld()->SpawnActor<ACLBendLateral>(FVector(10000,0,0),FRotator::ZeroRotator);
    PlayerCameraManager->ViewPitchMin=-45;
    PlayerCameraManager->ViewPitchMax=30;
    SetControlRotation(FRotator(-10,0,0));
    SetInputMode(FInputModeGameOnly());
    if(!IsLocalController() || !GEngine || !GEngine->GameViewport) return;
    HUD=SNew(SOverlay)
        +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(32)
        [SNew(STextBlock).Text_Lambda([this]{return FText::FromString(FString(TEXT("THE COUNTY LINE\n"))+ObjectiveText());}).Font(FCoreStyle::GetDefaultFontStyle("Regular",20)).ShadowOffset(FVector2D(1,1)).ColorAndOpacity(FLinearColor(0.88f,0.82f,0.68f))]
        +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(24,24,24,88)
        [SNew(SBorder).Padding(14).BorderBackgroundColor(FLinearColor(0.035f,0.03f,0.02f,0.9f))
            .Visibility_Lambda([this]{return (bPromptAvailable||bDeputyAvailable||ReachableFieldAction()>=0)&&!IsBookOpen()?EVisibility::HitTestInvisible:EVisibility::Collapsed;})
            [SNew(STextBlock).Text_Lambda([this]{const int32 Field=ReachableFieldAction();if(Field>=0) {const TCHAR* Names[]={TEXT("Return to the jail office"),TEXT("Speak with Salazar"),TEXT("Inspect the bottle"),TEXT("Examine the ditch bank"),TEXT("Take the road to Bend Lateral")};return FText::FromString(FString(TEXT("[ E / A ]   "))+Names[Field]);}return FText::FromString(bDeputyAvailable?TEXT("[ E / A ]   Talk   ·   Deputy Pruitt"):TEXT("[ E / A ]   Read   ·   Reed's report"));}).Font(FCoreStyle::GetDefaultFontStyle("Regular",24)).ColorAndOpacity(FLinearColor(0.95f,0.86f,0.65f))]]
        +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(24)
        [SNew(STextBlock).Text(FText::FromString(TEXT("WASD / LS  Walk     Mouse / RS  Look\nE / A  Interact     Tab / View  Book     Esc / Menu  Pause"))).Justification(ETextJustify::Center).Font(FCoreStyle::GetDefaultFontStyle("Regular",20)).ShadowOffset(FVector2D(1,1)).ColorAndOpacity(FLinearColor(0.93f,0.88f,0.77f))];
    GEngine->GameViewport->AddViewportWidgetContent(HUD.ToSharedRef(),0);
}

void ACLPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    EndInspection();
    if(InspectionCamera) InspectionCamera->Destroy();
    if(GEngine && GEngine->GameViewport)
    {
        if(Book.IsValid()) GEngine->GameViewport->RemoveViewportWidgetContent(Book.ToSharedRef());
        if(HUD.IsValid()) GEngine->GameViewport->RemoveViewportWidgetContent(HUD.ToSharedRef());
    }
    Book.Reset(); HUD.Reset();
    Super::EndPlay(Reason);
}

void ACLPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction(TEXT("Interact"),IE_Pressed,this,&ACLPlayerController::Interact);
    InputComponent->BindAction(TEXT("CountyBook"),IE_Pressed,this,&ACLPlayerController::ToggleBook);
    InputComponent->BindAction(TEXT("PauseMenu"),IE_Pressed,this,&ACLPlayerController::PauseMenu);
}

void ACLPlayerController::PlayerTick(float DeltaSeconds)
{
    Super::PlayerTick(DeltaSeconds);
    bPromptAvailable=CanReachReport();
    bDeputyAvailable=CanReachDeputy();
}

bool ACLPlayerController::WithinInteractionGate(FVector PawnPosition,FVector Eye,FVector Forward,FVector Target)
{
    return FVector::DistSquared(PawnPosition,Target)<=FMath::Square(250.f) && FVector::DotProduct(Forward.GetSafeNormal(),(Target-Eye).GetSafeNormal())>=FMath::Cos(FMath::DegreesToRadians(30.f));
}

bool ACLPlayerController::IsAtDesk() const
{
    if(!Office.IsValid() || !GetPawn()) return false;
    return FVector::DistSquared(GetPawn()->GetActorLocation(),Office->ReportLocation())<=FMath::Square(250.f);
}

bool ACLPlayerController::CanReachReport() const
{
    if(!Office.IsValid() || !GetPawn()) return false;
    FVector Eye; FRotator View; GetPlayerViewPoint(Eye,View);
    const FVector Target=Office->ReportLocation();
    if(!WithinInteractionGate(GetPawn()->GetActorLocation(),Eye,View.Vector(),Target)) return false;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ReportInteraction),false,GetPawn());
    // Trace to just above the paper so the tabletop is not mistaken for an occluder.
    const bool bHit=GetWorld()->LineTraceSingleByChannel(Hit,Eye,Target+FVector(0,0,3),ECC_Visibility,Params);
    return !bHit || Hit.GetComponent()==Office->ReportPaper;
}

UCLCaseState* ACLPlayerController::Case() const {return GetGameInstance()->GetSubsystem<UCLCaseState>();}
bool ACLPlayerController::CanReachDeputy() const
{
    if(!Office.IsValid() || !GetPawn()) return false;
    FVector Eye; FRotator View; GetPlayerViewPoint(Eye,View);
    const FVector Target=Office->DeputyLocation();
    if(!WithinInteractionGate(GetPawn()->GetActorLocation(),Eye,View.Vector(),Target)) return false;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DeputyInteraction),false,GetPawn());
    const bool bHit=GetWorld()->LineTraceSingleByChannel(Hit,Eye,Target,ECC_Visibility,Params);
    return !bHit || Hit.GetComponent()==Office->DeputyCollision;
}

FString ACLPlayerController::ObjectiveText() const
{
    const UCLCaseState* State=Case();
    if(!State) return TEXT("Jail office");
    if(IsInField()) return TEXT("Bend Lateral: speak with Salazar. Follow the fence east to inspect the bottle and bank.");
    if(State->Report.Status!=ECLReportStatus::Draft)
        return State->IsCurrentStateSaved()?TEXT("Date written. This office study is complete."):TEXT("Write the date at the desk to save your decision.");
    if(State->Report.bRead) return TEXT("Investigate Bend Lateral through the office door, or review and submit at the desk.");
    if(State->Report.bBriefedByPruitt) return TEXT("Read the cream report on the desk.");
    return TEXT("Speak with Deputy Pruitt, to the left of the desk.");
}

void ACLPlayerController::Interact()
{
    if(IsBookOpen()) return;
    const int32 Action=ReachableFieldAction();
    if(Action>=0) ShowBook(false,false,false,Action);
    else if(CanReachDeputy()) ShowBook(false,false,true);
    else if(CanReachReport()) ShowBook(!Case()->Report.bRead);
}
void ACLPlayerController::ToggleBook() {if(IsBookOpen()) CloseBook(); else ShowBook();}
void ACLPlayerController::PauseMenu() {if(IsBookOpen()) CloseBook(); else ShowBook(false,true);}

void ACLPlayerController::ShowBook(bool bReportCover,bool bPause,bool bConversation,int32 FieldAction)
{
    if(IsBookOpen() || !GEngine || !GEngine->GameViewport) return;
    if(ACharacter* C=Cast<ACharacter>(GetPawn())) C->GetCharacterMovement()->StopMovementImmediately();
    SetIgnoreMoveInput(true);SetIgnoreLookInput(true);
    if(FieldAction>=1 && FieldAction<=3 && IsInField()) BeginInspection(FieldAction);
    Book=SNew(SCLCountyBook).Owner(this).ReportCover(bReportCover).Pause(bPause).Conversation(bConversation).FieldAction(FieldAction);
    GEngine->GameViewport->AddViewportWidgetContent(Book.ToSharedRef(),10);
    bShowMouseCursor=true;
    FInputModeUIOnly Mode;Mode.SetWidgetToFocus(Book->InitialFocus());Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);
    SetPause(!IsInspecting());
    if(HUD.IsValid()) HUD->SetVisibility(EVisibility::Hidden);
    FSlateApplication::Get().SetAllUserFocus(Book->InitialFocus(),EFocusCause::SetDirectly);
}

void ACLPlayerController::CloseBook()
{
    if(!Book.IsValid()) return;
    if(GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(Book.ToSharedRef());
    Book.Reset();SetPause(false);
    EndInspection();
    if(HUD.IsValid()) HUD->SetVisibility(EVisibility::HitTestInvisible);
    ResetIgnoreMoveInput();ResetIgnoreLookInput();bShowMouseCursor=false;
    SetInputMode(FInputModeGameOnly());
    FSlateApplication::Get().SetAllUserFocusToGameViewport();
}


bool ACLPlayerController::IsInField() const
{
    return GetPawn() && GetPawn()->GetActorLocation().X>7000;
}
int32 ACLPlayerController::ReachableFieldAction() const
{
    if(!GetPawn() || !Bend.IsValid()) return -1;
    FVector Eye; FRotator View; GetPlayerViewPoint(Eye,View);
    for(int32 I=0;I<5;++I)
    {
        if((I==4)==IsInField()) continue;
        const FVector Target=I==4?FVector(-520,-180,118):Bend->Target(I);
        if(!WithinInteractionGate(GetPawn()->GetActorLocation(),Eye,View.Vector(),Target)) continue;
        FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(FieldInteraction),false,GetPawn());
        const bool bHit=GetWorld()->LineTraceSingleByChannel(Hit,Eye,Target,ECC_Visibility,Params);
        if(!bHit || (I<4 && Hit.GetComponent()==Bend->Markers[I]) || (I==4 && Hit.GetActor()==Office.Get())) return I;
    }
    return -1;
}
void ACLPlayerController::TravelToBend(bool bOutbound)
{
    CloseBook();
    if(!GetPawn()) return;
    GetPawn()->SetActorLocation(bOutbound?FVector(8800,-700,100):FVector(-350,-180,100),false,nullptr,ETeleportType::TeleportPhysics);
    SetControlRotation(bOutbound?FRotator(-10,45,0):FRotator(-10,0,0));
    if(Bend.IsValid()) Bend->SetFieldActive(bOutbound);
}

void ACLPlayerController::BeginInspection(int32 Action)
{
    if(!GetPawn() || !Bend.IsValid()) return;
    if(!InspectionCamera) InspectionCamera=GetWorld()->SpawnActor<ACameraActor>();
    if(!InspectionCamera) return;
    InspectionAction=Action;InspectionOrbit=0;InspectionZoom=1;
    bPawnWasHidden=GetPawn()->IsHidden();GetPawn()->SetActorHiddenInGame(true);
    Bend->SetWitnessSpeaking(Action==1);
    UpdateInspectionCamera();
    SetViewTarget(InspectionCamera);
}

void ACLPlayerController::AdjustInspection(float Orbit,float Zoom)
{
    if(!IsInspecting()) return;
    InspectionOrbit=FMath::Clamp(InspectionOrbit+Orbit,-25.f,25.f);
    InspectionZoom=FMath::Clamp(InspectionZoom+Zoom,.8f,1.25f);
    UpdateInspectionCamera();
}

void ACLPlayerController::UpdateInspectionCamera()
{
    if(!IsInspecting() || !Bend.IsValid() || !InspectionCamera) return;
    FVector Focus=Bend->Target(InspectionAction);
    float Radius=115,Height=65;
    if(InspectionAction==1) {Focus.Z+=42;Radius=225;Height=15;}
    if(InspectionAction==3) {Focus+=FVector(100,245,-60);Radius=340;Height=300;}
    const float Angle=FMath::DegreesToRadians(-90.f+InspectionOrbit);
    const FVector Position=Focus+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,Height)*InspectionZoom;
    const FRotator Facing=(Focus-Position).Rotation();
    // Leave clear space for the right-hand notebook without hiding the evidence.
    const FVector Aim=Focus+FRotationMatrix(Facing).GetUnitAxis(EAxis::Y)*Radius*InspectionZoom*.23f;
    InspectionCamera->SetActorLocationAndRotation(Position,(Aim-Position).Rotation());
    InspectionCamera->GetCameraComponent()->SetFieldOfView(50);
}

void ACLPlayerController::EndInspection()
{
    if(!IsInspecting()) return;
    if(GetPawn()) {GetPawn()->SetActorHiddenInGame(bPawnWasHidden);SetViewTarget(GetPawn());}
    if(Bend.IsValid()) Bend->SetWitnessSpeaking(false);
    InspectionAction=-1;
}
