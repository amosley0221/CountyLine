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
    FVector RegisterLocation() const { return FVector(-3900,-2200,107); }
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> GuestRegister;
private:
    class UStaticMeshComponent* Shape(const FString& Name, FVector Position, FVector Size, const TCHAR* Material, const TCHAR* Mesh=TEXT("Cube"), bool Collision=true);
    void Sign(const FString& Name,const FString& Text,FVector Position,float Yaw,float Size);
    void Store(const FString& Name,const FString& Title,FVector Position,float Width,float Height);
};
