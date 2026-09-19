#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CLJailOffice.generated.h"

UCLASS()
class COUNTYLINE_API ACLJailOffice : public AActor
{
    GENERATED_BODY()
public:
    ACLJailOffice();
    virtual void BeginPlay() override;
    FVector ReportLocation() const { return GetActorTransform().TransformPosition(FVector(95, -110, 86)); }
    FVector DeputyLocation() const { return GetActorTransform().TransformPosition(FVector(-120, -310, 135)); }
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> ReportPaper;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class USkeletalMeshComponent> DeputyMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UCapsuleComponent> DeputyCollision;
private:
    class UStaticMeshComponent* Shape(const FString& Name, const TCHAR* Mesh, FVector Position, FVector Size, const TCHAR* Material, bool Collision = true);
    void Label(const FString& Name, const FString& Text, FVector Position, FRotator Rotation, float Size, FColor Color);
};
