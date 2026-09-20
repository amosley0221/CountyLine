#include "Player/CLReedCharacter.h"
#include "World/CLCountyRoad.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/InputComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"

ACLReedCharacter::ACLReedCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(32.f, 90.f);
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0, 420, 0);
    GetCharacterMovement()->MaxWalkSpeed = 180.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 1000.f;
    GetCharacterMovement()->MaxAcceleration = 700.f;
    CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ShoulderCamera"));
    CameraArm->SetupAttachment(RootComponent);
    CameraArm->TargetArmLength = 265.f;
    CameraArm->SocketOffset = FVector(0, 48, 55);
    CameraArm->bUsePawnControlRotation = true;
    CameraArm->bEnableCameraLag = true;
    CameraArm->CameraLagSpeed = 10.f;
    CameraArm->ProbeSize = 14.f;
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraArm);
    FollowCamera->FieldOfView = 70.f;
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Body(TEXT("/Game/Art/Characters/SK_Reed_Period.SK_Reed_Period"));
    static ConstructorHelpers::FObjectFinder<UBlendSpace> Locomotion(TEXT("/Game/Art/Animations/BS_Reed_FieldLocomotion.BS_Reed_FieldLocomotion"));
    Footsteps=CreateDefaultSubobject<UAudioComponent>(TEXT("Footsteps"));
    Footsteps->SetupAttachment(RootComponent);Footsteps->bAutoActivate=false;
    Footsteps->bAllowSpatialization=false;Footsteps->SetVolumeMultiplier(.20f);
    DirtStep=LoadObject<USoundWave>(nullptr,TEXT("/Game/Audio/Field/S_StepDirt.S_StepDirt"));
    WoodStep=LoadObject<USoundWave>(nullptr,TEXT("/Game/Audio/Field/S_StepWood.S_StepWood"));
    GetMesh()->SetRelativeLocation(FVector(0, 0, -90));
    GetMesh()->SetRelativeRotation(FRotator(0, -90, 0));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (Body.Succeeded()) GetMesh()->SetSkeletalMesh(Body.Object);
    if (Locomotion.Succeeded())
    {
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        GetMesh()->SetAnimation(Locomotion.Object);
        GetMesh()->Play(true);
    }
}

void ACLReedCharacter::BeginPlay()
{
    Super::BeginPlay();
    LastStepPosition=GetActorLocation();
    if(UBlendSpace* Locomotion=LoadObject<UBlendSpace>(nullptr,TEXT("/Game/Art/Animations/BS_Reed_FieldLocomotion.BS_Reed_FieldLocomotion")))
        GetMesh()->PlayAnimation(Locomotion,true);
}

void ACLReedCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    AnimationSpeed=FMath::FInterpTo(AnimationSpeed,GetVelocity().Size2D(),DeltaSeconds,8.f);
    if (UAnimSingleNodeInstance* Anim = GetMesh()->GetSingleNodeInstance())
        Anim->SetBlendSpacePosition(FVector(AnimationSpeed, 0, 0));
    const float Distance=FVector::Dist2D(GetActorLocation(),LastStepPosition);
    LastStepPosition=GetActorLocation();
    if(Distance<100.f && GetCharacterMovement()->IsMovingOnGround() && GetVelocity().Size2D()>15.f)
    {
        StepDistance+=Distance;
        if(StepDistance>=78.f)
        {
            StepDistance=FMath::Fmod(StepDistance,78.f);
            const FName Place=ACLCountyRoad::LocationAt(GetActorLocation());
            Footsteps->SetSound(Place==TEXT("JailOffice") || Place==TEXT("LangHouse")?WoodStep:DirtStep);
            Footsteps->SetPitchMultiplier((FootstepCount++%2)==0?.96f:1.04f);
            Footsteps->Play();
        }
    }
    else StepDistance=0;
}

void ACLReedCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MoveForward"), this, &ACLReedCharacter::Forward);
    Input->BindAxis(TEXT("MoveRight"), this, &ACLReedCharacter::Right);
    Input->BindAxis(TEXT("LookYaw"), this, &ACLReedCharacter::Yaw);
    Input->BindAxis(TEXT("LookPitch"), this, &ACLReedCharacter::Pitch);
    Input->BindAxis(TEXT("LookYawRate"), this, &ACLReedCharacter::YawRate);
    Input->BindAxis(TEXT("LookPitchRate"), this, &ACLReedCharacter::PitchRate);
}
void ACLReedCharacter::Forward(float V) { if (Controller) AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::X), V); }
void ACLReedCharacter::Right(float V) { if (Controller) AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::Y), V); }
void ACLReedCharacter::Yaw(float V) { AddControllerYawInput(V); }
void ACLReedCharacter::Pitch(float V) { AddControllerPitchInput(V); }
void ACLReedCharacter::YawRate(float V) { AddControllerYawInput(V * 65.f * GetWorld()->GetDeltaSeconds()); }
void ACLReedCharacter::PitchRate(float V) { AddControllerPitchInput(V * 50.f * GetWorld()->GetDeltaSeconds()); }
