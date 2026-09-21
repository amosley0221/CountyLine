#include "World/CLPecosBend.h"
#include "Paper/CLCaseState.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Materials/MaterialInterface.h"

UStaticMeshComponent* ACLPecosBend::StreetMesh(const FString& Name,const TCHAR* Asset,FVector P,FRotator Rotation,FVector Scale)
{
    auto* C=CreateDefaultSubobject<UStaticMeshComponent>(*Name);C->SetupAttachment(ConstructionParent?ConstructionParent:RootComponent.Get());
    C->SetRelativeLocation(P);C->SetRelativeRotation(Rotation);C->SetRelativeScale3D(Scale);
    C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Art/Town/StreetKit/%s.%s"),Asset,Asset)));
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);return C;
}

UStaticMeshComponent* ACLPecosBend::Shape(const FString& Name,FVector P,FVector Size,const TCHAR* Material,const TCHAR* Mesh,bool Collision)
{
    auto* C=CreateDefaultSubobject<UStaticMeshComponent>(*Name);C->SetupAttachment(ConstructionParent?ConstructionParent:RootComponent.Get());
    C->SetRelativeLocation(P);C->SetRelativeScale3D(Size/100.f);
    C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),Mesh,Mesh)));
    const bool bPrototype=FString(Material)==TEXT("Brick") || FString(Material)==TEXT("Plaster");
    const FString Asset=bPrototype?FString::Printf(TEXT("/Game/Prototype/Materials/M_%s.M_%s"),Material,Material):FString::Printf(TEXT("/Game/Art/Materials/M_CL_%s.M_CL_%s"),Material,Material);
    C->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,*Asset));
    FString Finish;
    const FString Kind(Material);
    if(Name.Contains(TEXT("Inner")) || Name.Contains(TEXT("Laundry"))) {} // Keep plain interior plaster and cloth.
    else if(Name.StartsWith(TEXT("Home")) && Name.EndsWith(TEXT("Walls")) && Kind==TEXT("Plaster")) Finish=TEXT("Siding");
    else if(Name.Contains(TEXT("Glass"))) Finish=TEXT("Glass");
    else if(Kind==TEXT("Brick")) Finish=TEXT("Brick");
    else if(Kind==TEXT("Timber")) Finish=TEXT("Wood");
    else if(Kind==TEXT("Iron")) Finish=TEXT("Metal");
    else if(Kind==TEXT("Plaster")) Finish=TEXT("Stone");
    if(Name.Contains(TEXT("DisplayInnerBack")))
        if(auto* Backing=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Town/ShopDetails/M_DisplayBacking.M_DisplayBacking"))) C->SetMaterial(0,Backing);
    if(Kind==TEXT("RoadDust"))
        if(auto* Road=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Town/ShopDetails/M_WornRoadV2.M_WornRoadV2"))) C->SetMaterial(0,Road);
    if(!Finish.IsEmpty())
        if(auto* TownMaterial=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Art/Town/StreetFinishes/M_StreetFinish%s.M_StreetFinish%s"),*Finish,*Finish))) C->SetMaterial(0,TownMaterial);
    C->SetCollisionEnabled(Collision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Block);
    return C;
}

void ACLPecosBend::Sign(const FString& Name,const FString& Text,FVector P,float Yaw,float Size)
{
    auto* T=CreateDefaultSubobject<UTextRenderComponent>(*Name);T->SetupAttachment(ConstructionParent?ConstructionParent:RootComponent.Get());
    T->SetRelativeLocation(P);T->SetRelativeRotation(FRotator(0,Yaw,0));T->SetText(FText::FromString(Text));
    T->SetWorldSize(Size);T->SetHorizontalAlignment(EHTA_Center);T->SetVerticalAlignment(EVRTA_TextCenter);
    T->SetTextRenderColor(FColor(235,222,187));T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Each complete shop has a local frame, so doors, awnings and signs all face
// its street. Local front is -Y; +90 faces east and -90 faces west.
void ACLPecosBend::Store(const FString& Name,const FString& Title,FVector P,float Width,float Height,float Yaw)
{
    auto* Block=CreateDefaultSubobject<USceneComponent>(*(Name+TEXT("Block")));
    Block->SetupAttachment(RootComponent);Block->SetRelativeLocation(P);Block->SetRelativeRotation(FRotator(0,Yaw,0));
    ConstructionParent=Block;P=FVector::ZeroVector;
    // Closed-shop collision envelope remains unchanged; visible masonry leaves real display bays.
    Shape(Name+TEXT("Walls"),P+FVector(0,0,Height/2),FVector(Width,700,Height),TEXT("Brick"))->SetVisibility(false);
    Shape(Name+TEXT("RearMass"),FVector(0,70,Height/2),FVector(Width,560,Height),TEXT("Brick"),TEXT("Cube"),false);
    Shape(Name+TEXT("FrontHeader"),FVector(0,-280,(Height+260)/2),FVector(Width,140,Height-260),TEXT("Brick"),TEXT("Cube"),false);
    Shape(Name+TEXT("FrontBase"),FVector(0,-280,34),FVector(Width,140,68),TEXT("Brick"),TEXT("Cube"),false);
    Shape(Name+TEXT("DoorPier"),FVector(0,-280,164),FVector(Width*.28f,140,192),TEXT("Brick"),TEXT("Cube"),false);
    Shape(Name+TEXT("Cornice"),P+FVector(0,0,Height),FVector(Width+35,735,35),TEXT("Plaster"));
    Shape(Name+TEXT("Parapet"),P+FVector(0,-350,Height+65),FVector(Width,35,120),TEXT("Brick"));
    Shape(Name+TEXT("Door"),P+FVector(0,-355,112),FVector(100,12,224),TEXT("Timber"));
    for(int32 Side:{-1,1})
    {
        const float X=Side*Width*.28f, Bay=Width*.28f;
        Shape(Name+FString::Printf(TEXT("OuterPier%d"),Side),FVector(Side*Width*.46f,-280,164),FVector(Width*.08f,140,192),TEXT("Brick"),TEXT("Cube"),false);
        Shape(Name+FString::Printf(TEXT("DisplayInnerBack%d"),Side),FVector(X,-218,160),FVector(Bay,8,180),TEXT("Linen"),TEXT("Cube"),false);
        for(int32 Edge:{-1,1})
        {
            Shape(Name+FString::Printf(TEXT("FrameJamb%d_%d"),Side,Edge),FVector(X+Edge*(Bay/2-5),-360,160),FVector(10,18,180),TEXT("Timber"),TEXT("Cube"),false);
            Shape(Name+FString::Printf(TEXT("FrameRail%d_%d"),Side,Edge),FVector(X,-360,160+Edge*85),FVector(Bay,18,10),TEXT("Timber"),TEXT("Cube"),false);
        }
        auto* Pane=Shape(Name+FString::Printf(TEXT("DisplayPane%d"),Side),FVector(X,-369,160),FVector(Bay-20,2,160),TEXT("Iron"),TEXT("Cube"),false);
        if(auto* Glass=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Town/ShopDetails/M_DisplayGlass.M_DisplayGlass"))) Pane->SetMaterial(0,Glass);
        Pane->SetCastShadow(false);
        Shape(Name+FString::Printf(TEXT("DisplayLamp%d"),Side),FVector(X,-290,240),FVector(18,18,8),TEXT("Linen"),TEXT("Sphere"),false);
        auto* DisplayLight=CreateDefaultSubobject<UPointLightComponent>(*(Name+FString::Printf(TEXT("DisplayLight%d"),Side)));
        DisplayLight->SetupAttachment(Block);DisplayLight->SetRelativeLocation(FVector(X,-300,222));
        DisplayLight->SetIntensity(350);DisplayLight->SetAttenuationRadius(240);
        DisplayLight->SetLightColor(FLinearColor(1.f,.91f,.77f));DisplayLight->SetCastShadows(false);
        DisplayLight->SetMobility(EComponentMobility::Movable);
        for(int32 Row=0;Row<2;++Row)
        {
            Shape(Name+FString::Printf(TEXT("DisplayShelf%d_%d"),Side,Row),FVector(X,-285,90+Row*70),FVector(Bay-24,115,5),TEXT("Timber"),TEXT("Cube"),false);
            for(int32 Item=0;Item<5;++Item)
            {
                const FString Id=Name+FString::Printf(TEXT("Stock%d_%d_%d"),Side,Row,Item);
                const FVector At(X+(Item-2)*(Bay-45)/5,-285+(Item%2)*15,106+Row*70);
                if(Name==TEXT("Drugs"))
                {
                    Shape(Id,At,FVector(16,16,28),TEXT("BottleGlass"),TEXT("Cylinder"),false);
                    Shape(Id+TEXT("Stopper"),At+FVector(0,0,17),FVector(8,8,7),TEXT("Timber"),TEXT("Cylinder"),false);
                    Shape(Id+TEXT("Label"),At+FVector(0,-8,0),FVector(11,1,12),TEXT("PaperLabel"),TEXT("Cube"),false);
                }
                else if(Name==TEXT("DryGoods"))
                {
                    Shape(Id,At,FVector(34,50,18),Item%2?TEXT("Linen"):TEXT("BrownWool"),TEXT("Cube"),false);
                    Shape(Id+TEXT("Fold"),At+FVector(0,0,13),FVector(32,47,7),TEXT("Shirt"),TEXT("Cube"),false);
                }
                else if(Name==TEXT("ClosedShop"))
                {
                    Shape(Id,At,FVector(32,12,9),TEXT("Iron"),TEXT("Cube"),false);
                    Shape(Id+TEXT("Handle"),At+FVector(0,0,18),FVector(6,6,30),TEXT("Timber"),TEXT("Cube"),false);
                }
                else
                {
                    Shape(Id,At,FVector(30,37,26),TEXT("PaperLabel"),TEXT("Cube"),false);
                    Shape(Id+TEXT("Twine"),At+FVector(0,-19,0),FVector(2,1,26),TEXT("Linen"),TEXT("Cube"),false);
                }
            }
        }
        Shape(Name+FString::Printf(TEXT("Mullion%d"),Side),P+FVector(Side*Width*.28f,-377,160),FVector(6,6,160),TEXT("Timber"),TEXT("Cube"),false);
    }
    Shape(Name+TEXT("SignBoard"),P+FVector(0,-370,Height-65),FVector(Width-60,15,78),TEXT("Timber"));
    Sign(Name+TEXT("Lettering"),Title,P+FVector(0,-380,Height-65),-90,32);
    StreetMesh(Name+TEXT("CanvasAwning"),TEXT("SM_StreetAwning"),P,FRotator::ZeroRotator,FVector(Width/1000.f,1,1));
    // Retain simple post collision; the authored kit supplies their visible shape.
    for(int32 Side:{-1,1}) Shape(Name+FString::Printf(TEXT("PorchPost%d"),Side),P+FVector(Side*(Width*.488f),-645,130),FVector(8,8,260),TEXT("Iron"))->SetVisibility(false);
    Shape(Name+TEXT("Boardwalk"),P+FVector(0,-490,2),FVector(Width+30,340,8),TEXT("Timber"));
    for(int32 Side:{-1,1})
    {
        Shape(Name+FString::Printf(TEXT("Pilaster%d"),Side),P+FVector(Side*(Width/2-22),-368,Height/2),FVector(45,35,Height),TEXT("Plaster"),TEXT("Cube"),false);
        Shape(Name+FString::Printf(TEXT("Sill%d"),Side),P+FVector(Side*Width*.28f,-380,65),FVector(Width*.28f+20,40,15),TEXT("Plaster"),TEXT("Cube"),false);
    }
    Shape(Name+TEXT("DoorTransom"),P+FVector(0,-375,245),FVector(100,10,32),TEXT("Iron"),TEXT("Cube"),false);
    Shape(Name+TEXT("DoorHandle"),P+FVector(34,-374,112),FVector(5,8,24),TEXT("Brass"),TEXT("Cube"),false);
    // Recessed panels and divided shopfronts add readable depth at walking distance.
    for(int32 Side:{-1,1})
    {
        const float X=Side*Width*.28f;
        Shape(Name+FString::Printf(TEXT("WindowRail%d"),Side),FVector(X,-379,176),FVector(Width*.28f-12,8,7),TEXT("Timber"),TEXT("Cube"),false);
        Shape(Name+FString::Printf(TEXT("KickPanel%d"),Side),FVector(X,-380,38),FVector(Width*.28f-5,12,44),TEXT("Timber"),TEXT("Cube"),false);
        Shape(Name+FString::Printf(TEXT("KickInset%d"),Side),FVector(X,-387,38),FVector(Width*.28f-30,4,24),TEXT("Iron"),TEXT("Cube"),false);
        for(int32 Dentil=0;Dentil<12;++Dentil)
            Shape(Name+FString::Printf(TEXT("CorniceDentil%d_%d"),Side,Dentil),FVector(Side*(25+Dentil*Width/25),-380,Height-22),FVector(14,28,18),TEXT("Plaster"),TEXT("Cube"),false);
        Shape(Name+FString::Printf(TEXT("SignFrame%d"),Side),FVector(0,-382,Height-65+Side*38),FVector(Width-54,10,5),TEXT("Brass"),TEXT("Cube"),false);
    }
    if(Name==TEXT("Grocer"))
    {
        StreetMesh(Name+TEXT("ProduceDisplay"),TEXT("SM_StreetDisplay"),FVector(-Width*.28f,-440,6));
        Shape(Name+TEXT("ProduceBlock"),FVector(-Width*.28f,-440,30),FVector(210,90,48),TEXT("Timber"))->SetVisibility(false);
        StreetMesh(Name+TEXT("StockBarrel"),TEXT("SM_StreetBarrel"),FVector(Width*.40f,-465,6));
        Shape(Name+TEXT("StockBarrelBlock"),FVector(Width*.40f,-465,51),FVector(70,70,90),TEXT("Timber"),TEXT("Cylinder"))->SetVisibility(false);
    }
    if(Name==TEXT("PostOffice"))
    {
        Shape(Name+TEXT("LetterBox"),FVector(Width*.43f,-399,132),FVector(45,44,65),TEXT("Iron"));
        Shape(Name+TEXT("LetterSlot"),FVector(Width*.43f,-422,150),FVector(31,2,4),TEXT("Hair"),TEXT("Cube"),false);
        Sign(Name+TEXT("Collection"),TEXT("LETTERS"),FVector(Width*.43f,-424,131),-90,7);
    }
    if(Name==TEXT("ClosedShop"))
    {
        Shape(Name+TEXT("RepairBench"),FVector(-Width*.29f,-440,82),FVector(175,75,10),TEXT("Timber"));
        for(int32 Leg:{-1,1}) Shape(Name+FString::Printf(TEXT("RepairLeg%d"),Leg),FVector(-Width*.29f+Leg*65,-440,43),FVector(12,60,80),TEXT("Timber"));
        Shape(Name+TEXT("Vise"),FVector(-Width*.29f+45,-454,96),FVector(32,36,20),TEXT("Iron"),TEXT("Cube"),false);
    }
    if(Name==TEXT("Drugs")) Sign(Name+TEXT("WindowService"),TEXT("PRESCRIPTIONS\nSODA WATER"),FVector(-Width*.28f,-385,153),-90,14);
    if(Name==TEXT("PostOffice")) Sign(Name+TEXT("WindowService"),TEXT("LETTERS\nPARCELS"),FVector(-Width*.28f,-385,153),-90,17);
    ConstructionParent=nullptr;
}

// Small closed homes face the residential lane; porches and yards are walkable.
void ACLPecosBend::Home(const FString& Name,FVector P,float W,float D,const TCHAR* Material)
{
    Shape(Name+TEXT("Foundation"),P+FVector(0,0,18),FVector(W+25,D+25,36),TEXT("Rock"));
    Shape(Name+TEXT("Walls"),P+FVector(0,0,180),FVector(W,D,300),Material);
    const float Rise=D*.24f;
    // Boarded gable ends follow the roof pitch and close the old attic gaps.
    for(int32 Side:{-1,1})
    {
        for(int32 Row=0;Row<12;++Row)
            Shape(Name+FString::Printf(TEXT("Gable%d_%d"),Side,Row),P+FVector(Side*(W/2-5),0,330+(Row+.5f)*Rise/12),FVector(14,D*(1.f-float(Row)/12),Rise/12+1),TEXT("Timber"),TEXT("Cube"),false);
        for(int32 Bay:{-1,1})
        {
            const FVector Window=P+FVector(Side*(W/2+8),Bay*D*.25f,180);
            Shape(Name+FString::Printf(TEXT("SideFrame%d_%d"),Side,Bay),Window,FVector(18,135,160),TEXT("Plaster"),TEXT("Cube"),false);
            Shape(Name+FString::Printf(TEXT("SideGlass%d_%d"),Side,Bay),Window+FVector(Side*14,0,0),FVector(7,110,135),TEXT("Iron"),TEXT("Cube"),false);
            Shape(Name+FString::Printf(TEXT("SideSash%d_%d"),Side,Bay),Window+FVector(Side*20,0,0),FVector(5,110,6),TEXT("Timber"),TEXT("Cube"),false);
        }
        Shape(Name+FString::Printf(TEXT("Corner%d"),Side),P+FVector(Side*(W/2-9),-D/2-7,180),FVector(20,20,300),TEXT("Plaster"),TEXT("Cube"),false);
        Shape(Name+FString::Printf(TEXT("PorchRail%d"),Side),P+FVector(Side*(W*.32f),-D/2-210,100),FVector(W*.28f,10,12),TEXT("Timber"));
        for(int32 Rail=0;Rail<5;++Rail)
            Shape(Name+FString::Printf(TEXT("PorchSpindle%d_%d"),Side,Rail),P+FVector(Side*(W*.18f+Rail*W*.07f),-D/2-210,60),FVector(7,7,75),TEXT("Timber"),TEXT("Cube"),false);
    }
    Shape(Name+TEXT("RidgeCap"),P+FVector(0,0,330+Rise+12),FVector(W+105,24,18),TEXT("Iron"),TEXT("Cube"),false);
    // Deep roof panels overlap at the ridge. Dark attic walls close the gable ends.
    Shape(Name+TEXT("Attic"),P+FVector(0,0,330+Rise*.22f),FVector(W-20,D*.50f,Rise*.44f),TEXT("Timber"));
    for(int32 Side:{-1,1})
    {
        auto* Roof=Shape(Name+FString::Printf(TEXT("Roof%d"),Side),P+FVector(0,Side*D*.25f,330+Rise*.5f),FVector(W+100,D*.60f,35),TEXT("Iron"));
        Roof->SetRelativeRotation(FRotator(0,0,Side*26.f));
        const FVector Window=P+FVector(Side*W*.29f,-D/2-12,185);
        Shape(Name+FString::Printf(TEXT("WindowFrame%d"),Side),Window,FVector(155,20,165),TEXT("Plaster"));
        Shape(Name+FString::Printf(TEXT("WindowGlass%d"),Side),Window+FVector(0,-15,0),FVector(125,8,135),TEXT("Iron"));
        Shape(Name+FString::Printf(TEXT("WindowBar%d"),Side),Window+FVector(0,-21,0),FVector(7,6,135),TEXT("Timber"),TEXT("Cube"),false);
        Shape(Name+FString::Printf(TEXT("PorchPost%d"),Side),P+FVector(Side*(W/2-55),-D/2-210,140),FVector(14,14,280),TEXT("Timber"));
        // Yard side boundaries stop before the public lane.
        Shape(Name+FString::Printf(TEXT("YardRail%d"),Side),P+FVector(Side*(W/2+100),120,80),FVector(10,D+650,14),TEXT("Timber"));
        for(int32 I=0;I<5;++I)
            Shape(Name+FString::Printf(TEXT("YardPost%d_%d"),Side,I),P+FVector(Side*(W/2+100),-D/2-180+I*(D+600)/4,55),FVector(12,12,110),TEXT("Timber"));
    }
    Shape(Name+TEXT("DoorFrame"),P+FVector(0,-D/2-10,145),FVector(122,20,245),TEXT("Plaster"));
    Shape(Name+TEXT("Door"),P+FVector(0,-D/2-24,137),FVector(100,10,222),TEXT("Timber"));
    Shape(Name+TEXT("DoorHandle"),P+FVector(34,-D/2-33,125),FVector(6,8,20),TEXT("Brass"),TEXT("Cube"),false);
    for(int32 Panel=0;Panel<2;++Panel) Shape(Name+FString::Printf(TEXT("DoorPanel%d"),Panel),P+FVector(0,-D/2-31,85+Panel*98),FVector(75,5,75),TEXT("Timber"),TEXT("Cube"),false);
    Shape(Name+TEXT("Porch"),P+FVector(0,-D/2-125,10),FVector(W,250,20),TEXT("Timber"));
    Shape(Name+TEXT("PorchRoof"),P+FVector(0,-D/2-120,285),FVector(W+70,310,20),TEXT("Timber"));
    Shape(Name+TEXT("Chimney"),P+FVector(W*.28f,D*.20f,410),FVector(80,90,390),TEXT("Brick"));
    Shape(Name+TEXT("FrontPath"),P+FVector(0,-D/2-425,1),FVector(140,600,3),TEXT("RoadDust"),TEXT("Cube"),false);
    Shape(Name+TEXT("Bench"),P+FVector(-W*.28f,-D/2-105,52),FVector(170,55,18),TEXT("Timber"));
    for(int32 Side:{-1,1}) Shape(Name+FString::Printf(TEXT("BenchLeg%d"),Side),P+FVector(-W*.28f+Side*65,-D/2-105,25),FVector(14,45,50),TEXT("Timber"));
    Shape(Name+TEXT("Shed"),P+FVector(-W*.28f,D/2+330,115),FVector(290,250,230),TEXT("Timber"));
    auto* ShedRoof=Shape(Name+TEXT("ShedRoof"),P+FVector(-W*.28f,D/2+330,250),FVector(330,300,25),TEXT("Iron"));
    ShedRoof->SetRelativeRotation(FRotator(0,0,8));
    Shape(Name+TEXT("ShedDoor"),P+FVector(-W*.28f,D/2+199,105),FVector(95,8,200),TEXT("Iron"));
    Shape(Name+TEXT("GardenBed"),P+FVector(W*.25f,D/2+290,5),FVector(270,260,10),TEXT("Soil"));
    for(int32 Row=0;Row<3;++Row) for(int32 Plant=0;Plant<4;++Plant)
        Shape(Name+FString::Printf(TEXT("Garden%d_%d"),Row,Plant),P+FVector(W*.25f-90+Row*90,D/2+200+Plant*60,24),FVector(28,28,35),TEXT("Leaf"),TEXT("Sphere"),false);
    for(int32 Side:{-1,1}) Shape(Name+FString::Printf(TEXT("ClothesPost%d"),Side),P+FVector(Side*W*.3f,D/2+650,110),FVector(10,10,220),TEXT("Timber"));
    Shape(Name+TEXT("ClothesLine"),P+FVector(0,D/2+650,205),FVector(W*.6f,2,2),TEXT("Iron"),TEXT("Cube"),false);
    for(int32 I=0;I<3;++I) Shape(Name+FString::Printf(TEXT("Laundry%d"),I),P+FVector(-W*.2f+I*W*.2f,D/2+650,168),FVector(70,3,72),TEXT("Plaster"),TEXT("Cube"),false);
}

ACLPecosBend::ACLPecosBend()
{
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("PecosBend"));
    Shape(TEXT("TownGround"),FVector(-3300,-100,-42),FVector(8800,8200,80),TEXT("Soil"));
    Shape(TEXT("ResidentialGround"),FVector(-3300,5650,-42),FVector(8800,3700,80),TEXT("Soil"));
    Shape(TEXT("ResidentialLane"),FVector(-3300,4300,.2),FVector(8500,600,2),TEXT("RoadDust"),TEXT("Cube"),false);
    Shape(TEXT("WestMarketStreet"),FVector(-5650,1750,.2),FVector(500,5100,2),TEXT("RoadDust"),TEXT("Cube"),false);
    Shape(TEXT("WestServiceLane"),FVector(-7250,1750,.2),FVector(300,5100,2),TEXT("RoadDust"),TEXT("Cube"),false);
    Shape(TEXT("EastShopWalk"),FVector(-1770,2125,1),FVector(230,2700,4),TEXT("RoadDust"),TEXT("Cube"),false);
    Shape(TEXT("CourtNorthExtension"),FVector(-2200,4100,.2),FVector(620,950,2),TEXT("RoadDust"),TEXT("Cube"),false);
    Home(TEXT("HomeWest"),FVector(-5500,5550,0),1000,900,TEXT("Plaster"));
    Home(TEXT("HomeBrick"),FVector(-3650,5650,0),1100,950,TEXT("Brick"));
    Home(TEXT("HomeTimber"),FVector(-1250,5550,0),1000,900,TEXT("Timber"));
    Home(TEXT("HomeEast"),FVector(400,5600,0),850,800,TEXT("Plaster"));
    // Temporary civilian art; this unnamed River Road visitor is not Salazar.
    ResidentCollision=CreateDefaultSubobject<UCapsuleComponent>(TEXT("ResidentCollision"));
    ResidentCollision->SetupAttachment(RootComponent);
    ResidentCollision->SetRelativeLocation(FVector(-5350,4930,105));
    ResidentCollision->InitCapsuleSize(30,85);
    ResidentCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    ResidentMesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ResidentMesh"));
    ResidentMesh->SetupAttachment(RootComponent);
    ResidentMesh->SetRelativeLocation(FVector(-5350,4930,20));
    ResidentMesh->SetRelativeRotation(FRotator(0,180,0));
    ResidentMesh->SetRelativeScale3D(FVector(.94f));
    ResidentMesh->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Art/Characters/SK_Salazar_Period.SK_Salazar_Period")));
    ResidentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Sign(TEXT("ResidentialSign"),TEXT("HOMES  /  NORTH LANE"),FVector(-1300,3750,180),-90,23);
    Shape(TEXT("ResidentialSignBoard"),FVector(-1300,3760,180),FVector(430,15,60),TEXT("Timber"));
    Shape(TEXT("ResidentialSignPost"),FVector(-1300,3760,85),FVector(12,12,170),TEXT("Timber"));
    Shape(TEXT("CourtStreet"),FVector(-2200,-100,.2),FVector(620,7700,2),TEXT("RoadDust"),TEXT("Cube"),false);
    Shape(TEXT("SquareRoad"),FVector(-4100,-800,.2),FVector(7000,600,2),TEXT("RoadDust"),TEXT("Cube"),false);
    Shape(TEXT("CourthouseWalk"),FVector(-3900,-100,1),FVector(2400,760,4),TEXT("RoadDust"),TEXT("Cube"),false);
    // Reference landmark: brick county courthouse, two storeys and a cupola.
    const FVector Court(-3900,1200,0);
    Shape(TEXT("CourthouseMass"),Court+FVector(0,0,700),FVector(1800,1400,640),TEXT("Brick"));
    Shape(TEXT("CourthouseBase"),Court+FVector(0,0,-12),FVector(1860,1460,24),TEXT("Plaster"));
    Shape(TEXT("CourthouseCornice"),Court+FVector(0,0,1020),FVector(1900,1500,60),TEXT("Plaster"));
    Shape(TEXT("CourthouseRoof"),Court+FVector(0,0,1070),FVector(1760,1370,60),TEXT("Iron"));
    // Ground-floor lobby replaces the old solid block; the exterior footprint stays put.
    for(int32 Side:{-1,1})
    {
        Shape(FString::Printf(TEXT("CourtLobbySide%d"),Side),Court+FVector(Side*880,0,190),FVector(40,1400,380),TEXT("Brick"));
        Shape(FString::Printf(TEXT("CourtLobbyFront%d"),Side),Court+FVector(Side*510,-680,190),FVector(780,40,380),TEXT("Brick"));
        Shape(FString::Printf(TEXT("CourtBayPier%d"),Side),Court+FVector(Side*177.5,-735,175),FVector(115,100,350),TEXT("Brick"));
        Shape(FString::Printf(TEXT("CourtDoorJamb%d"),Side),Court+FVector(Side*130,-800,155),FVector(30,30,310),TEXT("Plaster"));
        Shape(FString::Printf(TEXT("CourtOpenDoor%d"),Side),Court+FVector(Side*155,-735,155),FVector(14,170,310),TEXT("Timber"));
        Shape(FString::Printf(TEXT("CourtInnerSide%d"),Side),Court+FVector(Side*856,0,185),FVector(6,1320,370),TEXT("Plaster"),TEXT("Cube"),false);
        Shape(FString::Printf(TEXT("CourtLobbyBench%d"),Side),Court+FVector(Side*650,-200,45),FVector(80,300,14),TEXT("Timber"));
        for(int32 Leg:{-1,1}) Shape(FString::Printf(TEXT("CourtLobbyBenchLeg%d_%d"),Side,Leg),Court+FVector(Side*650,-200+Leg*115,20),FVector(60,14,40),TEXT("Timber"));
    }
    Shape(TEXT("CourtCentralBay"),Court+FVector(0,-735,685),FVector(470,100,670),TEXT("Brick"));
    Shape(TEXT("CourtDoorHeader"),Court+FVector(0,-800,330),FVector(290,30,40),TEXT("Plaster"));
    Shape(TEXT("CourtLobbyHeader"),Court+FVector(0,-680,345),FVector(240,40,70),TEXT("Brick"));
    Shape(TEXT("CourtLobbyBack"),Court+FVector(0,680,190),FVector(1800,40,380),TEXT("Brick"));
    Shape(TEXT("CourtInnerBack"),Court+FVector(0,656,185),FVector(1700,6,370),TEXT("Plaster"),TEXT("Cube"),false);
    Shape(TEXT("CourtLobbyCeiling"),Court+FVector(0,0,377),FVector(1760,1360,6),TEXT("Plaster"),TEXT("Cube"),false);
    Shape(TEXT("CourtCounter"),Court+FVector(0,100,46),FVector(440,80,92),TEXT("Timber"));
    Shape(TEXT("CourtCounterTop"),Court+FVector(0,100,97),FVector(470,96,10),TEXT("Timber"));
    Shape(TEXT("CourtCounterPaper"),Court+FVector(-120,75,103),FVector(35,45,2),TEXT("PaperLabel"),TEXT("Cube"),false);
    Shape(TEXT("CourtStamp"),Court+FVector(-70,85,111),FVector(12,12,16),TEXT("Brass"),TEXT("Cube"),false);
    Sign(TEXT("ClerkCounterName"),TEXT("INEZ PADILLA  /  COUNTY CLERK"),Court+FVector(0,47,68),-90,13);
    Sign(TEXT("ClerkOfficeLabel"),TEXT("COUNTY CLERK\nREPORTS AND MINUTES"),Court+FVector(0,648,265),-90,22);
    auto* LobbyLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("CourtLobbyLight"));
    LobbyLight->SetupAttachment(RootComponent);LobbyLight->SetRelativeLocation(Court+FVector(0,-80,325));
    LobbyLight->SetIntensity(1800);LobbyLight->SetAttenuationRadius(1450);LobbyLight->SetLightColor(FLinearColor(1.f,.88f,.72f));LobbyLight->SetMobility(EComponentMobility::Movable);
    Shape(TEXT("CourtCeilingLamp"),Court+FVector(0,-80,345),FVector(32,32,20),TEXT("Linen"),TEXT("Sphere"),false);
    // Original static proxy, approximately 162 cm; unique animated character art comes later.
    const FVector Inez= Court+FVector(0,200,0);
    ClerkCollision=CreateDefaultSubobject<UCapsuleComponent>(TEXT("ClerkCollision"));
    ClerkCollision->SetupAttachment(RootComponent);ClerkCollision->SetRelativeLocation(Inez+FVector(0,0,81));
    ClerkCollision->InitCapsuleSize(28,81);ClerkCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    auto Figure=[this,Inez](const TCHAR* Name,FVector Offset,FVector Size,const TCHAR* Material,const TCHAR* Mesh=TEXT("Sphere"))
    {return Shape(Name,Inez+Offset,Size,Material,Mesh,false);};
    Figure(TEXT("InezSkirt"),FVector(0,0,53),FVector(48,37,90),TEXT("Trouser"),TEXT("Cone"));
    Figure(TEXT("InezBlouse"),FVector(0,0,110),FVector(42,28,48),TEXT("Linen"));
    Figure(TEXT("InezNeck"),FVector(0,0,139),FVector(10,10,13),TEXT("Skin"));
    Figure(TEXT("InezHead"),FVector(0,0,149),FVector(19,18,25),TEXT("Skin"));
    Figure(TEXT("InezHair"),FVector(0,3,154),FVector(20,17,17),TEXT("Hair"));
    Figure(TEXT("InezBun"),FVector(0,12,148),FVector(12,10,12),TEXT("Hair"));
    Figure(TEXT("InezNose"),FVector(0,-9,148),FVector(4,5,6),TEXT("Skin"));
    Figure(TEXT("InezMouth"),FVector(0,-8.5,142),FVector(6,1.5,1.5),TEXT("Mouth"));
    for(int32 Side:{-1,1})
    {
        Figure(*FString::Printf(TEXT("InezEye%d"),Side),FVector(Side*4,-8,152),FVector(3,2,2),TEXT("EyeWhite"));
        Figure(*FString::Printf(TEXT("InezIris%d"),Side),FVector(Side*4,-9,152),FVector(1.5,1,1.5),TEXT("Iris"));
        Figure(*FString::Printf(TEXT("InezSleeve%d"),Side),FVector(Side*24,-5,111),FVector(13,15,38),TEXT("Linen"));
        Figure(*FString::Printf(TEXT("InezHand%d"),Side),FVector(Side*24,-12,92),FVector(8,11,13),TEXT("Skin"));
        Figure(*FString::Printf(TEXT("InezShoe%d"),Side),FVector(Side*10,-4,6),FVector(12,25,12),TEXT("Leather"));
    }

    Shape(TEXT("CourtFloorBand"),Court+FVector(0,0,510),FVector(1830,1430,28),TEXT("Plaster"),TEXT("Cube"),false);
    for(int32 Side:{-1,1})
    {
        for(int32 Row=0;Row<15;++Row)
            Shape(FString::Printf(TEXT("CourtQuoin%d_%d"),Side,Row),Court+FVector(Side*865,-716,105+Row*60),FVector(Row%2?65:100,38,42),TEXT("Plaster"),TEXT("Cube"),false);
    }
    // Backs point toward the courthouse; seating looks across the forecourt
    // or the open north green, never straight into a nearby wall.
    for(int32 Side:{-1,1}) for(int32 End:{-1,1})
    {
        const FVector Seat(-3900+Side*800,End<0?-150:2600,0);
        Shape(FString::Printf(TEXT("SquareSeat%d_%d"),Side,End),Seat+FVector(0,0,46),FVector(190,60,12),TEXT("Timber"))->SetVisibility(false);
        Shape(FString::Printf(TEXT("SquareSeatBack%d_%d"),Side,End),Seat+FVector(0,-End*26,82),FVector(190,10,70),TEXT("Timber"))->SetVisibility(false);
        StreetMesh(FString::Printf(TEXT("SquareSlatBench%d_%d"),Side,End),TEXT("SM_StreetBench"),Seat,FRotator(0,End<0?0:180,0));
    }
    Sign(TEXT("CourtEntryText"),TEXT("RIVAS COUNTY\nCOURTHOUSE"),Court+FVector(0,-810,410),-90,30);
    for(int32 Floor=0;Floor<2;++Floor) for(int32 Bay=-3;Bay<=3;++Bay)
    {
        if(Bay==0) continue;
        const FVector P=Court+FVector(Bay*245,-710,250+Floor*465);
        const FString Name=FString::Printf(TEXT("CourtWindow%d_%d"),Floor,Bay);
        Shape(Name+TEXT("Stone"),P,FVector(150,25,270),TEXT("Plaster"));
        Shape(Name+TEXT("Glass"),P+FVector(0,-20,0),FVector(120,8,235),TEXT("Iron"));
        Shape(Name+TEXT("Cross"),P+FVector(0,-25,0),FVector(120,8,8),TEXT("Timber"),TEXT("Cube"),false);
        Shape(Name+TEXT("Sill"),P+FVector(0,-28,-145),FVector(178,55,18),TEXT("Plaster"),TEXT("Cube"),false);
        Shape(Name+TEXT("Lintel"),P+FVector(0,-20,145),FVector(180,38,30),TEXT("Plaster"),TEXT("Cube"),false);
        Shape(Name+TEXT("VerticalSash"),P+FVector(0,-26,0),FVector(7,8,235),TEXT("Timber"),TEXT("Cube"),false);
    }
    for(int32 Side:{-1,1}) for(int32 Bay=-2;Bay<=2;++Bay) for(int32 Floor=0;Floor<2;++Floor)
    {
        const FVector P=Court+FVector(Side*910,Bay*240,260+Floor*465);
        const FString Name=FString::Printf(TEXT("CourtSide%d_%d_%d"),Side,Bay,Floor);
        Shape(Name+TEXT("Frame"),P,FVector(25,145,265),TEXT("Plaster"));
        Shape(Name+TEXT("Glass"),P+FVector(Side*18,0,0),FVector(8,115,230),TEXT("Iron"));
    }
    Shape(TEXT("CupolaBase"),Court+FVector(0,0,1180),FVector(370,370,210),TEXT("Brick"));
    Shape(TEXT("CupolaLouver"),Court+FVector(0,0,1370),FVector(240,240,190),TEXT("Iron"));
    for(int32 X:{-1,1}) for(int32 Y:{-1,1}) Shape(FString::Printf(TEXT("CupolaPillar%d%d"),X,Y),Court+FVector(X*130,Y*130,1380),FVector(25,25,210),TEXT("Plaster"));
    Shape(TEXT("CupolaCap"),Court+FVector(0,0,1560),FVector(460,460,250),TEXT("Iron"),TEXT("Cone"));
    Shape(TEXT("CupolaFinial"),Court+FVector(0,0,1720),FVector(12,12,150),TEXT("Iron"),TEXT("Cylinder"));
    Store(TEXT("DryGoods"),TEXT("DRY GOODS"),FVector(-6600,2300,0),950,520,90);
    Store(TEXT("Grocer"),TEXT("GENERAL STORE"),FVector(-6600,1150,0),1000,450,90);
    Store(TEXT("PostOffice"),TEXT("POST OFFICE"),FVector(-950,2800,0),950,560,-90);
    Store(TEXT("Drugs"),TEXT("DRUGS"),FVector(-950,1500,0),950,480,-90);
    Store(TEXT("ClosedShop"),TEXT("REPAIRS"),FVector(-6600,50,0),900,420,90);

    // Jail exterior wraps its existing room without moving its report or deputy.
    Shape(TEXT("JailFrontNorth"),FVector(-579,195,180),FVector(18,550,360),TEXT("Brick"));
    Shape(TEXT("JailFrontSouth"),FVector(-579,-380,180),FVector(18,180,360),TEXT("Brick"));
    Shape(TEXT("JailLintel"),FVector(-579,-185,300),FVector(18,210,100),TEXT("Brick"));
    Shape(TEXT("JailParapet"),FVector(-575,0,410),FVector(40,975,115),TEXT("Brick"));
    Shape(TEXT("JailCornice"),FVector(-590,0,477),FVector(65,1000,25),TEXT("Plaster"));
    Sign(TEXT("JailFrontLettering"),TEXT("RIVAS COUNTY JAIL"),FVector(-612,0,410),180,34);
    Shape(TEXT("JailAwning"),FVector(-700,-180,270),FVector(270,280,15),TEXT("Timber"));

    // Lang's ground-floor lobby: real doorway, counter, seating and coat hooks.
    Shape(TEXT("LangFloor"),FVector(-4200,-2200,-12),FVector(1120,1020,24),TEXT("Timber"));
    Shape(TEXT("LangBack"),FVector(-4200,-2700,175),FVector(1140,20,350),TEXT("Brick"));
    Shape(TEXT("LangInnerBack"),FVector(-4200,-2686,175),FVector(1100,6,340),TEXT("Plaster"),TEXT("Cube"),false);
    Shape(TEXT("LangInnerWest"),FVector(-4746,-2200,175),FVector(6,980,340),TEXT("Plaster"),TEXT("Cube"),false);
    Shape(TEXT("LangInnerEast"),FVector(-3654,-2200,175),FVector(6,980,340),TEXT("Plaster"),TEXT("Cube"),false);
    for(int32 Side:{-1,1})
    {
        Shape(FString::Printf(TEXT("LangSide%d"),Side),FVector(-4200+Side*560,-2200,175),FVector(20,1020,350),TEXT("Brick"));
        Shape(FString::Printf(TEXT("LangFront%d"),Side),FVector(-4200+Side*335,-1700,175),FVector(450,20,350),TEXT("Brick"));
        Shape(FString::Printf(TEXT("LangFrontGlass%d"),Side),FVector(-4200+Side*335,-1685,165),FVector(200,10,180),TEXT("Iron"),TEXT("Cube"),false);
        Shape(FString::Printf(TEXT("LangPorchPost%d"),Side),FVector(-4200+Side*500,-1420,155),FVector(18,18,310),TEXT("Timber"));
    }
    Shape(TEXT("LangDoorHeader"),FVector(-4200,-1700,310),FVector(220,20,80),TEXT("Timber"));
    Shape(TEXT("LangUpperFloor"),FVector(-4200,-2200,540),FVector(1140,1040,390),TEXT("Brick"));
    Shape(TEXT("LangRoof"),FVector(-4200,-2200,750),FVector(1220,1120,50),TEXT("Iron"));
    for(int32 I=-1;I<=1;++I)
    {
        Shape(FString::Printf(TEXT("LangUpperFrame%d"),I),FVector(-4200+I*330,-1670,555),FVector(170,25,230),TEXT("Plaster"));
        Shape(FString::Printf(TEXT("LangUpperGlass%d"),I),FVector(-4200+I*330,-1653,555),FVector(140,8,200),TEXT("Iron"));
        Shape(FString::Printf(TEXT("LangRearFrame%d"),I),FVector(-4200+I*330,-2728,555),FVector(170,25,230),TEXT("Plaster"));
        Shape(FString::Printf(TEXT("LangRearGlass%d"),I),FVector(-4200+I*330,-2745,555),FVector(140,8,200),TEXT("Iron"));
    }
    Shape(TEXT("LangPorchRoof"),FVector(-4200,-1550,320),FVector(1220,390,20),TEXT("Timber"));
    Shape(TEXT("LangSignBoard"),FVector(-4200,-1710,815),FVector(1040,25,100),TEXT("Timber"));
    Sign(TEXT("LangLettering"),TEXT("LANG'S  /  ROOMS & BOARD"),FVector(-4200,-1690,815),90,36);
    Shape(TEXT("LangCounter"),FVector(-3900,-2200,50),FVector(270,100,100),TEXT("Timber"));
    GuestRegister=Shape(TEXT("LangGuestRegister"),RegisterLocation(),FVector(36,46,4),TEXT("PaperLabel"));
    Sign(TEXT("LangRegisterLabel"),TEXT("GUEST REGISTER"),FVector(-3900,-2145,73),90,13);
    Shape(TEXT("LangBench"),FVector(-4550,-2350,43),FVector(110,230,12),TEXT("Timber"));
    Shape(TEXT("LangBenchBack"),FVector(-4600,-2350,82),FVector(12,230,80),TEXT("Timber"));
    for(int32 X:{-4580,-4520}) for(int32 Y:{-2440,-2260})
        Shape(FString::Printf(TEXT("LangBenchLeg%d%d"),X,Y),FVector(X,Y,19),FVector(10,10,38),TEXT("Timber"));
    for(int32 I=0;I<3;++I) Shape(FString::Printf(TEXT("LangHook%d"),I),FVector(-4430+I*110,-2680,185),FVector(8,20,12),TEXT("Brass"));
    auto* Lamp=CreateDefaultSubobject<UPointLightComponent>(TEXT("LangLobbyLight"));Lamp->SetupAttachment(RootComponent);
    Lamp->SetRelativeLocation(FVector(-4200,-2180,295));Lamp->SetIntensity(2200);Lamp->SetAttenuationRadius(1100);
    Lamp->SetLightColor(FLinearColor(1.f,.82f,.6f));Lamp->SetMobility(EComponentMobility::Movable);
    auto* Fill=CreateDefaultSubobject<UPointLightComponent>(TEXT("LangCounterFill"));Fill->SetupAttachment(RootComponent);
    Fill->SetRelativeLocation(FVector(-3980,-2050,250));Fill->SetIntensity(3500);Fill->SetAttenuationRadius(1000);
    Fill->SetLightColor(FLinearColor(1.f,.88f,.72f));Fill->SetCastShadows(false);Fill->SetMobility(EComponentMobility::Movable);
    Shape(TEXT("LangLightShade"),FVector(-4200,-2180,310),FVector(45,45,20),TEXT("Timber"),TEXT("Cone"),false);
    // Poles and wires echo the concept image while leaving every walking lane clear.
    for(int32 I=0;I<6;++I)
    {
        const FVector P(-1650,-3350+I*1100,0);
        Shape(FString::Printf(TEXT("TownPole%d"),I),P+FVector(0,0,420),FVector(20,20,840),TEXT("Timber"),TEXT("Cylinder"));
        Shape(FString::Printf(TEXT("TownCrossarm%d"),I),P+FVector(0,0,750),FVector(190,16,18),TEXT("Timber"));
        if(I<5) for(int32 Side:{-1,1}) Shape(FString::Printf(TEXT("TownWire%d_%d"),I,Side),P+FVector(Side*70,550,765),FVector(2,1100,2),TEXT("Iron"),TEXT("Cube"),false);
    }
    Sign(TEXT("CourtStreetSign"),TEXT("COURT STREET\n< LANG'S   /   JAIL >"),FVector(-2100,-1100,160),-90,26);
    Shape(TEXT("CourtSignBoard"),FVector(-2100,-1090,160),FVector(490,15,100),TEXT("Timber"));
    Shape(TEXT("CourtSignPost"),FVector(-2100,-1090,75),FVector(12,12,150),TEXT("Timber"));
    const FVector Trees[]={FVector(-2800,-100,0),FVector(-5100,-100,0),FVector(-2700,2400,0),FVector(-5200,2400,0),FVector(-1100,650,0)};
    for(int32 I=0;I<5;++I)
    {
        Shape(FString::Printf(TEXT("SquareTreeTrunk%d"),I),Trees[I]+FVector(0,0,240),FVector(35,35,480),TEXT("Timber"),TEXT("Cylinder"))->SetVisibility(false);
        StreetMesh(FString::Printf(TEXT("SquareCottonwood%d"),I),TEXT("SM_StreetCottonwood"),Trees[I],FRotator(0,I*71,0),FVector(1.f+(I%3)*.12f));
    }
    // Flush, divided walks keep the approved roads and entry approaches clear.
    for(int32 I=0;I<23;++I)
    {
        const float Y=700+I*120;
        Shape(FString::Printf(TEXT("CourtPaving%d"),I),FVector(-1750+(I%3-1)*2,Y,3.f+(I%4)*.12f),FVector(300,115+(I%3),3),TEXT("Plaster"),TEXT("Cube"),false);
        Shape(FString::Printf(TEXT("CourtCurb%d"),I),FVector(-1910,Y,3),FVector(18,118,4),TEXT("Plaster"),TEXT("Cube"),false);
    }
    // The Enterprise fronts the square's southern approach beside Lang's.
    Shape(TEXT("EnterpriseFloor"),FVector(-2650,-2450,-12),FVector(900,1000,24),TEXT("Timber"));
    Shape(TEXT("EnterpriseBack"),FVector(-2650,-2940,175),FVector(900,20,350),TEXT("Brick"));
    for(int32 Side:{-1,1})
    {
        Shape(FString::Printf(TEXT("EnterpriseSide%d"),Side),FVector(-2650+Side*440,-2450,175),FVector(20,1000,350),TEXT("Brick"));
        Shape(FString::Printf(TEXT("EnterpriseFront%d"),Side),FVector(-2650+Side*275,-1950,175),FVector(350,20,350),TEXT("Brick"));
        Shape(FString::Printf(TEXT("EnterpriseInnerSide%d"),Side),FVector(-2650+Side*426,-2450,170),FVector(6,950,340),TEXT("Plaster"),TEXT("Cube"),false);
    }
    Shape(TEXT("EnterpriseInnerBack"),FVector(-2650,-2926,170),FVector(850,6,340),TEXT("Plaster"),TEXT("Cube"),false);
    Shape(TEXT("EnterpriseHeader"),FVector(-2650,-1950,305),FVector(200,20,90),TEXT("Brick"));
    Shape(TEXT("EnterpriseRoof"),FVector(-2650,-2450,365),FVector(960,1060,30),TEXT("Iron"));
    Shape(TEXT("EnterpriseParapet"),FVector(-2650,-1950,420),FVector(900,40,100),TEXT("Brick"));
    Sign(TEXT("EnterpriseSign"),TEXT("THE ENTERPRISE"),FVector(-2650,-1927,420),90,35);
    Shape(TEXT("EnterprisePorch"),FVector(-2650,-1840,-1),FVector(960,220,2),TEXT("Timber"));
    Shape(TEXT("MaraDesk"),FVector(-2650,-2440,47),FVector(360,80,94),TEXT("Timber"));
    Shape(TEXT("MaraDeskTop"),FVector(-2650,-2440,99),FVector(380,94,10),TEXT("Timber"));
    Shape(TEXT("EnterpriseTypewriter"),FVector(-2750,-2440,113),FVector(60,40,20),TEXT("Iron"));
    Shape(TEXT("EnterpriseTypePaper"),FVector(-2750,-2455,137),FVector(25,3,28),TEXT("PaperLabel"),TEXT("Cube"),false);
    Sign(TEXT("MaraDeskName"),TEXT("MARA HOLT / EDITOR"),FVector(-2650,-2390,70),90,14);
    auto* EnterpriseLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("EnterpriseLight"));
    EnterpriseLight->SetupAttachment(RootComponent);EnterpriseLight->SetRelativeLocation(FVector(-2650,-2350,300));
    EnterpriseLight->SetIntensity(1800);EnterpriseLight->SetAttenuationRadius(1000);EnterpriseLight->SetMobility(EComponentMobility::Movable);
    const FVector Mara(-2650,-2550,0);
    MaraCollision=CreateDefaultSubobject<UCapsuleComponent>(TEXT("MaraCollision"));MaraCollision->SetupAttachment(RootComponent);
    MaraCollision->SetRelativeLocation(Mara+FVector(0,0,84));MaraCollision->InitCapsuleSize(28,84);MaraCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    auto MaraFigure=[this,Mara](const TCHAR* Name,FVector Offset,FVector Size,const TCHAR* Material,const TCHAR* Mesh=TEXT("Sphere"))
    {return Shape(Name,Mara+FVector(Offset.X,-Offset.Y,Offset.Z)*1.037f,Size*1.037f,Material,Mesh,false);};
    MaraFigure(TEXT("MaraSkirt"),FVector(0,0,53),FVector(48,37,90),TEXT("Trouser"),TEXT("Cone"));
    MaraFigure(TEXT("MaraBlouse"),FVector(0,0,110),FVector(42,28,48),TEXT("BrownWool"));
    MaraFigure(TEXT("MaraNeck"),FVector(0,0,139),FVector(10,10,13),TEXT("Skin"));
    MaraFigure(TEXT("MaraHead"),FVector(0,0,149),FVector(19,18,25),TEXT("Skin"));
    MaraFigure(TEXT("MaraHair"),FVector(0,3,154),FVector(20,17,17),TEXT("Hair"));
    MaraFigure(TEXT("MaraBun"),FVector(0,12,148),FVector(12,10,12),TEXT("Hair"));
    MaraFigure(TEXT("MaraNose"),FVector(0,-9,148),FVector(4,5,6),TEXT("Skin"));
    MaraFigure(TEXT("MaraMouth"),FVector(0,-8.5,142),FVector(6,1.5,1.5),TEXT("Mouth"));
    for(int32 Side:{-1,1})
    {
        MaraFigure(*FString::Printf(TEXT("MaraEye%d"),Side),FVector(Side*4,-8,152),FVector(3,2,2),TEXT("EyeWhite"));
        MaraFigure(*FString::Printf(TEXT("MaraIris%d"),Side),FVector(Side*4,-9,152),FVector(1.5,1,1.5),TEXT("Iris"));
        MaraFigure(*FString::Printf(TEXT("MaraSleeve%d"),Side),FVector(Side*24,-5,111),FVector(13,15,38),TEXT("BrownWool"));
        MaraFigure(*FString::Printf(TEXT("MaraHand%d"),Side),FVector(Side*24,-12,92),FVector(8,11,13),TEXT("Skin"));
        MaraFigure(*FString::Printf(TEXT("MaraShoe%d"),Side),FVector(Side*10,-4,6),FVector(12,25,12),TEXT("Leather"));
    }

    NewsBoard=Shape(TEXT("EnterpriseNoticeBoard"),FVector(-2360,-1925,150),FVector(185,12,190),TEXT("Timber"));
    Shape(TEXT("EnterpriseNoticePaper"),FVector(-2360,-1917,150),FVector(172,2,176),TEXT("PaperLabel"),TEXT("Cube"),false);
    NewsLettering=CreateDefaultSubobject<UTextRenderComponent>(TEXT("EnterpriseNoticeText"));NewsLettering->SetupAttachment(RootComponent);
    NewsLettering->SetRelativeLocation(FVector(-2360,-1914,155));NewsLettering->SetRelativeRotation(FRotator(0,90,0));
    NewsLettering->SetHorizontalAlignment(EHTA_Center);NewsLettering->SetVerticalAlignment(EVRTA_TextCenter);NewsLettering->SetWorldSize(13);NewsLettering->SetTextRenderColor(FColor(45,36,25));
    NewsLettering->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Town edge: visible rail barriers around the compact authored footprint.
    for(int32 I=0;I<24;++I) for(int32 Side:{-1,1})
    {
        const float X=-7600+I*(8600.f/23.f),Y=Side<0?-4060:7400;
        Shape(FString::Printf(TEXT("TownFencePost%d_%d"),I,Side),FVector(X,Y,70),FVector(12,12,140),TEXT("Timber"));
        if(I<23) for(int32 Rail=0;Rail<2;++Rail) Shape(FString::Printf(TEXT("TownRail%d_%d_%d"),I,Side,Rail),FVector(X+4300.f/23.f,Y,50+Rail*55),FVector(8600.f/23.f,8,10),TEXT("Timber"));
    }
    Shape(TEXT("WestTownWall"),FVector(-7640,1670,90),FVector(25,11500,180),TEXT("Rock"));
    Shape(TEXT("TownEastNorth"),FVector(1060,4475,90),FVector(25,5850,180),TEXT("Rock"));
    Shape(TEXT("TownEastSouth"),FVector(1060,-2800,90),FVector(25,2400,180),TEXT("Rock"));
}

void ACLPecosBend::BeginPlay()
{
    Super::BeginPlay();
    if(UAnimSequence* Idle=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Art/Animations/A_FieldIdle.A_FieldIdle")))
        ResidentMesh->PlayAnimation(Idle,true);
}

void ACLPecosBend::RefreshEnterpriseNotice(const FCLReportState& Report)
{
    const FString Headline=Report.EnterpriseHeadline();
    if(Headline==LastEnterpriseHeadline) return;
    LastEnterpriseHeadline=Headline;
    FString Printed=Headline.Replace(TEXT(" / "),TEXT("\n"));
    Printed=Printed.Replace(TEXT("REED SIGNS REPORT"),TEXT("REED SIGNS\nREPORT"));
    NewsLettering->SetText(FText::FromString(Printed+TEXT("\n\nREAD THE NOTICE")));
}
