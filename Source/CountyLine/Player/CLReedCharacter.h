#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CLReedCharacter.generated.h"

UCLASS()
class COUNTYLINE_API ACLReedCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    ACLReedCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class USpringArmComponent> CameraArm;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UCameraComponent> FollowCamera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UAudioComponent> Footsteps;
    int32 GetFootstepCount() const { return FootstepCount; }
    void Jog(float Value);
private:
    UPROPERTY() TObjectPtr<class USoundWave> DirtStep;
    UPROPERTY() TObjectPtr<class USoundWave> WoodStep;
    FVector LastStepPosition = FVector::ZeroVector;
    float StepDistance = 0;
    float AnimationSpeed = 0;
    int32 FootstepCount = 0;
    void Forward(float Value);
    void Right(float Value);
    void Yaw(float Value);
    void Pitch(float Value);
    void YawRate(float Value);
    void PitchRate(float Value);
};
