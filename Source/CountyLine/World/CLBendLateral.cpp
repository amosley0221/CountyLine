#include "World/CLBendLateral.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Animation/BlendSpace.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundAttenuation.h"
#include "Animation/AnimSequence.h"
#include "Materials/MaterialInterface.h"

ACLBendLateral::ACLBendLateral()
{
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    auto* Sky=CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Sky"));
    Sky->SetupAttachment(RootComponent);
    auto* Sun=CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
    Sun->SetupAttachment(RootComponent);Sun->SetMobility(EComponentMobility::Movable);
    Sun->SetRelativeRotation(FRotator(-32,-40,0));Sun->SetIntensity(3);
    Sun->SetLightColor(FLinearColor(1.f,.90f,.74f));Sun->bAtmosphereSunLight=true;
    Sun->LightSourceAngle=2.f;
    auto* Ambient=CreateDefaultSubobject<USkyLightComponent>(TEXT("OpenSkyFill"));
    Ambient->SetupAttachment(RootComponent);Ambient->SetMobility(EComponentMobility::Movable);
    Ambient->SetIntensity(.65f);Ambient->SetRealTimeCaptureEnabled(true);
    auto* Fog=CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("DistanceHaze"));
    Fog->SetupAttachment(RootComponent);Fog->SetFogDensity(.008f);
    Fog->SetFogHeightFalloff(.25f);Fog->SetFogInscatteringColor(FLinearColor(.48f,.49f,.43f));
    for(const TCHAR* Name : {TEXT("SM_BendGround"),TEXT("SM_BendDetails"),TEXT("SM_BendWater"),TEXT("SM_BendFence"),TEXT("SM_BendVegetation"),TEXT("SM_BendHorizon")})
    {
        auto* Mesh=CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Mesh->SetupAttachment(RootComponent);
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Art/BendLateral/%s.%s"),Name,Name)));
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    Salazar=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Salazar"));
    Salazar->SetupAttachment(RootComponent);Salazar->SetRelativeLocation(FVector(-550,-50,0));
    Salazar->SetRelativeRotation(FRotator(0,180,0));
    Salazar->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Art/Characters/SK_Salazar_Period.SK_Salazar_Period")));
    Salazar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto Collision=[this](const FString& Name,FVector Pos,FVector Size,bool bTarget=false)
    {
        auto* C=CreateDefaultSubobject<UStaticMeshComponent>(*Name);C->SetupAttachment(RootComponent);
        C->SetRelativeLocation(Pos);C->SetRelativeScale3D(Size/100.f);
        C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        C->SetHiddenInGame(true);C->SetVisibility(false);C->SetCastShadow(false);
        C->SetCollisionProfileName(TEXT("BlockAll"));
        if(bTarget) {C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);}
        return C;
    };
    Collision(TEXT("Ground"),FVector(0,0,-41),FVector(3600,2600,80));
    for(int32 I=0;I<4;++I)
    {
        const FVector P=I==0?FVector(-1250,-450,100):I==1?FVector(-550,-50,90):I==2?FVector(150,255,24):FVector(850,255,50);
        Markers.Add(Collision(FString::Printf(TEXT("Marker%d"),I),P,I==1?FVector(45,45,180):I==2?FVector(15,15,48):FVector(65,20,100),I!=1));
    }
    // Match the scenery's fences. The channel is not a swimming area in this slice.
    Collision(TEXT("ChannelFence"),FVector(0,350,100),FVector(3600,30,200));
    Collision(TEXT("NorthLimit"),FVector(0,1300,150),FVector(3600,40,300));
    Collision(TEXT("SouthLimit"),FVector(0,-1230,150),FVector(3600,40,300));
    Collision(TEXT("EastLimit"),FVector(1780,0,150),FVector(40,2600,300));
    Collision(TEXT("WestLimitNorth"),FVector(-1780,475,150),FVector(40,1650,300));
    Collision(TEXT("WestLimitSouth"),FVector(-1780,-1215,150),FVector(40,170,300));
    auto* Sign=CreateDefaultSubobject<UTextRenderComponent>(TEXT("RoadSign"));
    Sign->SetupAttachment(RootComponent);Sign->SetRelativeLocation(FVector(-1250,-464,124));
    Sign->SetRelativeRotation(FRotator(0,-90,0));Sign->SetWorldSize(12);
    Sign->SetTextRenderColor(FColor(232,221,184));Sign->SetHorizontalAlignment(EHTA_Center);
    Sign->SetText(FText::FromString(TEXT("PECOS BEND\nJAIL OFFICE")));
    // Shallow, unassigned boot impressions on the accessible bank. Their
    // position is above the water, not a trail establishing a cause of death.
    for(int32 I=0;I<4;++I)
    {
        auto* Print=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("BankPrint%d"),I));
        Print->SetupAttachment(RootComponent);
        Print->SetRelativeLocation(FVector(775+I*35,270+(I%2)*17,.4));
        Print->SetRelativeRotation(FRotator(0,12+(I%2)*8,0));
        Print->SetRelativeScale3D(FVector(.26,.095,.008));
        Print->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
        Print->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Materials/M_CL_Rut.M_CL_Rut")));
        Print->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    auto Audio=[this](const TCHAR* Name,const TCHAR* Path)
    {
        auto* C=CreateDefaultSubobject<UAudioComponent>(Name);
        C->SetupAttachment(RootComponent);C->bAutoActivate=false;
        C->bAllowSpatialization=false;
        C->SetSound(LoadObject<USoundWave>(nullptr,Path));
        return C;
    };
    WindAudio=Audio(TEXT("FieldWind"),TEXT("/Game/Audio/Field/S_BendWind.S_BendWind"));
    BirdsAudio=Audio(TEXT("DistantBirds"),TEXT("/Game/Audio/Field/S_DistantBirds.S_DistantBirds"));
    WaterAudio=Audio(TEXT("ChannelWater"),TEXT("/Game/Audio/Field/S_ChannelWater.S_ChannelWater"));
    WaterAudio->SetRelativeLocation(FVector(0,545,-30));
    WaterAudio->bAllowSpatialization=true;WaterAudio->bOverrideAttenuation=true;
    WaterAudio->AttenuationOverrides.bAttenuate=true;
    WaterAudio->AttenuationOverrides.bSpatialize=true;
    WaterAudio->AttenuationOverrides.AttenuationShape=EAttenuationShape::Box;
    WaterAudio->AttenuationOverrides.AttenuationShapeExtents=FVector(1800,130,100);
    WaterAudio->AttenuationOverrides.FalloffDistance=1100;
}
FVector ACLBendLateral::Target(int32 Index) const
{
    return Markers.IsValidIndex(Index)?Markers[Index]->GetComponentLocation():GetActorLocation();
}
void ACLBendLateral::BeginPlay()
{
    Super::BeginPlay();
    SetWitnessSpeaking(false);
}

void ACLBendLateral::SetWitnessSpeaking(bool bSpeaking)
{
    const TCHAR* Path=bSpeaking?TEXT("/Game/Art/Animations/A_SalazarSpeaking.A_SalazarSpeaking"):TEXT("/Game/Art/Animations/A_FieldIdle.A_FieldIdle");
    if(UAnimSequence* Motion=LoadObject<UAnimSequence>(nullptr,Path)) Salazar->PlayAnimation(Motion,true);
}

void ACLBendLateral::SetFieldActive(bool bActive)
{
    if(bFieldActive==bActive) return;
    bFieldActive=bActive;
    if(bActive)
    {
        WindAudio->FadeIn(.7f,.15f);WaterAudio->FadeIn(.7f,.28f);BirdsAudio->FadeIn(1.f,.045f);
    }
    else
    {
        WindAudio->FadeOut(.4f,0);WaterAudio->FadeOut(.4f,0);BirdsAudio->FadeOut(.4f,0);
    }
}
