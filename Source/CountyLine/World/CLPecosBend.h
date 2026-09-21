#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CLPecosBend.generated.h"

UCLASS()
class COUNTYLINE_API ACLPecosBend : public AActor
{
    GENERATED_BODY()
public:
    ACLPecosBend();
    virtual void BeginPlay() override;
    FVector ResidentLocation() const { return GetActorTransform().TransformPosition(FVector(-5350,4930,130)); }
    UPROPERTY() TObjectPtr<class UCapsuleComponent> ResidentCollision;
    UPROPERTY() TObjectPtr<class USkeletalMeshComponent> ResidentMesh;
    FVector ClerkLocation() const { return GetActorTransform().TransformPosition(FVector(-3900,1400,145)); }
    UPROPERTY() TObjectPtr<class UCapsuleComponent> ClerkCollision;
    FVector MaraLocation() const { return GetActorTransform().TransformPosition(FVector(-2650,-2550,148)); }
    FVector NewsLocation() const { return GetActorTransform().TransformPosition(FVector(-2360,-1925,150)); }
    UPROPERTY() TObjectPtr<class UCapsuleComponent> MaraCollision;
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> NewsBoard;
    UPROPERTY() TObjectPtr<class UTextRenderComponent> NewsLettering;
    void RefreshEnterpriseNotice(const struct FCLReportState& Report);
    FVector RegisterLocation() const { return FVector(-3900,-2200,107); }
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> GuestRegister;
private:
    class UStaticMeshComponent* StreetMesh(const FString& Name,const TCHAR* Asset,FVector Position,FRotator Rotation=FRotator::ZeroRotator,FVector Scale=FVector::OneVector);
    class UStaticMeshComponent* Shape(const FString& Name, FVector Position, FVector Size, const TCHAR* Material, const TCHAR* Mesh=TEXT("Cube"), bool Collision=true);
    void Sign(const FString& Name,const FString& Text,FVector Position,float Yaw,float Size);
    void Store(const FString& Name,const FString& Title,FVector Position,float Width,float Height,float Yaw);
    FString LastEnterpriseHeadline;
    USceneComponent* ConstructionParent = nullptr;
    void Home(const FString& Name,FVector Position,float Width,float Depth,const TCHAR* Material);
};
