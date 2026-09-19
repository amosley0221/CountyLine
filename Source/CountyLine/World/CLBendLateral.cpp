#include "World/CLBendLateral.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Animation/BlendSpace.h"
#include "Engine/SkeletalMesh.h"

ACLBendLateral::ACLBendLateral()
{
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    auto* Sky=CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Sky"));Sky->SetupAttachment(RootComponent);
    auto* Sun=CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));Sun->SetupAttachment(RootComponent);
    Sun->SetRelativeRotation(FRotator(-35,-40,0));Sun->SetIntensity(3);Sun->bAtmosphereSunLight=true;
    Salazar=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Salazar"));Salazar->SetupAttachment(RootComponent);
    Salazar->SetRelativeLocation(FVector(-550,-50,0));Salazar->SetRelativeRotation(FRotator(0,0,0));
    Salazar->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin")));
    Salazar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto Shape=[this](const FString& Name,FVector Pos,FVector Size,const TCHAR* Mat)
    {
        auto* C=CreateDefaultSubobject<UStaticMeshComponent>(*Name);
        C->SetupAttachment(RootComponent); C->SetRelativeLocation(Pos); C->SetRelativeScale3D(Size/100.f);
        C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        C->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Prototype/Materials/M_%s.M_%s"),Mat,Mat)));
        C->SetCollisionProfileName(TEXT("BlockAll")); return C;
    };
    Shape(TEXT("Ground"),FVector(0,0,-40),FVector(3600,2600,80),TEXT("Plaster"));
    Shape(TEXT("Road"),FVector(0,-500,2),FVector(3400,420,4),TEXT("Wood"));
    Shape(TEXT("Water"),FVector(100,520,4),FVector(3000,190,8),TEXT("Glass"));
    Shape(TEXT("FarBank"),FVector(0,700,35),FVector(3400,180,70),TEXT("Brick"));
    Shape(TEXT("NearBank"),FVector(0,350,20),FVector(3400,140,40),TEXT("Brick"));
    for(int32 I=0;I<4;++I)
    {
        const FVector P=I==0?FVector(-1250,-450,85):I==1?FVector(-550,-50,90):I==2?FVector(150,255,40):FVector(850,255,50);
        Markers.Add(Shape(FString::Printf(TEXT("Marker%d"),I),P,I==1?FVector(45,45,180):I==2?FVector(12,12,45):FVector(65,20,100),I==2?TEXT("Glass"):TEXT("Ledger")));
        if(I==1) Markers.Last()->SetVisibility(false);
        auto* Label=CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("Label%d"),I));
        Label->SetupAttachment(RootComponent);Label->SetRelativeLocation(P+FVector(0,-20,125));Label->SetRelativeRotation(FRotator(0,-90,0));
        const TCHAR* Names[]={TEXT("JAIL OFFICE"),TEXT("SALAZAR"),TEXT("BOTTLE"),TEXT("DITCH BANK")};
        Label->SetText(FText::FromString(Names[I]));Label->SetWorldSize(24);Label->SetHorizontalAlignment(EHTA_Center);
    }
    Shape(TEXT("NorthLimit"),FVector(0,1300,150),FVector(3600,40,300),TEXT("Wood"));
    Shape(TEXT("SouthLimit"),FVector(0,-1300,150),FVector(3600,40,300),TEXT("Wood"));
    Shape(TEXT("EastLimit"),FVector(1800,0,150),FVector(40,2600,300),TEXT("Wood"));
    Shape(TEXT("WestLimit"),FVector(-1800,0,150),FVector(40,2600,300),TEXT("Wood"));
    for(int32 I=0;I<3;++I)
    {
        auto* Light=CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("DayFill%d"),I));
        Light->SetupAttachment(RootComponent);Light->SetRelativeLocation(FVector((I-1)*1100,0,1100));
        Light->SetIntensity(6000);Light->SetAttenuationRadius(2800);Light->SetLightColor(FLinearColor(1.f,.88f,.68f));Light->SetCastShadows(false);
    }
}
FVector ACLBendLateral::Target(int32 Index) const {return Markers.IsValidIndex(Index)?Markers[Index]->GetComponentLocation():GetActorLocation();}
void ACLBendLateral::BeginPlay()
{
    Super::BeginPlay();
    Salazar->PlayAnimation(LoadObject<UBlendSpace>(nullptr,TEXT("/Game/Mannequin/Animations/ThirdPerson_IdleRun_2D.ThirdPerson_IdleRun_2D")),true);
}
