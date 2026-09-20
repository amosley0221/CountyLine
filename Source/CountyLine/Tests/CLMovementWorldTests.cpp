#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Paper/CLCaseState.h"
#include "Paper/CLSaveValidation.h"
#include "Player/CLReedCharacter.h"
#include "World/CLCountyRoad.h"
#include "World/CLWorldState.h"
#include "Animation/BlendSpace.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"

// Pure-logic coverage for walking, jogging, town geography, and checkpoint
// recovery. The runtime smoke test walks the real routes; these suites pin the
// rules underneath it. All save fixtures are in memory: nothing here loads,
// writes, or deletes the player's CountyLine_JailPrototype slot.
namespace CLMovementWorldTestUtil
{
    // Authored walking speeds, mirrored from ACLReedCharacter.
    constexpr float WalkSpeed = 180.f;
    constexpr float JogSpeed = 360.f;
    constexpr double GroundZ = 92.0;

    static const FName Places[] = {TEXT("JailOffice"), TEXT("CourtStreet"), TEXT("LangHouse"), TEXT("CountyRoad"), TEXT("BendLateral")};

    static FName PlaceAt(double X, double Y, double Z = GroundZ) { return ACLCountyRoad::LocationAt(FVector(X, Y, Z)); }

    static bool HasAxisKey(const TCHAR* Axis, FKey Key, float Scale)
    {
        TArray<FInputAxisKeyMapping> Mappings;
        GetDefault<UInputSettings>()->GetAxisMappingByName(FName(Axis), Mappings);
        return Mappings.ContainsByPredicate([Key, Scale](const FInputAxisKeyMapping& M) { return M.Key == Key && M.Scale == Scale; });
    }

    // A walked leg: the places crossed in order, and how much of it had no place
    // at all. LongestGap counts consecutive 10 cm samples that named nowhere, so a
    // zero-width seam between two zones reads as one sample while a real hole in
    // the map reads as many.
    struct FWalk
    {
        TArray<FName> Visited;
        int32 Samples = 0;
        int32 LongestGap = 0;
        FVector2D FirstGapAt = FVector2D::ZeroVector;
    };

    static FWalk WalkSegment(const FVector2D& From, const FVector2D& To)
    {
        FWalk Walk;
        const double Length = (To - From).Size();
        const int32 Steps = FMath::Max(1, FMath::CeilToInt(Length / 10.0));
        int32 Run = 0;
        for (int32 Step = 0; Step <= Steps; ++Step)
        {
            const FVector2D Point = From + (To - From) * (static_cast<double>(Step) / Steps);
            const FName Place = PlaceAt(Point.X, Point.Y);
            ++Walk.Samples;
            if (Place.IsNone())
            {
                if (Run == 0 && Walk.LongestGap == 0) Walk.FirstGapAt = Point;
                Walk.LongestGap = FMath::Max(Walk.LongestGap, ++Run);
            }
            else
            {
                Run = 0;
                if (Walk.Visited.IsEmpty() || Walk.Visited.Last() != Place) Walk.Visited.Add(Place);
            }
        }
        return Walk;
    }

    // Every sampled position must belong to a location, including exact seams.
    // Collision-aware traversal is checked separately by the runtime smoke test.
    static void ExpectWalkable(FAutomationTestBase& Test, const FString& Ctx, const FWalk& Walk)
    {
        Test.TestTrue(FString::Printf(TEXT("%s has no unnamed samples (longest %d, first at %.1f, %.1f)"), *Ctx, Walk.LongestGap, Walk.FirstGapAt.X, Walk.FirstGapAt.Y), Walk.LongestGap == 0);
    }

    static FString Describe(const TArray<FName>& Visited)
    {
        FString Text;
        for (const FName Place : Visited) Text += (Text.IsEmpty() ? TEXT("") : TEXT(" -> ")) + Place.ToString();
        return Text;
    }

    // The recovery rule ACLPlayerController::RestoreSafePosition applies: a saved
    // position is used only when its id is an authored checkpoint and its stored
    // transform still matches that checkpoint, otherwise the office is used.
    static bool WouldKeepSavedPosition(const FCLWorldState& World)
    {
        FTransform Authored;
        return ACLCountyRoad::SafeCheckpoint(World.LastSafeLocation, Authored) && World.LastSafeTransform.Equals(Authored, .01f);
    }

    static FCLReportState FieldReport()
    {
        FCLReportState R;
        R.bRead = true; R.bBriefedByPruitt = true; R.ClosingLine = 1;
        R.FieldNotes = {TEXT("BottleObserved"), TEXT("SalazarStatement")};
        R.Submit(ECLReportStatus::Signed);
        R.Pursue(ECLFollowupLead::Bottle);
        R.CompleteFollowup(2);
        R.FileFollowup(ECLFollowupOutcome::FileSupplement);
        return R;
    }

    static UCLPrototypeSave* ThroughMemory(FAutomationTestBase& Test, const FString& Ctx, const FCLReportState& Report, const FCLWorldState& World, bool bTyped)
    {
        UCLPrototypeSave* Save = NewObject<UCLPrototypeSave>();
        Save->Report = Report; Save->World = World; Save->bTypedCopy = bTyped;
        TArray<uint8> Bytes;
        if (!Test.TestTrue(Ctx + TEXT(" serializes"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return nullptr;
        UCLPrototypeSave* Loaded = Cast<UCLPrototypeSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
        Test.TestNotNull(Ctx + TEXT(" deserializes"), Loaded);
        return Loaded;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLWalkJogControlsTest,"CountyLine.Movement.WalkAndJogControls",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLWalkJogControlsTest::RunTest(const FString& Parameters)
{
    using namespace CLMovementWorldTestUtil;

    // Walking is bound on keyboard and controller, in both directions.
    TestTrue(TEXT("W walks forward"), HasAxisKey(TEXT("MoveForward"), EKeys::W, 1.f));
    TestTrue(TEXT("S walks back"), HasAxisKey(TEXT("MoveForward"), EKeys::S, -1.f));
    TestTrue(TEXT("A walks left"), HasAxisKey(TEXT("MoveRight"), EKeys::A, -1.f));
    TestTrue(TEXT("D walks right"), HasAxisKey(TEXT("MoveRight"), EKeys::D, 1.f));
    TestTrue(TEXT("Left stick walks forward"), HasAxisKey(TEXT("MoveForward"), EKeys::Gamepad_LeftY, 1.f));
    TestTrue(TEXT("Left stick walks sideways"), HasAxisKey(TEXT("MoveRight"), EKeys::Gamepad_LeftX, 1.f));

    // Jogging is hold-to-jog on both hands and on the controller.
    TestTrue(TEXT("Left Shift jogs"), HasAxisKey(TEXT("Jog"), EKeys::LeftShift, 1.f));
    TestTrue(TEXT("Right Shift jogs"), HasAxisKey(TEXT("Jog"), EKeys::RightShift, 1.f));
    TestTrue(TEXT("Left shoulder jogs"), HasAxisKey(TEXT("Jog"), EKeys::Gamepad_LeftShoulder, 1.f));
    TArray<FInputAxisKeyMapping> JogKeys;
    GetDefault<UInputSettings>()->GetAxisMappingByName(TEXT("Jog"), JogKeys);
    TestTrue(TEXT("Jog has at least three keys"), JogKeys.Num() >= 3);
    for (const FInputAxisKeyMapping& Mapping : JogKeys)
    {
        // Jog() only reacts above zero, so a negative mapping would be a dead key.
        TestTrue(FString::Printf(TEXT("Jog key %s has positive scale"), *Mapping.Key.ToString()), Mapping.Scale > 0.f);
        TestFalse(FString::Printf(TEXT("Jog key %s is not a walking key"), *Mapping.Key.ToString()),
            Mapping.Key == EKeys::W || Mapping.Key == EKeys::A || Mapping.Key == EKeys::S || Mapping.Key == EKeys::D ||
            Mapping.Key == EKeys::Gamepad_LeftY || Mapping.Key == EKeys::Gamepad_LeftX);
    }

    // Movement defaults Reed walks with; jogging doubles the speed at runtime.
    const ACLReedCharacter* Defaults = GetDefault<ACLReedCharacter>();
    const UCharacterMovementComponent* Movement = Defaults ? Defaults->GetCharacterMovement() : nullptr;
    if (TestNotNull(TEXT("Reed has a movement component"), Movement))
    {
        TestEqual(TEXT("Default speed is the walking speed"), Movement->MaxWalkSpeed, WalkSpeed);
        TestTrue(TEXT("Reed turns toward movement"), Movement->bOrientRotationToMovement);
        TestTrue(TEXT("Acceleration supports reaching jogging speed within one second"), Movement->MaxAcceleration >= JogSpeed);
        TestTrue(TEXT("Walking stops promptly"), Movement->BrakingDecelerationWalking > 0.f);
        TestTrue(TEXT("Turn rate is finite and positive"), Movement->RotationRate.Yaw > 0.f && FMath::IsFinite(Movement->RotationRate.Yaw));
    }

    // The locomotion blend space has to cover both speeds, or jogging plays a walk.
    const UBlendSpace* Locomotion = LoadObject<UBlendSpace>(nullptr, TEXT("/Game/Art/Animations/BS_Reed_FieldLocomotion.BS_Reed_FieldLocomotion"));
    if (TestNotNull(TEXT("Locomotion blend space loads"), Locomotion))
    {
        const FBlendParameter& Speed = Locomotion->GetBlendParameter(0);
        TestTrue(TEXT("Blend covers standing still"), Speed.Min <= 0.f);
        TestTrue(TEXT("Blend covers walking"), Speed.Max >= WalkSpeed);
        TestTrue(TEXT("Blend covers jogging"), Speed.Max >= JogSpeed);
    }

    // Footstep audio for both authored surfaces.
    TestNotNull(TEXT("Dirt footstep sound loads"), LoadObject<USoundWave>(nullptr, TEXT("/Game/Audio/Field/S_StepDirt.S_StepDirt")));
    TestNotNull(TEXT("Wood footstep sound loads"), LoadObject<USoundWave>(nullptr, TEXT("/Game/Audio/Field/S_StepWood.S_StepWood")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLTownGeographyTest,"CountyLine.World.LocationZones",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLTownGeographyTest::RunTest(const FString& Parameters)
{
    using namespace CLMovementWorldTestUtil;

    // Landmarks from Docs/WORLD_GEOGRAPHY.md resolve to the expected place.
    // These read the authored coordinates; they do not change them.
    TestTrue(TEXT("Jail office at the origin"), PlaceAt(0, 0) == FName(TEXT("JailOffice")));
    TestTrue(TEXT("Court Street spine"), PlaceAt(-2200, -100) == FName(TEXT("CourtStreet")));
    TestTrue(TEXT("Square crossroad is on Court Street"), PlaceAt(-3650, -800) == FName(TEXT("CourtStreet")));
    TestTrue(TEXT("Courthouse stands on the square"), PlaceAt(-3900, 1200) == FName(TEXT("CourtStreet")));
    TestTrue(TEXT("Lang's lobby"), PlaceAt(-4200, -2200) == FName(TEXT("LangHouse")));
    TestTrue(TEXT("Bend Lateral east of town"), PlaceAt(10000, 0) == FName(TEXT("BendLateral")));
    TestTrue(TEXT("County road between town and the field"), PlaceAt(4000, -800) == FName(TEXT("CountyRoad")));

    // Interiors win over the streets that enclose them.
    TestTrue(TEXT("Lang's interior beats Court Street"), PlaceAt(-4200, -1900) == FName(TEXT("LangHouse")));
    TestTrue(TEXT("Jail interior beats the county road"), PlaceAt(-350, -180) == FName(TEXT("JailOffice")));

    // Nowhere places stay unnamed, so nothing is discovered off the map.
    TestTrue(TEXT("Far west is unnamed"), PlaceAt(-20000, 0).IsNone());
    TestTrue(TEXT("Far east is unnamed"), PlaceAt(20000, 0).IsNone());
    TestTrue(TEXT("Far north is unnamed"), PlaceAt(0, 9000).IsNone());
    TestTrue(TEXT("Beyond Bend Lateral is unnamed"), PlaceAt(11800, 0).IsNone());

    // Height gate: underground and rooftop positions are not places.
    TestFalse(TEXT("Floor height is a place"), PlaceAt(0, 0, -20).IsNone());
    TestTrue(TEXT("Below the floor is unnamed"), PlaceAt(0, 0, -21).IsNone());
    TestFalse(TEXT("Upper bound is a place"), PlaceAt(0, 0, 250).IsNone());
    TestTrue(TEXT("Above the roof is unnamed"), PlaceAt(0, 0, 251).IsNone());
    TestTrue(TEXT("Deep fall is unnamed"), PlaceAt(0, 0, -5000).IsNone());

    // The walked routes the prototype relies on, sampled every 10 cm. A gap here
    // means discovery and checkpoint updates silently stop mid-stride.
    struct FRoute { const TCHAR* Name; TArray<FVector2D> Points; };
    const TArray<FRoute> Routes = {
        {TEXT("Jail to Court Street"), {{-350, -180}, {-850, -180}, {-850, -800}, {-2200, -800}}},
        {TEXT("Court Street to Lang's"), {{-2200, -800}, {-3900, -250}, {-3900, -800}, {-4200, -800}, {-4200, -1920}}},
        {TEXT("Lang's back to the jail"), {{-4200, -1920}, {-4200, -800}, {-2200, -800}, {-850, -800}, {-850, -180}, {-350, -180}}},
        {TEXT("Jail to Bend Lateral"), {{-350, -180}, {4000, -800}, {8800, -700}, {10000, 0}}},
        {TEXT("Bend Lateral back to town"), {{10000, 0}, {8800, -700}, {4000, -800}, {-350, -180}, {-850, -800}, {-2200, -800}}}
    };

    TSet<FName> RoutePlaces;
    for (const FRoute& Route : Routes)
    {
        TArray<FName> Visited;
        for (int32 I = 0; I + 1 < Route.Points.Num(); ++I)
        {
            const FWalk Leg = WalkSegment(Route.Points[I], Route.Points[I + 1]);
            ExpectWalkable(*this, Route.Name, Leg);
            for (const FName Place : Leg.Visited)
                if (Visited.IsEmpty() || Visited.Last() != Place) Visited.Add(Place);
        }
        TestTrue(FString(Route.Name) + TEXT(" crosses at least two places: ") + Describe(Visited), Visited.Num() >= 2);
        for (const FName Place : Visited)
        {
            RoutePlaces.Add(Place);
            FTransform Unused;
            TestTrue(FString(Route.Name) + TEXT(" place ") + Place.ToString() + TEXT(" has a checkpoint"), ACLCountyRoad::SafeCheckpoint(Place, Unused));
        }
    }

    // Walking the prototype's routes reaches every named place.
    for (const FName Place : Places)
        TestTrue(TEXT("Routes reach ") + Place.ToString(), RoutePlaces.Contains(Place));

    // Documented transitions, in order, without passing through an unrelated place.
    const FWalk ToLangs = WalkSegment({-4200, -800}, {-4200, -1920});
    ExpectWalkable(*this, TEXT("Court Street to Lang's door"), ToLangs);
    TestTrue(TEXT("Court Street leads into Lang's: ") + Describe(ToLangs.Visited), ToLangs.Visited.Num() == 2 && ToLangs.Visited[0] == FName(TEXT("CourtStreet")) && ToLangs.Visited[1] == FName(TEXT("LangHouse")));
    const FWalk ToField = WalkSegment({4000, -800}, {8800, -700});
    ExpectWalkable(*this, TEXT("County road to Bend Lateral"), ToField);
    TestTrue(TEXT("The road leads into Bend Lateral: ") + Describe(ToField.Visited), ToField.Visited.Num() == 2 && ToField.Visited[0] == FName(TEXT("CountyRoad")) && ToField.Visited[1] == FName(TEXT("BendLateral")));
    const FWalk OutOfJail = WalkSegment({-350, -180}, {-850, -180});
    ExpectWalkable(*this, TEXT("Jail door to the road"), OutOfJail);
    TestTrue(TEXT("Leaving the jail reaches the road: ") + Describe(OutOfJail.Visited), OutOfJail.Visited.Num() == 2 && OutOfJail.Visited[0] == FName(TEXT("JailOffice")) && OutOfJail.Visited[1] == FName(TEXT("CountyRoad")));

    // The residential lane north of the square is part of Court Street for
    // discovery and shares its recovery point, per Docs/WORLD_GEOGRAPHY.md.
    TestTrue(TEXT("North Lane belongs to Court Street"), PlaceAt(-2800, 4300) == FName(TEXT("CourtStreet")));
    TestTrue(TEXT("Residential homes stand on Court Street"), PlaceAt(-5500, 5600) == FName(TEXT("CourtStreet")) && PlaceAt(400, 5600) == FName(TEXT("CourtStreet")));
    TestTrue(TEXT("The lane ends at the northern boundary"), PlaceAt(-2800, 7380).IsNone() && PlaceAt(-2800, 7379) == FName(TEXT("CourtStreet")));
    TestTrue(TEXT("East of the lane is unnamed"), PlaceAt(1040, 5600).IsNone());
    const FWalk ToLane = WalkSegment({-2800, -100}, {-2800, 5600});
    ExpectWalkable(*this, TEXT("Square to the residential lane"), ToLane);
    TestTrue(TEXT("The lane walk stays on Court Street: ") + Describe(ToLane.Visited), ToLane.Visited.Num() == 1 && ToLane.Visited[0] == FName(TEXT("CourtStreet")));
    {
        // The lane uses the existing Court Street checkpoint; no new id was added.
        FCLWorldState Lane;
        FTransform Authored;
        TestTrue(TEXT("Court Street checkpoint serves the lane"), ACLCountyRoad::SafeCheckpoint(PlaceAt(-2800, 4300), Authored));
        Lane.SetLastSafePosition(PlaceAt(-2800, 4300), Authored);
        TestTrue(TEXT("A walk home is recoverable"), WouldKeepSavedPosition(Lane));
    }

    // The county road owns the exact boundary, so discovery never drops out.
    TestTrue(TEXT("County road owns the x=-1520 boundary"), PlaceAt(-1520, -800) == FName(TEXT("CountyRoad")));
    TestTrue(TEXT("A step west of the seam is Court Street"), PlaceAt(-1520.1, -800) == FName(TEXT("CourtStreet")));
    TestTrue(TEXT("A step east of the seam is the county road"), PlaceAt(-1519.9, -800) == FName(TEXT("CountyRoad")));

    // Every place a walk can report is a place that can be discovered and saved.
    for (const FName Place : Places)
    {
        FCLWorldState World;
        TestTrue(Place.ToString() + TEXT(" is a valid location id"), FCLWorldState::IsValidLocationId(Place));
        TestTrue(Place.ToString() + TEXT(" can be discovered"), World.DiscoverLocation(Place));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLCheckpointRecoveryTest,"CountyLine.World.CheckpointRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLCheckpointRecoveryTest::RunTest(const FString& Parameters)
{
    using namespace CLMovementWorldTestUtil;

    // Every place has a checkpoint, and every checkpoint stands in its own place.
    TArray<FVector> Seen;
    for (const FName Place : Places)
    {
        const FString Ctx = Place.ToString();
        FTransform Checkpoint;
        if (!TestTrue(Ctx + TEXT(" has a checkpoint"), ACLCountyRoad::SafeCheckpoint(Place, Checkpoint))) continue;
        TestTrue(Ctx + TEXT(" checkpoint is a finite transform"), FCLWorldState::IsFiniteTransform(Checkpoint));
        TestTrue(Ctx + TEXT(" checkpoint stands in its own place"), ACLCountyRoad::LocationAt(Checkpoint.GetLocation()) == Place);
        TestFalse(Ctx + TEXT(" checkpoint shares no position with another"), Seen.Contains(Checkpoint.GetLocation()));
        Seen.Add(Checkpoint.GetLocation());
        FCLWorldState World;
        TestTrue(Ctx + TEXT(" checkpoint can be stored as a safe position"), World.SetLastSafePosition(Place, Checkpoint));
        TestTrue(Ctx + TEXT(" stored checkpoint is a valid world state"), World.IsValidState());
        TestTrue(Ctx + TEXT(" stored checkpoint is kept on recovery"), WouldKeepSavedPosition(World));
    }

    // Unknown ids are refused, and the caller's transform is left alone.
    const FTransform Sentinel = FTransform(FRotator(0, 123, 0), FVector(77, -88, 99));
    const TCHAR* UnknownIds[] = {TEXT("UnknownLocation"), TEXT("Courthouse"), TEXT("Depot"), TEXT("BendLateral2"), TEXT("Jail Office"), TEXT("")};
    for (const TCHAR* Id : UnknownIds)
    {
        FTransform Out = Sentinel;
        TestFalse(FString::Printf(TEXT("'%s' is not a checkpoint"), Id), ACLCountyRoad::SafeCheckpoint(FName(Id), Out));
        TestTrue(FString::Printf(TEXT("'%s' leaves the transform alone"), Id), Out.Equals(Sentinel, 0.f));
    }
    FTransform NoneOut = Sentinel;
    TestFalse(TEXT("NAME_None is not a checkpoint"), ACLCountyRoad::SafeCheckpoint(NAME_None, NoneOut));
    TestTrue(TEXT("NAME_None leaves the transform alone"), NoneOut.Equals(Sentinel, 0.f));
    // Ids are FNames, so checkpoint lookup ignores case, like discovery does.
    FTransform CaseOut;
    TestTrue(TEXT("Checkpoint lookup ignores case"), ACLCountyRoad::SafeCheckpoint(TEXT("jailoffice"), CaseOut));

    // The recovery decision: keep the saved position only when it is an authored
    // checkpoint that still matches. Everything else falls back to the office.
    FTransform Office;
    TestTrue(TEXT("The office checkpoint exists"), ACLCountyRoad::SafeCheckpoint(TEXT("JailOffice"), Office));
    for (const FName Place : Places)
    {
        const FString Ctx = Place.ToString();
        FTransform Authored;
        if (!ACLCountyRoad::SafeCheckpoint(Place, Authored)) continue;

        FCLWorldState Exact;
        Exact.DiscoverLocation(Place);
        Exact.SetLastSafePosition(Place, Authored);
        TestTrue(Ctx + TEXT(" exact checkpoint is kept"), WouldKeepSavedPosition(Exact));

        // Rounding from serialization or a slightly different floor must not evict it.
        FCLWorldState Nudged = Exact;
        FTransform Near = Authored;
        Near.SetTranslation(Authored.GetTranslation() + FVector(0.004, -0.004, 0.004));
        TestTrue(Ctx + TEXT(" tiny drift is still kept"), Nudged.SetLastSafePosition(Place, Near) && WouldKeepSavedPosition(Nudged));

        // Stale coordinates: the id is known, but the position is no longer the checkpoint.
        const TArray<FVector> StaleOffsets = {FVector(0, 0, -5092), FVector(500, 0, 0), FVector(0, -2000, 0), FVector(0, 0, 3)};
        for (const FVector& Offset : StaleOffsets)
        {
            FCLWorldState Stale = Exact;
            FTransform Moved = Authored;
            Moved.SetTranslation(Authored.GetTranslation() + Offset);
            TestTrue(Ctx + TEXT(" stale position stored"), Stale.SetLastSafePosition(Place, Moved));
            TestFalse(Ctx + TEXT(" stale position falls back"), WouldKeepSavedPosition(Stale));
            // A stale checkpoint is still a structurally valid save: recovery, not
            // validation, is what rejects it.
            TestTrue(Ctx + TEXT(" stale position still validates"), Stale.IsValidState());
            TestTrue(Ctx + TEXT(" discoveries survive a stale checkpoint"), Stale.IsLocationDiscovered(Place));
        }

        // A rotated checkpoint is stale too: recovery restores the authored facing.
        FCLWorldState Turned = Exact;
        FTransform Spun = Authored;
        Spun.SetRotation(FRotator(0, Authored.Rotator().Yaw + 90.f, 0).Quaternion());
        TestTrue(Ctx + TEXT(" turned position stored"), Turned.SetLastSafePosition(Place, Spun));
        TestFalse(Ctx + TEXT(" turned position falls back"), WouldKeepSavedPosition(Turned));
    }

    // Unknown and unset saved locations fall back.
    for (const TCHAR* Id : UnknownIds)
    {
        FCLWorldState World;
        if (!FCLWorldState::IsValidLocationId(FName(Id))) continue;
        World.SetLastSafePosition(FName(Id), Office);
        TestFalse(FString::Printf(TEXT("Saved '%s' falls back"), Id), WouldKeepSavedPosition(World));
        TestTrue(FString::Printf(TEXT("Saved '%s' is still a valid state"), Id), World.IsValidState());
    }
    const FCLWorldState Fresh;
    TestFalse(TEXT("A save with no safe position falls back"), WouldKeepSavedPosition(Fresh));
    TestFalse(TEXT("A fresh world has no safe position"), Fresh.HasLastSafePosition());

    // Report and world state both survive a save, for every place, and the
    // restored checkpoint is still accepted after serialization.
    const FCLReportState Report = FieldReport();
    for (const FName Place : Places)
    for (const bool bTyped : {true, false})
    {
        const FString Ctx = Place.ToString() + (bTyped ? TEXT(" [typed]") : TEXT(" [handwritten]"));
        FTransform Authored;
        if (!ACLCountyRoad::SafeCheckpoint(Place, Authored)) continue;
        FCLWorldState World;
        for (const FName Discovered : Places) World.DiscoverLocation(Discovered);
        World.SetLastSafePosition(Place, Authored);

        UCLPrototypeSave* Loaded = ThroughMemory(*this, Ctx, Report, World, bTyped);
        if (!Loaded) continue;
        FCLReportState OutReport; bool bOutTyped = !bTyped; FCLWorldState OutWorld;
        TestTrue(Ctx + TEXT(" loads"), CLSaveValidation::CopyIfValid(Loaded, OutReport, bOutTyped, OutWorld));
        TestTrue(Ctx + TEXT(" report preserved"), FCLReportState::StaticStruct()->CompareScriptStruct(&OutReport, &Report, 0));
        TestTrue(Ctx + TEXT(" follow-up record preserved"), OutReport.FollowupOutcome == ECLFollowupOutcome::FileSupplement && OutReport.SupplementFacts == Report.SupplementFacts);
        TestTrue(Ctx + TEXT(" copy preference preserved"), bOutTyped == bTyped);
        TestTrue(Ctx + TEXT(" discoveries preserved"), OutWorld.DiscoveredLocations == World.DiscoveredLocations);
        TestTrue(Ctx + TEXT(" safe location preserved"), OutWorld.LastSafeLocation == Place);
        TestTrue(Ctx + TEXT(" checkpoint survives serialization exactly"), OutWorld.LastSafeTransform.Equals(Authored, 0.f));
        TestTrue(Ctx + TEXT(" loaded checkpoint is kept on recovery"), WouldKeepSavedPosition(OutWorld));
        for (const FName Discovered : Places)
            TestTrue(Ctx + TEXT(" still knows ") + Discovered.ToString(), OutWorld.IsLocationDiscovered(Discovered));
    }

    // A saved stale checkpoint survives a round trip and still falls back, with
    // the report and the discovery list untouched.
    {
        FTransform Authored;
        ACLCountyRoad::SafeCheckpoint(TEXT("BendLateral"), Authored);
        FCLWorldState World;
        World.DiscoverLocation(TEXT("JailOffice"));
        World.DiscoverLocation(TEXT("BendLateral"));
        FTransform Stale = Authored;
        Stale.SetTranslation(FVector(0, 0, -5000));
        World.SetLastSafePosition(TEXT("BendLateral"), Stale);
        UCLPrototypeSave* Loaded = ThroughMemory(*this, TEXT("Stale field checkpoint"), Report, World, true);
        if (Loaded)
        {
            FCLReportState OutReport; bool bOutTyped = false; FCLWorldState OutWorld;
            TestTrue(TEXT("Stale save loads"), CLSaveValidation::CopyIfValid(Loaded, OutReport, bOutTyped, OutWorld));
            TestFalse(TEXT("Stale save falls back on recovery"), WouldKeepSavedPosition(OutWorld));
            TestTrue(TEXT("Stale save keeps its discoveries"), OutWorld.IsLocationDiscovered(TEXT("BendLateral")) && OutWorld.IsLocationDiscovered(TEXT("JailOffice")));
            TestTrue(TEXT("Stale save keeps the report"), FCLReportState::StaticStruct()->CompareScriptStruct(&OutReport, &Report, 0));
            // Recovery would rewrite the safe position to the office; the discovery
            // list and report are not part of that decision.
            TestTrue(TEXT("Office checkpoint replaces the stale one"), OutWorld.SetLastSafePosition(TEXT("JailOffice"), Office));
            TestTrue(TEXT("Rewritten position is kept"), WouldKeepSavedPosition(OutWorld));
            TestTrue(TEXT("Rewriting keeps Bend Lateral discovered"), OutWorld.IsLocationDiscovered(TEXT("BendLateral")));
        }
    }
    return true;
}
#endif
