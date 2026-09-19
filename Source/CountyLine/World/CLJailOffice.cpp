#include "World/CLJailOffice.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Materials/MaterialInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/BlendSpace.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"

UStaticMeshComponent* ACLJailOffice::Shape(const FString& Name, const TCHAR* Mesh, FVector Position, FVector Size, const TCHAR* Material, bool Collision)
{
    UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
    Part->SetupAttachment(RootComponent);
    Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Mesh, Mesh)));
    Part->SetRelativeLocation(Position);
    Part->SetRelativeScale3D(Size / 100.f);
    Part->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Prototype/Materials/M_%s.M_%s"), Material, Material)));
    Part->SetCollisionEnabled(Collision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Part->SetCollisionResponseToAllChannels(ECR_Block);
    return Part;
}

void ACLJailOffice::Label(const FString& Name, const FString& Text, FVector Position, FRotator Rotation, float Size, FColor Color)
{
    UTextRenderComponent* T = CreateDefaultSubobject<UTextRenderComponent>(*Name);
    T->SetupAttachment(RootComponent);
    T->SetRelativeLocation(Position);
    T->SetRelativeRotation(Rotation);
    T->SetText(FText::FromString(Text));
    T->SetWorldSize(Size);
    T->SetTextRenderColor(Color);
    T->SetHorizontalAlignment(EHTA_Center);
    T->SetVerticalAlignment(EVRTA_TextCenter);
    T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

ACLJailOffice::ACLJailOffice()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Office"));
    // Pruitt waits clear of the entrance-to-desk route. Both characters use temporary art.
    DeputyCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("DeputyCollision"));
    DeputyCollision->SetupAttachment(RootComponent);
    DeputyCollision->SetRelativeLocation(FVector(-120,-310,90));
    DeputyCollision->InitCapsuleSize(32,90);
    DeputyCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    DeputyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("DeputyPruitt"));
    DeputyMesh->SetupAttachment(RootComponent);
    DeputyMesh->SetRelativeLocation(FVector(-120,-310,0));
    DeputyMesh->SetRelativeRotation(FRotator(0,0,0));
    DeputyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DeputyMesh->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin")));
    DeputyMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    Shape(TEXT("Floor"), TEXT("Cube"), FVector(0,0,-12), FVector(1120,920,24), TEXT("Wood"));
    for (int32 I = 0; I < 23; ++I)
        Shape(FString::Printf(TEXT("FloorJoint%d"), I), TEXT("Cube"), FVector(0,-440+I*40,0.15f), FVector(1100,1,0.3f), TEXT("Ink"), false);
    Shape(TEXT("BackWall"), TEXT("Cube"), FVector(560,0,170), FVector(20,940,340), TEXT("Plaster"));
    Shape(TEXT("LeftWall"), TEXT("Cube"), FVector(0,-460,170), FVector(1140,20,340), TEXT("Plaster"));
    Shape(TEXT("RightWall"), TEXT("Cube"), FVector(0,460,170), FVector(1140,20,340), TEXT("Brick"));
    Shape(TEXT("EntryWall"), TEXT("Cube"), FVector(-560,0,170), FVector(20,940,340), TEXT("Plaster"));
    Shape(TEXT("Ceiling"), TEXT("Cube"), FVector(0,0,350), FVector(1140,940,20), TEXT("Plaster"));
    for (int32 X = -450; X <= 450; X += 225)
        Shape(FString::Printf(TEXT("Beam%d"),X), TEXT("Cube"), FVector(X,0,328), FVector(16,920,25), TEXT("Wood"));
    Shape(TEXT("LeftWainscot"), TEXT("Cube"), FVector(0,-444,48), FVector(1100,8,96), TEXT("Wood"));
    Shape(TEXT("BackWainscot"), TEXT("Cube"), FVector(544,0,48), FVector(8,900,96), TEXT("Wood"));
    Shape(TEXT("Door"), TEXT("Cube"), FVector(-545,-180,118), FVector(9,110,236), TEXT("Wood"));
    Shape(TEXT("DoorGlass"), TEXT("Cube"), FVector(-538,-180,163), FVector(2,80,90), TEXT("Glass"),false);
    Shape(TEXT("DoorKnob"), TEXT("Sphere"), FVector(-529,-139,95), FVector(9), TEXT("Brass"),false);
    Label(TEXT("DoorSign"), TEXT("RIVAS COUNTY\nJAIL OFFICE"), FVector(-535,-180,163), FRotator(0,0,0), 10, FColor(239,229,204));
    // Windows are translucent-looking opaque placeholders; the prototype remains an enclosed room.
    for (int32 Y : {-280, 320})
    {
        Shape(FString::Printf(TEXT("WindowFrame%d"),Y),TEXT("Cube"),FVector(544,Y,207),FVector(12,150,150),TEXT("Wood"));
        Shape(FString::Printf(TEXT("WindowGlass%d"),Y),TEXT("Cube"),FVector(536,Y,207),FVector(3,132,132),TEXT("Glass"),false);
        Shape(FString::Printf(TEXT("WindowMullion%d"),Y),TEXT("Cube"),FVector(531,Y,207),FVector(6,5,132),TEXT("Wood"),false);
        Shape(FString::Printf(TEXT("WindowCross%d"),Y),TEXT("Cube"),FVector(531,Y,207),FVector(6,132,5),TEXT("Wood"),false);
    }
    // Desk and the report are separate components so the interaction trace can identify the paper.
    Shape(TEXT("DeskTop"),TEXT("Cube"),FVector(150,-110,78),FVector(116,204,12),TEXT("Wood"));
    for (int32 X : {109,191}) for (int32 Y : {-193,-27})
        Shape(FString::Printf(TEXT("DeskLeg%d_%d"),X,Y),TEXT("Cube"),FVector(X,Y,35),FVector(12,12,70),TEXT("Wood"));
    Shape(TEXT("Blotter"),TEXT("Cube"),FVector(134,-110,84.4f),FVector(72,105,0.8f),TEXT("Ink"),false);
    ReportPaper = Shape(TEXT("Report26_001"),TEXT("Cube"),FVector(109,-110,85.5f),FVector(27,39,1.4f),TEXT("Paper"));
    for (int32 I=0; I<7; ++I)
        Shape(FString::Printf(TEXT("TypedLine%d"),I),TEXT("Cube"),FVector(108,-121+I*3.3f,86.3f),FVector(19,0.4f,0.1f),TEXT("Ink"),false);
    Shape(TEXT("ReportClip"),TEXT("Cube"),FVector(108,-127,86.8f),FVector(6,2,1),TEXT("Brass"),false);
    Shape(TEXT("Book"),TEXT("Cube"),FVector(150,-39,88),FVector(37,28,7),TEXT("Ledger"));
    Shape(TEXT("BookPages"),TEXT("Cube"),FVector(148,-39,88),FVector(34,25,5),TEXT("Paper"),false);
    // Candlestick telephone proxy, no rotary dial.
    Shape(TEXT("PhoneBase"),TEXT("Cylinder"),FVector(174,-179,87),FVector(22,22,5),TEXT("Ink"),false);
    Shape(TEXT("PhoneStem"),TEXT("Cylinder"),FVector(174,-179,102),FVector(5,5,28),TEXT("Brass"),false);
    Shape(TEXT("PhoneMouth"),TEXT("Sphere"),FVector(174,-179,118),FVector(13,13,9),TEXT("Ink"),false);
    Shape(TEXT("Receiver"),TEXT("Cylinder"),FVector(174,-164,104),FVector(7,7,21),TEXT("Ink"),false);
    Shape(TEXT("ChairSeat"),TEXT("Cube"),FVector(277,-110,47),FVector(46,48,8),TEXT("Wood"));
    Shape(TEXT("ChairBack"),TEXT("Cube"),FVector(297,-110,78),FVector(7,48,64),TEXT("Wood"));
    for(int32 X : {260,293}) for(int32 Y : {-126,-94})
        Shape(FString::Printf(TEXT("ChairLeg%d%d"),X,Y),TEXT("Cube"),FVector(X,Y,22),FVector(5,5,44),TEXT("Wood"));
    // Empty holding cell, cot, bench, and a secured gate.
    for(int32 X=-30; X<=530; X+=28)
        Shape(FString::Printf(TEXT("CellBar%d"),X),TEXT("Cylinder"),FVector(X,180,145),FVector(3,3,290),TEXT("Iron"));
    Shape(TEXT("CellHeader"),TEXT("Cube"),FVector(250,180,288),FVector(580,8,10),TEXT("Iron"));
    Shape(TEXT("CellRail"),TEXT("Cube"),FVector(250,180,100),FVector(580,6,5),TEXT("Iron"));
    Shape(TEXT("CellEnd"),TEXT("Cube"),FVector(-44,315,145),FVector(12,270,290),TEXT("Brick"));
    Shape(TEXT("CotFrame"),TEXT("Cube"),FVector(390,367,40),FVector(200,78,9),TEXT("Iron"));
    Shape(TEXT("CotMattress"),TEXT("Cube"),FVector(390,367,49),FVector(193,71,12),TEXT("Ledger"));
    Shape(TEXT("CotPillow"),TEXT("Cube"),FVector(462,367,59),FVector(39,62,10),TEXT("Paper"),false);
    Shape(TEXT("Bench"),TEXT("Cube"),FVector(-230,410,44),FVector(220,50,12),TEXT("Wood"));
    Shape(TEXT("Cabinet"),TEXT("Cube"),FVector(374,-399,81),FVector(165,75,162),TEXT("Wood"));
    for(int32 I=0;I<4;++I)
    {
        Shape(FString::Printf(TEXT("Drawer%d"),I),TEXT("Cube"),FVector(374,-358,25+I*37),FVector(150,5,31),TEXT("Ledger"));
        Shape(FString::Printf(TEXT("DrawerHandle%d"),I),TEXT("Cube"),FVector(374,-353,25+I*37),FVector(25,4,3),TEXT("Brass"),false);
    }
    Shape(TEXT("NoticeBoard"),TEXT("Cube"),FVector(160,-444,208),FVector(180,8,91),TEXT("Wood"),false);
    Label(TEXT("OfficeSign"),TEXT("RIVAS COUNTY\nACTING SHERIFF S. REED"),FVector(160,-438,211),FRotator(0,90,0),12,FColor(239,229,204));
    Label(TEXT("CellSign"),TEXT("COUNTY JAIL  /  REGISTER AT DESK"),FVector(250,170,308),FRotator(0,-90,0),9,FColor(239,229,204));
    // Small authored props give the report and save station readable silhouettes.
    Shape(TEXT("CountyBookCover"),TEXT("Cube"),FVector(151,-46,86),FVector(34,45,3),TEXT("Ledger"),false);
    Shape(TEXT("CountyBookPages"),TEXT("Cube"),FVector(151,-46,88),FVector(31,42,2),TEXT("Paper"),false);
    Shape(TEXT("CountyBookTop"),TEXT("Cube"),FVector(151,-46,89.5f),FVector(34,45,1),TEXT("Ledger"),false);
    Shape(TEXT("PenRest"),TEXT("Cube"),FVector(131,-78,85.5f),FVector(22,3,2),TEXT("Brass"),false);
    Shape(TEXT("InkBottle"),TEXT("Cylinder"),FVector(168,-76,89),FVector(7,7,10),TEXT("Ink"),false);
    Shape(TEXT("DeskLampBase"),TEXT("Cylinder"),FVector(184,-35,86),FVector(20,20,4),TEXT("Brass"),false);
    Shape(TEXT("DeskLampStem"),TEXT("Cylinder"),FVector(184,-35,103),FVector(3,3,32),TEXT("Brass"),false);
    Shape(TEXT("DeskLampShade"),TEXT("Cone"),FVector(184,-35,121),FVector(29,29,16),TEXT("Ledger"),false);
    auto* DeskLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("DeskReadingLight"));
    DeskLight->SetupAttachment(RootComponent); DeskLight->SetRelativeLocation(FVector(177,-45,112));
    DeskLight->SetIntensity(140); DeskLight->SetAttenuationRadius(240);
    DeskLight->SetLightColor(FLinearColor(1.f,0.78f,0.46f)); DeskLight->SetCastShadows(false);
    DeskLight->SetMobility(EComponentMobility::Movable);
    Shape(TEXT("EntryRunner"),TEXT("Cube"),FVector(-265,-115,0.6f),FVector(330,155,1),TEXT("Ledger"),false);
    Shape(TEXT("NoticePaper"),TEXT("Cube"),FVector(438,-441,204),FVector(48,1,66),TEXT("Paper"),false);
    Label(TEXT("NoticeText"),TEXT("COUNTY BUSINESS\nOFFICE HOURS\n8 TO 5"),FVector(438,-439,205),FRotator(0,90,0),6,FColor(42,37,32));
    // Practical electric interior, with cool fill from the window wall.
    auto* Lamp=CreateDefaultSubobject<UPointLightComponent>(TEXT("OfficeBulb"));
    Lamp->SetupAttachment(RootComponent); Lamp->SetRelativeLocation(FVector(70,-90,285));
    Lamp->SetIntensity(2100); Lamp->SetAttenuationRadius(1050); Lamp->SetLightColor(FLinearColor(1.f,0.87f,0.66f));
    Lamp->SetMobility(EComponentMobility::Movable);
    Shape(TEXT("BulbShade"),TEXT("Cone"),FVector(70,-90,299),FVector(54,54,24),TEXT("Iron"),false);
    Shape(TEXT("Bulb"),TEXT("Sphere"),FVector(70,-90,285),FVector(11),TEXT("Light"),false);
    auto* Fill=CreateDefaultSubobject<URectLightComponent>(TEXT("WindowFill"));
    Fill->SetupAttachment(RootComponent); Fill->SetRelativeLocation(FVector(520,-240,220));
    Fill->SetRelativeRotation(FRotator(0,180,0)); Fill->SetIntensity(1500); Fill->SetAttenuationRadius(1200);
    Fill->SetLightColor(FLinearColor(0.58f,0.72f,1.f)); Fill->SetMobility(EComponentMobility::Movable);
    auto* Ambient=CreateDefaultSubobject<UPointLightComponent>(TEXT("EntryFill"));
    Ambient->SetupAttachment(RootComponent); Ambient->SetRelativeLocation(FVector(-300,0,260));
    Ambient->SetIntensity(1300); Ambient->SetAttenuationRadius(1150); Ambient->SetCastShadows(false);
    Ambient->SetLightColor(FLinearColor(0.73f,0.8f,1.f)); Ambient->SetMobility(EComponentMobility::Movable);
}

void ACLJailOffice::BeginPlay()
{
    Super::BeginPlay();
    if (UBlendSpace* Idle=LoadObject<UBlendSpace>(nullptr,TEXT("/Game/Mannequin/Animations/ThirdPerson_IdleRun_2D.ThirdPerson_IdleRun_2D")))
        DeputyMesh->PlayAnimation(Idle,true);
}
