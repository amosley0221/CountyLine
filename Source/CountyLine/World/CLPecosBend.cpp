#include "World/CLPecosBend.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

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
    if(!Finish.IsEmpty())
        if(auto* TownMaterial=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Art/Town/Materials/M_Town%s.M_Town%s"),*Finish,*Finish))) C->SetMaterial(0,TownMaterial);
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
    Shape(Name+TEXT("Walls"),P+FVector(0,0,Height/2),FVector(Width,700,Height),TEXT("Brick"));
    Shape(Name+TEXT("Cornice"),P+FVector(0,0,Height),FVector(Width+35,735,35),TEXT("Plaster"));
    Shape(Name+TEXT("Parapet"),P+FVector(0,-350,Height+65),FVector(Width,35,120),TEXT("Brick"));
    Shape(Name+TEXT("Door"),P+FVector(0,-355,112),FVector(100,12,224),TEXT("Timber"));
    for(int32 Side:{-1,1})
    {
        Shape(Name+FString::Printf(TEXT("Frame%d"),Side),P+FVector(Side*Width*.28f,-360,160),FVector(Width*.28f,18,180),TEXT("Plaster"));
        Shape(Name+FString::Printf(TEXT("Glass%d"),Side),P+FVector(Side*Width*.28f,-372,160),FVector(Width*.28f-20,6,160),TEXT("Iron"));
        Shape(Name+FString::Printf(TEXT("Mullion%d"),Side),P+FVector(Side*Width*.28f,-377,160),FVector(6,6,160),TEXT("Timber"),TEXT("Cube"),false);
    }
    Shape(Name+TEXT("SignBoard"),P+FVector(0,-370,Height-65),FVector(Width-60,15,78),TEXT("Timber"));
    Sign(Name+TEXT("Lettering"),Title,P+FVector(0,-380,Height-65),-90,32);
    Shape(Name+TEXT("PorchRoof"),P+FVector(0,-470,275),FVector(Width+30,300,15),TEXT("Timber"));
    for(int32 Side:{-1,1}) Shape(Name+FString::Printf(TEXT("PorchPost%d"),Side),P+FVector(Side*(Width/2-20),-600,135),FVector(12,12,270),TEXT("Timber"));
    Shape(Name+TEXT("Boardwalk"),P+FVector(0,-490,2),FVector(Width+30,340,8),TEXT("Timber"));
    for(int32 Side:{-1,1})
    {
        Shape(Name+FString::Printf(TEXT("Pilaster%d"),Side),P+FVector(Side*(Width/2-22),-368,Height/2),FVector(45,35,Height),TEXT("Plaster"),TEXT("Cube"),false);
        Shape(Name+FString::Printf(TEXT("Sill%d"),Side),P+FVector(Side*Width*.28f,-380,65),FVector(Width*.28f+20,40,15),TEXT("Plaster"),TEXT("Cube"),false);
    }
    Shape(Name+TEXT("DoorTransom"),P+FVector(0,-375,245),FVector(100,10,32),TEXT("Iron"),TEXT("Cube"),false);
    Shape(Name+TEXT("DoorHandle"),P+FVector(34,-374,112),FVector(5,8,24),TEXT("Brass"),TEXT("Cube"),false);
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
    Sign(TEXT("ResidentialSign"),TEXT("HOMES  /  NORTH LANE"),FVector(-1300,3750,180),-90,23);
    Shape(TEXT("ResidentialSignBoard"),FVector(-1300,3760,180),FVector(430,15,60),TEXT("Timber"));
    Shape(TEXT("ResidentialSignPost"),FVector(-1300,3760,85),FVector(12,12,170),TEXT("Timber"));
    Shape(TEXT("CourtStreet"),FVector(-2200,-100,.2),FVector(620,7700,2),TEXT("RoadDust"),TEXT("Cube"),false);
    Shape(TEXT("SquareRoad"),FVector(-4100,-800,.2),FVector(7000,600,2),TEXT("RoadDust"),TEXT("Cube"),false);
    Shape(TEXT("CourthouseWalk"),FVector(-3900,-100,1),FVector(2400,760,4),TEXT("RoadDust"),TEXT("Cube"),false);
    // Reference landmark: brick county courthouse, two storeys and a cupola.
    const FVector Court(-3900,1200,0);
    Shape(TEXT("CourthouseMass"),Court+FVector(0,0,510),FVector(1800,1400,1020),TEXT("Brick"));
    Shape(TEXT("CourthouseBase"),Court+FVector(0,0,50),FVector(1860,1460,100),TEXT("Plaster"));
    Shape(TEXT("CourthouseCornice"),Court+FVector(0,0,1020),FVector(1900,1500,60),TEXT("Plaster"));
    Shape(TEXT("CourthouseRoof"),Court+FVector(0,0,1070),FVector(1760,1370,60),TEXT("Iron"));
    Shape(TEXT("CourtCentralBay"),Court+FVector(0,-735,515),FVector(470,100,1030),TEXT("Brick"));
    Shape(TEXT("CourtEntryFrame"),Court+FVector(0,-800,175),FVector(250,30,350),TEXT("Plaster"));
    Shape(TEXT("CourtEntryDoor"),Court+FVector(0,-820,155),FVector(195,16,310),TEXT("Timber"));
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
        Shape(FString::Printf(TEXT("SquareSeat%d_%d"),Side,End),Seat+FVector(0,0,46),FVector(190,60,12),TEXT("Timber"));
        Shape(FString::Printf(TEXT("SquareSeatBack%d_%d"),Side,End),Seat+FVector(0,-End*26,82),FVector(190,10,70),TEXT("Timber"));
        for(int32 Leg:{-1,1}) Shape(FString::Printf(TEXT("SquareSeatLeg%d_%d_%d"),Side,End,Leg),Seat+FVector(Leg*65,0,20),FVector(12,45,40),TEXT("Iron"));
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
        Shape(FString::Printf(TEXT("SquareTreeTrunk%d"),I),Trees[I]+FVector(0,0,240),FVector(35,35,480),TEXT("Timber"),TEXT("Cylinder"));
        for(int32 J=0;J<4;++J)
            Shape(FString::Printf(TEXT("SquareTreeCrown%d_%d"),I,J),Trees[I]+FVector((J%2)*130-65,(J/2)*130-65,450+(J%2)*65),FVector(250,250,300),J%2?TEXT("LeafLight"):TEXT("Leaf"),TEXT("Sphere"),false);
    }
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
