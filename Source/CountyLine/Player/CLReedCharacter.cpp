#include "Player/CLReedCharacter.h"
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
    static ConstructorHelpers::FObjectFinder<UBlendSpace> Locomotion(TEXT("/Game/Mannequin/Animations/ThirdPerson_IdleRun_2D.ThirdPerson_IdleRun_2D"));
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
    if(UBlendSpace* Locomotion=LoadObject<UBlendSpace>(nullptr,TEXT("/Game/Mannequin/Animations/ThirdPerson_IdleRun_2D.ThirdPerson_IdleRun_2D")))
        GetMesh()->PlayAnimation(Locomotion,true);
}

void ACLReedCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (UAnimSingleNodeInstance* Anim = GetMesh()->GetSingleNodeInstance())
        Anim->SetBlendSpacePosition(FVector(GetVelocity().Size2D(), 0, 0));
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
