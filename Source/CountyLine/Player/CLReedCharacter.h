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
private:
    void Forward(float Value);
    void Right(float Value);
    void Yaw(float Value);
    void Pitch(float Value);
    void YawRate(float Value);
    void PitchRate(float Value);
};
