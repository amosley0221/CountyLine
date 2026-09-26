#include "World/CLCountyRoad.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

ACLCountyRoad::ACLCountyRoad()
{
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("CountyRoad"));
    auto Box=[this](const FString& Name,FVector Position,FVector Size,const TCHAR* Material,bool Collision=true)
    {
        auto* C=CreateDefaultSubobject<UStaticMeshComponent>(*Name);
        C->SetupAttachment(RootComponent);C->SetRelativeLocation(Position);C->SetRelativeScale3D(Size/100.f);
        C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        C->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Art/Materials/M_CL_%s.M_CL_%s"),Material,Material)));
        if(FString(Material)==TEXT("Soil") || FString(Material)==TEXT("RoadDust"))
            if(auto* Ground=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Town/StreetFinishes/M_CountyGroundV3.M_CountyGroundV3"))) C->SetMaterial(0,Ground);
        if(FString(Material)==TEXT("RoadDust"))
        {
            C->SetRelativeLocation(FVector(Position.X,Position.Y,-1.9f-Size.Z*.5f));
            C->SetCastShadow(false);
        }
        C->SetCollisionEnabled(Collision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
        C->SetCollisionResponseToAllChannels(ECR_Block);
        return C;
    };
    Box(TEXT("RoadGround"),FVector(3400,0,-42),FVector(10000,3600,80),TEXT("Soil"));
    Box(TEXT("DistantGround"),FVector(5000,0,-100),FVector(60000,60000,20),TEXT("Soil"),false);
    Box(TEXT("DirtRoad"),FVector(3800,-800,.6),FVector(9800,550,2),TEXT("RoadDust"),false);
    Box(TEXT("OfficeApproach"),FVector(-850,-350,-1),FVector(570,1250,2),TEXT("RoadDust"),false);
    for(int32 Side:{-1,1})
    {
        Box(FString::Printf(TEXT("Rut%d"),Side),FVector(3800,-800+Side*130,1.8),FVector(9800,14,1),TEXT("Rut"),false);
        for(int32 I=0;I<26;++I)
        {
            const float X=-1500+I*380;
            if(X<800) continue; // Court Street now opens into the town footprint.
            const float Y=Side<0?-1500:1650;
            Box(FString::Printf(TEXT("FencePost%d_%d"),Side,I),FVector(X,Y,70),FVector(12,12,140),TEXT("Timber"));
            if(I<25) for(int32 Rail=0;Rail<2;++Rail)
                Box(FString::Printf(TEXT("FenceRail%d_%d_%d"),Side,I,Rail),FVector(X+190,Y,50+Rail*55),FVector(380,7,8),TEXT("Timber"));
        }
    }
    // Visible perimeter barriers prevent walking off the small prototype ground.
    for(int32 I=0;I<9;++I)
    {
        const float X=800+I*850;
        Box(FString::Printf(TEXT("NorthRock%d"),I),FVector(X,1100+(I%3)*90,50),FVector(150,120,100),TEXT("Rock"));
    }
    auto Sign=[this,&Box](const TCHAR* Name,FVector P,const TCHAR* Text)
    {
        Box(FString(Name)+TEXT("Post"),P+FVector(0,0,80),FVector(12,12,160),TEXT("Timber"));
        Box(FString(Name)+TEXT("Board"),P+FVector(0,0,155),FVector(270,12,80),TEXT("Timber"));
        auto* T=CreateDefaultSubobject<UTextRenderComponent>(Name);T->SetupAttachment(RootComponent);
        T->SetRelativeLocation(P+FVector(0,-8,155));T->SetRelativeRotation(FRotator(0,-90,0));
        T->SetText(FText::FromString(Text));T->SetWorldSize(20);T->SetHorizontalAlignment(EHTA_Center);
        T->SetTextRenderColor(FColor(239,229,204));T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    };
    Sign(TEXT("OfficeRoadSign"),FVector(-1150,-460,0),TEXT("BEND LATERAL  >\n<  JAIL OFFICE"));
    Sign(TEXT("MidRoadSign"),FVector(4000,-450,0),TEXT("BEND LATERAL  >\n<  PECOS BEND"));
    Sign(TEXT("BendRoadSign"),FVector(7800,-460,0),TEXT("BEND LATERAL  >\nKEEP TO THE BANK"));
}

FName ACLCountyRoad::LocationAt(FVector P)
{
    if(P.Z < -20 || P.Z > 250) return NAME_None;
    if(P.X>-560 && P.X<560 && P.Y>-450 && P.Y<450) return TEXT("JailOffice");
    if(P.X>-4750 && P.X<-3650 && P.Y>-2690 && P.Y<-1700) return TEXT("LangHouse");
    if(P.X>-7620 && P.X<-1520 && P.Y>-4040 && P.Y<3880) return TEXT("CourtStreet");
    if(P.X>-7620 && P.X<1040 && P.Y>=1630 && P.Y<7380) return TEXT("CourtStreet"); // Commercial and residential blocks; same recovery point.
    if(P.X>8220 && P.X<11760 && P.Y>-1200 && P.Y<330) return TEXT("BendLateral");
    if(P.X>=-1520 && P.X<=8220 && P.Y>-1480 && P.Y<1630) return TEXT("CountyRoad");
    return NAME_None;
}

bool ACLCountyRoad::SafeCheckpoint(FName Location,FTransform& OutTransform)
{
    if(Location==TEXT("JailOffice")) OutTransform=FTransform(FRotator(0,0,0),FVector(-350,-180,100));
    else if(Location==TEXT("CountyRoad")) OutTransform=FTransform(FRotator(0,0,0),FVector(4000,-800,100));
    else if(Location==TEXT("BendLateral")) OutTransform=FTransform(FRotator(0,45,0),FVector(8800,-700,100));
    else if(Location==TEXT("CourtStreet")) OutTransform=FTransform(FRotator(0,180,0),FVector(-2200,-800,100));
    else if(Location==TEXT("LangHouse")) OutTransform=FTransform(FRotator(0,-90,0),FVector(-4200,-1920,100));
    else return false;
    return true;
}
