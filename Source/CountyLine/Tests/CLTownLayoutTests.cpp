#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Paper/CLCaseState.h"
#include "Paper/CLSaveValidation.h"
#include "World/CLCountyRoad.h"
#include "World/CLPecosBend.h"
#include "World/CLWorldState.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"

// Town layout regressions that need no world: zone classification across the
// western commercial block and its service lane, the location ids and
// checkpoints that existing saves depend on, and the shop frames the map
// generator reads. Geometry comes from the town actor's defaults, so a moved
// shop or a re-cut zone fails here long before a playtest. Saves are in memory
// only: nothing touches the player's CountyLine_JailPrototype slot.
namespace CLTownLayoutTestUtil
{
    constexpr double GroundZ = 92.0;

    // Shops, with the street each one fronts. The smoke test checks a shop's
    // approach is unobstructed; these tests check the frame it was built in.
    struct FShop { const TCHAR* Name; const TCHAR* Street; };
    static const FShop Shops[] = {
        {TEXT("DryGoods"), TEXT("WestMarketStreet")},
        {TEXT("Grocer"), TEXT("WestMarketStreet")},
        {TEXT("ClosedShop"), TEXT("WestMarketStreet")},
        {TEXT("PostOffice"), TEXT("EastShopWalk")},
        {TEXT("Drugs"), TEXT("EastShopWalk")}
    };

    static FName PlaceAt(double X, double Y, double Z = GroundZ) { return ACLCountyRoad::LocationAt(FVector(X, Y, Z)); }

    // Default subobjects are named objects under the class default object, the
    // same lookup the runtime layout checks use.
    static const USceneComponent* Part(const TCHAR* Name)
    {
        ACLPecosBend* Town = const_cast<ACLPecosBend*>(GetDefault<ACLPecosBend>());
        return Town ? FindObjectFast<USceneComponent>(Town, FName(Name)) : nullptr;
    }

    // Components are built but never registered on the class default object, so
    // compose the attachment chain by hand instead of reading world transforms.
    static FTransform Composed(const USceneComponent* Component)
    {
        FTransform Result = FTransform::Identity;
        for (const USceneComponent* Node = Component; Node; Node = Node->GetAttachParent())
            Result = Result * Node->GetRelativeTransform();
        return Result;
    }

    // Every shop part is authored with local front at -Y.
    static FVector FacingOf(const FTransform& Frame) { return Frame.GetRotation().RotateVector(FVector(0, -1, 0)); }

    struct FFootprint
    {
        FVector Centre = FVector::ZeroVector;
        FVector2D Extent = FVector2D::ZeroVector; // World-aligned half sizes.
        bool bValid = false;
    };

    // A Shape() box: relative scale carries its size, because the basic cube is
    // 100 units. Only cardinal rotations are used in town, so the world-aligned
    // extent is the local one, swapped when the box is turned a quarter turn.
    static FFootprint FootprintOf(const TCHAR* Name)
    {
        FFootprint Box;
        const USceneComponent* Component = Part(Name);
        if (!Component) return Box;
        const FTransform Frame = Composed(Component);
        const FVector Size = Component->GetRelativeScale3D() * 100.f;
        const double Yaw = Frame.Rotator().Yaw;
        const bool bTurned = FMath::IsNearlyEqual(FMath::Abs(FMath::UnwindDegrees(Yaw)), 90.0, 1.0);
        Box.Centre = Frame.GetTranslation();
        Box.Extent = bTurned ? FVector2D(Size.Y, Size.X) * .5 : FVector2D(Size.X, Size.Y) * .5;
        Box.bValid = true;
        return Box;
    }

    static bool Contains(const FFootprint& Box, const FVector& Point, double Margin = 0.0)
    {
        return Box.bValid &&
            FMath::Abs(Point.X - Box.Centre.X) <= Box.Extent.X + Margin &&
            FMath::Abs(Point.Y - Box.Centre.Y) <= Box.Extent.Y + Margin;
    }

    static bool Overlaps(const FFootprint& A, const FFootprint& B)
    {
        return A.bValid && B.bValid &&
            FMath::Abs(A.Centre.X - B.Centre.X) < A.Extent.X + B.Extent.X &&
            FMath::Abs(A.Centre.Y - B.Centre.Y) < A.Extent.Y + B.Extent.Y;
    }

    // Samples a straight walk every 10 cm and returns the longest run of samples
    // that belonged to no location. One sample is a zone seam; more is a hole.
    static int32 LongestUnnamedRun(const FVector2D& From, const FVector2D& To)
    {
        const int32 Steps = FMath::Max(1, FMath::CeilToInt((To - From).Size() / 10.0));
        int32 Longest = 0, Run = 0;
        for (int32 Step = 0; Step <= Steps; ++Step)
        {
            const FVector2D Point = From + (To - From) * (static_cast<double>(Step) / Steps);
            if (PlaceAt(Point.X, Point.Y).IsNone()) Longest = FMath::Max(Longest, ++Run);
            else Run = 0;
        }
        return Longest;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLCommercialBlockTest,"CountyLine.World.CommercialBlockZones",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLCommercialBlockTest::RunTest(const FString& Parameters)
{
    using namespace CLTownLayoutTestUtil;

    // The western block spans the junction of the two Court Street rules: the
    // square's band below y=1630 and the commercial and residential band above
    // it. Each street is swept over the ground it actually covers, read from the
    // town's own geometry, so lengthening a street extends this check with it.
    for (const TCHAR* StreetName : {TEXT("WestMarketStreet"), TEXT("WestServiceLane"), TEXT("EastShopWalk")})
    {
        const FFootprint Street = FootprintOf(StreetName);
        if (!TestTrue(FString(StreetName) + TEXT(" exists"), Street.bValid)) continue;
        bool bNamed = true, bRecoverable = true;
        double FirstUnnamed = 0;
        for (double Y = Street.Centre.Y - Street.Extent.Y; Y <= Street.Centre.Y + Street.Extent.Y && bNamed; Y += 50)
        for (const double Side : {-1.0, 0.0, 1.0})
        {
            const double X = Street.Centre.X + Side * (Street.Extent.X - 1);
            const FName Place = PlaceAt(X, Y);
            FTransform Unused;
            if (Place.IsNone()) { bNamed = false; FirstUnnamed = Y; break; }
            bRecoverable &= ACLCountyRoad::SafeCheckpoint(Place, Unused);
        }
        TestTrue(FString::Printf(TEXT("%s is a named place over its whole length (first gap at y=%.0f)"), StreetName, FirstUnnamed), bNamed);
        TestTrue(FString(StreetName) + TEXT(" can be recovered along its whole length"), bRecoverable);
    }
    for (double Y = -900; Y <= 4300; Y += 50)
    {
        if (!TestTrue(FString::Printf(TEXT("Market Street at y=%.0f is Court Street"), Y), PlaceAt(-5650, Y) == FName(TEXT("CourtStreet")))) break;
        if (!TestTrue(FString::Printf(TEXT("Service lane at y=%.0f is Court Street"), Y), PlaceAt(-7250, Y) == FName(TEXT("CourtStreet")))) break;
    }
    TestTrue(TEXT("The y=1630 rule junction is continuous"), PlaceAt(-5650, 1629) == FName(TEXT("CourtStreet")) && PlaceAt(-5650, 1630) == FName(TEXT("CourtStreet")) && PlaceAt(-5650, 1631) == FName(TEXT("CourtStreet")));

    // Shop plots and the ground in front of and behind them are all named, so a
    // customer standing anywhere on the block can be placed and recovered.
    for (const FShop& Shop : Shops)
    {
        const FFootprint Walls = FootprintOf(*(FString(Shop.Name) + TEXT("Walls")));
        if (!TestTrue(FString(Shop.Name) + TEXT(" has walls"), Walls.bValid)) continue;
        const FVector Facing = FacingOf(Composed(Part(*(FString(Shop.Name) + TEXT("Block")))));
        for (const double Along : {-700.0, -350.0, 0.0, 350.0, 700.0, 900.0})
        {
            const FVector Point = Walls.Centre + Facing * Along;
            const FName Place = PlaceAt(Point.X, Point.Y);
            TestFalse(FString::Printf(TEXT("%s at %.0f in front is a named place"), Shop.Name, Along), Place.IsNone());
            FTransform Unused;
            TestTrue(FString::Printf(TEXT("%s at %.0f has a recovery point"), Shop.Name, Along), ACLCountyRoad::SafeCheckpoint(Place, Unused));
        }
    }

    // The block's edges: past the service lane the map ends, and the lane itself
    // stays inside the zone with room to walk behind the shops.
    TestTrue(TEXT("West of the service lane is unnamed"), PlaceAt(-7620, 1750).IsNone() && PlaceAt(-7700, 1750).IsNone());
    TestTrue(TEXT("The service lane's west kerb is named"), PlaceAt(-7400, 1750) == FName(TEXT("CourtStreet")));
    TestTrue(TEXT("The commercial band ends at x=1040"), PlaceAt(1040, 2500).IsNone() && PlaceAt(1039, 2500) == FName(TEXT("CourtStreet")));

    // Walking the block: north along Market Street, back down the service lane,
    // and across to the eastern storefront walk. A hole here would stop
    // discovery and checkpoint updates mid-stride.
    TestEqual(TEXT("Market Street is walkable end to end"), LongestUnnamedRun({-5650, -800}, {-5650, 3600}), 0);
    TestEqual(TEXT("The service lane is walkable end to end"), LongestUnnamedRun({-7250, -800}, {-7250, 3600}), 0);
    TestEqual(TEXT("The lane connects to the square"), LongestUnnamedRun({-7250, -800}, {-2200, -800}), 0);
    TestEqual(TEXT("Market Street connects to the service lane"), LongestUnnamedRun({-5650, 3400}, {-7250, 3400}), 0);
    TestEqual(TEXT("The eastern shop walk is walkable"), LongestUnnamedRun({-1770, 800}, {-1770, 3450}), 0);
    TestEqual(TEXT("The eastern walk joins Court Street"), LongestUnnamedRun({-1770, 3450}, {-2200, 4300}), 0);

    // The whole block shares Court Street's recovery point; no new id appeared.
    FCLWorldState World;
    TestTrue(TEXT("Standing on Market Street discovers Court Street"), World.DiscoverLocation(PlaceAt(-5650, 2300)) && World.IsLocationDiscovered(TEXT("CourtStreet")));
    TestFalse(TEXT("The service lane adds no second discovery"), World.DiscoverLocation(PlaceAt(-7250, 2300)));
    TestEqual(TEXT("The block is one discovery"), World.NumDiscoveredLocations(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLShopFrontageTest,"CountyLine.World.ShopFrontage",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLShopFrontageTest::RunTest(const FString& Parameters)
{
    using namespace CLTownLayoutTestUtil;

    TArray<FFootprint> Footprints;
    for (const FShop& Shop : Shops)
    {
        const FString Name(Shop.Name);
        const USceneComponent* Block = Part(*(Name + TEXT("Block")));
        if (!TestNotNull(*(Name + TEXT(" has a block frame")), Block)) continue;
        const FTransform Frame = Composed(Block);
        const FVector Facing = FacingOf(Frame);
        const FVector Sideways = Frame.GetRotation().RotateVector(FVector(1, 0, 0));

        // Shops are quarter-turned; the map generator refuses anything else.
        const double Yaw = FMath::UnwindDegrees(Frame.Rotator().Yaw);
        TestTrue(*FString::Printf(TEXT("%s stands on a quarter turn (yaw %.1f)"), Shop.Name, Yaw), FMath::IsNearlyZero(FMath::Fmod(FMath::Abs(Yaw), 90.0), 0.01) || FMath::IsNearlyEqual(FMath::Fmod(FMath::Abs(Yaw), 90.0), 90.0, 0.01));
        TestTrue(*FString::Printf(TEXT("%s faces a cardinal direction"), Shop.Name), FMath::IsNearlyEqual(FMath::Abs(Facing.X) + FMath::Abs(Facing.Y), 1.0, 0.01));

        // Door, sign, porch and boardwalk all belong to the front.
        auto Ahead = [&](const TCHAR* Suffix, double& OutAlong, double& OutSideways)
        {
            const USceneComponent* Component = Part(*(Name + Suffix));
            if (!Component) return false;
            const FVector Offset = Composed(Component).GetTranslation() - Frame.GetTranslation();
            OutAlong = FVector::DotProduct(Offset, Facing);
            OutSideways = FVector::DotProduct(Offset, Sideways);
            return true;
        };
        double DoorAlong = 0, DoorSide = 0, WalkAlong = 0, WalkSide = 0, SignAlong = 0, SignSide = 0, PorchAlong = 0, PorchSide = 0;
        if (TestTrue(*(Name + TEXT(" has a door")), Ahead(TEXT("Door"), DoorAlong, DoorSide)))
        {
            TestTrue(*FString::Printf(TEXT("%s door is on the front wall"), Shop.Name), DoorAlong > 300.0);
            TestTrue(*FString::Printf(TEXT("%s door is centred"), Shop.Name), FMath::Abs(DoorSide) < 60.0);
        }
        if (TestTrue(*(Name + TEXT(" has a boardwalk")), Ahead(TEXT("Boardwalk"), WalkAlong, WalkSide)))
            TestTrue(*FString::Printf(TEXT("%s boardwalk lies beyond its door"), Shop.Name), WalkAlong > DoorAlong);
        if (TestTrue(*(Name + TEXT(" has a porch roof")), Ahead(TEXT("PorchRoof"), PorchAlong, PorchSide)))
            TestTrue(*FString::Printf(TEXT("%s porch covers the frontage"), Shop.Name), PorchAlong > DoorAlong);
        if (TestTrue(*(Name + TEXT(" has a sign board")), Ahead(TEXT("SignBoard"), SignAlong, SignSide)))
            TestTrue(*FString::Printf(TEXT("%s sign hangs over the front"), Shop.Name), SignAlong > 300.0);

        // The lettering reads outward from the shop, not into its own wall.
        if (const USceneComponent* Lettering = Part(*(Name + TEXT("Lettering"))))
        {
            const FVector Reads = Composed(Lettering).GetRotation().RotateVector(FVector(1, 0, 0));
            TestTrue(*FString::Printf(TEXT("%s lettering reads toward the street"), Shop.Name), FVector::DotProduct(Reads, Facing) > .99);
        }

        // The street it fronts is actually in front of it.
        const FFootprint Street = FootprintOf(Shop.Street);
        if (TestTrue(*FString::Printf(TEXT("%s street %s exists"), Shop.Name, Shop.Street), Street.bValid))
        {
            const FVector Frontage = Frame.GetTranslation() + Facing * 900.0;
            TestTrue(*FString::Printf(TEXT("%s frontage reaches %s"), Shop.Name, Shop.Street), Contains(Street, Frontage, 150.0));
            const FVector Behind = Frame.GetTranslation() - Facing * 900.0;
            TestFalse(*FString::Printf(TEXT("%s does not back onto %s"), Shop.Name, Shop.Street), Contains(Street, Behind));
        }

        const FFootprint Walls = FootprintOf(*(Name + TEXT("Walls")));
        if (Walls.bValid)
        {
            // A quarter-turned shop is deeper than it is wide in world axes.
            const bool bTurned = FMath::IsNearlyEqual(FMath::Abs(Facing.X), 1.0, 0.01);
            TestTrue(*FString::Printf(TEXT("%s footprint follows its turn"), Shop.Name), bTurned ? Walls.Extent.X < Walls.Extent.Y : Walls.Extent.X > Walls.Extent.Y);
            TestFalse(*FString::Printf(TEXT("%s walls stay off its own street"), Shop.Name), Overlaps(Walls, FootprintOf(Shop.Street)));
            Footprints.Add(Walls);
        }
    }

    // Shops never share ground with each other.
    for (int32 A = 0; A < Footprints.Num(); ++A)
        for (int32 B = A + 1; B < Footprints.Num(); ++B)
            TestFalse(FString::Printf(TEXT("Shops %d and %d do not overlap"), A, B), Overlaps(Footprints[A], Footprints[B]));

    // The west block faces east onto Market Street, the east block faces west
    // onto its walk, so the two ranks look at each other across Court Street.
    const FVector WestFacing = FacingOf(Composed(Part(TEXT("DryGoodsBlock"))));
    const FVector EastFacing = FacingOf(Composed(Part(TEXT("PostOfficeBlock"))));
    TestTrue(TEXT("The west rank faces east"), WestFacing.X > .99);
    TestTrue(TEXT("The east rank faces west"), EastFacing.X < -.99);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLLocationContractTest,"CountyLine.World.LocationContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLLocationContractTest::RunTest(const FString& Parameters)
{
    using namespace CLTownLayoutTestUtil;

    // Saved games store a location id and the checkpoint transform recorded with
    // it. Recovery keeps a saved position only while both still match the
    // authored checkpoint, so renaming an id or nudging a checkpoint silently
    // sends returning players back to the office. These are the values already
    // written into saves; changing one needs a save migration, not an edit.
    struct FContract { const TCHAR* Id; FVector Position; double Yaw; };
    const FContract Contract[] = {
        {TEXT("JailOffice"), FVector(-350, -180, 100), 0},
        {TEXT("CountyRoad"), FVector(4000, -800, 100), 0},
        {TEXT("BendLateral"), FVector(8800, -700, 100), 45},
        {TEXT("CourtStreet"), FVector(-2200, -800, 100), 180},
        {TEXT("LangHouse"), FVector(-4200, -1920, 100), -90}
    };

    for (const FContract& Entry : Contract)
    {
        const FString Ctx(Entry.Id);
        FTransform Checkpoint;
        if (!TestTrue(Ctx + TEXT(" is still a known location"), ACLCountyRoad::SafeCheckpoint(Entry.Id, Checkpoint))) continue;
        TestTrue(Ctx + TEXT(" checkpoint has not moved"), Checkpoint.GetLocation().Equals(Entry.Position, 0.01));
        TestTrue(Ctx + TEXT(" checkpoint still faces the same way"), FMath::IsNearlyEqual(FMath::UnwindDegrees(Checkpoint.Rotator().Yaw), FMath::UnwindDegrees(Entry.Yaw), 0.01));

        // A save written with these values, as an existing player's save was,
        // still loads and still recovers to where they stood.
        FCLWorldState Saved;
        for (const FContract& Known : Contract) Saved.DiscoverLocation(Known.Id);
        TestTrue(Ctx + TEXT(" can be stored from the saved values"), Saved.SetLastSafePosition(Entry.Id, FTransform(FRotator(0, Entry.Yaw, 0), Entry.Position)));
        UCLPrototypeSave* Save = NewObject<UCLPrototypeSave>();
        Save->World = Saved;
        Save->Report.bRead = true;
        TArray<uint8> Bytes;
        if (!TestTrue(Ctx + TEXT(" save serializes"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) continue;
        const UCLPrototypeSave* Loaded = Cast<UCLPrototypeSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
        if (!TestNotNull(*(Ctx + TEXT(" save loads")), Loaded)) continue;
        FCLReportState OutReport; bool bOutTyped = true; FCLWorldState OutWorld;
        TestTrue(Ctx + TEXT(" save passes validation"), CLSaveValidation::CopyIfValid(Loaded, OutReport, bOutTyped, OutWorld));
        FTransform Authored;
        ACLCountyRoad::SafeCheckpoint(OutWorld.LastSafeLocation, Authored);
        TestTrue(Ctx + TEXT(" saved position is still accepted on recovery"), OutWorld.LastSafeTransform.Equals(Authored, .01f));
        for (const FContract& Known : Contract)
            TestTrue(Ctx + TEXT(" save still knows ") + Known.Id, OutWorld.IsLocationDiscovered(Known.Id));
    }

    // Each checkpoint stands in the place it names, and the places stay distinct.
    TSet<FName> Ids;
    for (const FContract& Entry : Contract)
    {
        FTransform Checkpoint;
        ACLCountyRoad::SafeCheckpoint(Entry.Id, Checkpoint);
        TestTrue(FString(Entry.Id) + TEXT(" checkpoint stands in its own place"), ACLCountyRoad::LocationAt(Checkpoint.GetLocation()) == FName(Entry.Id));
        bool bAlready = false;
        Ids.Add(FName(Entry.Id), &bAlready);
        TestFalse(FString(Entry.Id) + TEXT(" is listed once"), bAlready);
    }

    // New districts have so far joined an existing id rather than adding one.
    // If one of these becomes a checkpoint, saves gain a value older builds do
    // not know, so update the save notes with it.
    for (const TCHAR* Proposed : {TEXT("MarketStreet"), TEXT("ServiceLane"), TEXT("NorthLane"), TEXT("Courthouse"), TEXT("PecosBend"), TEXT("DryGoods")})
    {
        FTransform Unused;
        TestFalse(FString::Printf(TEXT("'%s' is not a separate location yet"), Proposed), ACLCountyRoad::SafeCheckpoint(FName(Proposed), Unused));
    }
    return true;
}
#endif
