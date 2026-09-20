#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Paper/CLCaseState.h"
#include "Paper/CLSaveValidation.h"
#include "World/CLWorldState.h"
#include "Kismet/GameplayStatics.h"
#include <limits>

// Memory-only fixtures. Nothing here loads, writes, or deletes the player's slot.
namespace CLWorldStateTestUtil
{
    static FTransform MakeTransform(double X, double Y, double Z, double Yaw = 0.0, double Scale = 1.0)
    {
        FTransform T = FTransform::Identity;
        T.SetTranslation(FVector(X, Y, Z));
        T.SetRotation(FRotator(0.0, Yaw, 0.0).Quaternion());
        T.SetScale3D(FVector(Scale));
        return T;
    }

    // Built component by component so a bad value is never handed to a constructor.
    static FTransform WithTranslation(const FVector& V) { FTransform T = FTransform::Identity; T.SetTranslation(V); return T; }
    static FTransform WithScale(const FVector& V) { FTransform T = FTransform::Identity; T.SetScale3D(V); return T; }
    static FTransform WithRotation(const FQuat& Q) { FTransform T = FTransform::Identity; T.SetRotation(Q); return T; }

    static bool SameTransform(const FTransform& A, const FTransform& B)
    {
        return A.GetTranslation().Equals(B.GetTranslation(), 0.0) &&
            A.GetScale3D().Equals(B.GetScale3D(), 0.0) &&
            A.GetRotation().Equals(B.GetRotation(), 0.0);
    }

    static void ExpectSameWorld(FAutomationTestBase& Test, const FString& Ctx, const FCLWorldState& Actual, const FCLWorldState& Expected)
    {
        Test.TestTrue(Ctx + TEXT(" discovered locations"), Actual.DiscoveredLocations == Expected.DiscoveredLocations);
        Test.TestTrue(Ctx + TEXT(" last safe location"), Actual.LastSafeLocation == Expected.LastSafeLocation);
        Test.TestTrue(Ctx + TEXT(" last safe transform"), SameTransform(Actual.LastSafeTransform, Expected.LastSafeTransform));
    }

    // A world state with both a discovery list and a recorded safe position.
    static FCLWorldState PopulatedWorld()
    {
        FCLWorldState World;
        World.DiscoverLocation(TEXT("JailOffice"));
        World.DiscoverLocation(TEXT("BendLateral"));
        World.DiscoverLocation(TEXT("Courthouse_Annex2"));
        World.SetLastSafePosition(TEXT("BendLateral"), MakeTransform(1234.5, -678.25, 90.125, 47.5, 1.25));
        return World;
    }

    // A report carrying read, carbon, evidence, and filed follow-up data.
    static FCLReportState FollowedUpReport()
    {
        FCLReportState R;
        R.bRead = true;
        R.bBriefedByPruitt = true;
        R.ClosingLine = 1;
        R.FieldNotes = {TEXT("BottleObserved"), TEXT("SalazarStatement")};
        R.Submit(ECLReportStatus::Signed);
        R.Pursue(ECLFollowupLead::Bottle);
        R.CompleteFollowup(2);
        R.FileFollowup(ECLFollowupOutcome::FileSupplement);
        return R;
    }

    static UCLPrototypeSave* MakeSave(const FCLReportState& Report, const FCLWorldState& World, bool bTyped)
    {
        UCLPrototypeSave* Save = NewObject<UCLPrototypeSave>();
        Save->Report = Report; Save->World = World; Save->bTypedCopy = bTyped;
        return Save;
    }

    static UCLPrototypeSave* ThroughMemory(FAutomationTestBase& Test, const FString& Ctx, UCLPrototypeSave* Save)
    {
        TArray<uint8> Bytes;
        if (!Test.TestTrue(Ctx + TEXT(" serializes"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return nullptr;
        UCLPrototypeSave* Loaded = Cast<UCLPrototypeSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
        Test.TestNotNull(Ctx + TEXT(" deserializes"), Loaded);
        return Loaded;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLWorldStateOpsTest,"CountyLine.World.StateOperations",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLWorldStateOpsTest::RunTest(const FString& Parameters)
{
    using namespace CLWorldStateTestUtil;

    // Defaults are the state an older save loads with.
    const FCLWorldState Empty;
    TestEqual(TEXT("No discoveries by default"), Empty.NumDiscoveredLocations(), 0);
    TestFalse(TEXT("No safe position by default"), Empty.HasLastSafePosition());
    TestTrue(TEXT("Default last safe location is unset"), Empty.LastSafeLocation.IsNone());
    TestTrue(TEXT("Default transform is identity"), SameTransform(Empty.LastSafeTransform, FTransform::Identity));
    TestTrue(TEXT("Default world state is valid"), Empty.IsValidState());

    const TCHAR* ValidIds[] = {TEXT("JailOffice"), TEXT("BendLateral"), TEXT("L_JailOffice"), TEXT("Route2Bridge"), TEXT("a"), TEXT("Salazar_Ranch_North_Gate")};
    for (const TCHAR* Id : ValidIds)
        TestTrue(FString::Printf(TEXT("'%s' is a valid id"), Id), FCLWorldState::IsValidLocationId(FName(Id)));
    const FString MaxLength = FString::ChrN(FCLWorldState::MaxLocationIdLength, TEXT('A'));
    TestTrue(TEXT("Longest allowed id is valid"), FCLWorldState::IsValidLocationId(FName(*MaxLength)));

    const TCHAR* InvalidIds[] = {TEXT(""), TEXT(" "), TEXT("Bend Lateral"), TEXT("2ndStreet"), TEXT("_Office"), TEXT("Bend-Lateral"), TEXT("Bend.Lateral"), TEXT("Office!"), TEXT("Bend/Lateral"), TEXT("Bend\tLateral")};
    for (const TCHAR* Id : InvalidIds)
        TestFalse(FString::Printf(TEXT("'%s' is not a valid id"), Id), FCLWorldState::IsValidLocationId(FName(Id)));
    TestFalse(TEXT("NAME_None is not a valid id"), FCLWorldState::IsValidLocationId(NAME_None));
    TestFalse(TEXT("Over-long id is invalid"), FCLWorldState::IsValidLocationId(FName(*FString::ChrN(FCLWorldState::MaxLocationIdLength + 1, TEXT('A')))));

    // Discovery records each id once, keeps insertion order, and rejects the rest.
    FCLWorldState World;
    TestTrue(TEXT("First discovery recorded"), World.DiscoverLocation(TEXT("JailOffice")));
    TestTrue(TEXT("Second discovery recorded"), World.DiscoverLocation(TEXT("BendLateral")));
    TestEqual(TEXT("Two locations discovered"), World.NumDiscoveredLocations(), 2);
    TestTrue(TEXT("Discovery order preserved"), World.DiscoveredLocations[0] == FName(TEXT("JailOffice")) && World.DiscoveredLocations[1] == FName(TEXT("BendLateral")));
    TestTrue(TEXT("Discovered location is queryable"), World.IsLocationDiscovered(TEXT("JailOffice")));
    TestFalse(TEXT("Unknown location is not discovered"), World.IsLocationDiscovered(TEXT("Courthouse")));
    TestFalse(TEXT("NAME_None is never discovered"), World.IsLocationDiscovered(NAME_None));

    FCLWorldState Before = World;
    TestFalse(TEXT("Repeat discovery rejected"), World.DiscoverLocation(TEXT("JailOffice")));
    ExpectSameWorld(*this, TEXT("Repeat discovery"), World, Before);
    // FName comparison ignores case, so ids differing only in case are the same location.
    TestFalse(TEXT("Case variant is a repeat"), World.DiscoverLocation(TEXT("jailoffice")));
    TestTrue(TEXT("Case variant queries as discovered"), World.IsLocationDiscovered(TEXT("JAILOFFICE")));
    ExpectSameWorld(*this, TEXT("Case variant discovery"), World, Before);
    for (const TCHAR* Id : InvalidIds)
    {
        TestFalse(FString::Printf(TEXT("Discovering '%s' rejected"), Id), World.DiscoverLocation(FName(Id)));
        ExpectSameWorld(*this, FString::Printf(TEXT("Rejected discovery '%s'"), Id), World, Before);
    }
    TestFalse(TEXT("Discovering NAME_None rejected"), World.DiscoverLocation(NAME_None));
    ExpectSameWorld(*this, TEXT("Rejected NAME_None discovery"), World, Before);
    TestTrue(TEXT("World stays valid after rejections"), World.IsValidState());

    // Last safe position.
    const FTransform Safe = MakeTransform(120.0, -45.5, 10.25, 33.75, 1.5);
    TestTrue(TEXT("Safe position recorded"), World.SetLastSafePosition(TEXT("JailOffice"), Safe));
    TestTrue(TEXT("Safe position reported"), World.HasLastSafePosition());
    TestTrue(TEXT("Safe location stored"), World.LastSafeLocation == FName(TEXT("JailOffice")));
    TestTrue(TEXT("Safe transform stored"), SameTransform(World.LastSafeTransform, Safe));
    TestEqual(TEXT("Recording a safe position does not discover"), World.NumDiscoveredLocations(), 2);
    TestTrue(TEXT("Undiscovered location may still be safe"), World.SetLastSafePosition(TEXT("Courthouse"), Safe));
    TestFalse(TEXT("Safe position still does not discover"), World.IsLocationDiscovered(TEXT("Courthouse")));
    TestTrue(TEXT("Safe position replaced"), World.SetLastSafePosition(TEXT("JailOffice"), Safe));

    const double Nan = FMath::Sqrt(-1.0);
    const double Inf = std::numeric_limits<double>::infinity();
    TArray<TPair<FString, FTransform>> BadTransforms;
    BadTransforms.Emplace(TEXT("NaN translation"), WithTranslation(FVector(Nan, 0, 0)));
    BadTransforms.Emplace(TEXT("infinite translation"), WithTranslation(FVector(0, Inf, 0)));
    BadTransforms.Emplace(TEXT("negative infinite translation"), WithTranslation(FVector(0, 0, -Inf)));
    BadTransforms.Emplace(TEXT("NaN scale"), WithScale(FVector(1, Nan, 1)));
    BadTransforms.Emplace(TEXT("infinite scale"), WithScale(FVector(Inf, 1, 1)));
    BadTransforms.Emplace(TEXT("NaN rotation"), WithRotation(FQuat(Nan, 0, 0, 1)));
    BadTransforms.Emplace(TEXT("infinite rotation"), WithRotation(FQuat(0, 0, Inf, 1)));
    BadTransforms.Emplace(TEXT("denormalized rotation"), WithRotation(FQuat(0.5, 0.5, 0.5, 0.5) * 2.0));
    BadTransforms.Emplace(TEXT("zero rotation"), WithRotation(FQuat(0, 0, 0, 0)));

    Before = World;
    for (const TPair<FString, FTransform>& Bad : BadTransforms)
    {
        TestFalse(Bad.Key + TEXT(" is not finite"), FCLWorldState::IsFiniteTransform(Bad.Value));
        TestFalse(Bad.Key + TEXT(" rejected"), World.SetLastSafePosition(TEXT("JailOffice"), Bad.Value));
        ExpectSameWorld(*this, Bad.Key + TEXT(" rejected"), World, Before);
    }
    for (const TCHAR* Id : InvalidIds)
    {
        TestFalse(FString::Printf(TEXT("Safe position for '%s' rejected"), Id), World.SetLastSafePosition(FName(Id), Safe));
        ExpectSameWorld(*this, FString::Printf(TEXT("Rejected safe id '%s'"), Id), World, Before);
    }
    TestFalse(TEXT("Safe position for NAME_None rejected"), World.SetLastSafePosition(NAME_None, Safe));
    ExpectSameWorld(*this, TEXT("Rejected NAME_None safe position"), World, Before);
    TestTrue(TEXT("Identity transform accepted"), World.SetLastSafePosition(TEXT("JailOffice"), FTransform::Identity));
    TestTrue(TEXT("World still valid"), World.IsValidState());

    // Reset returns the compatibility default.
    FCLWorldState Cleared = PopulatedWorld();
    Cleared.Reset();
    ExpectSameWorld(*this, TEXT("Reset world"), Cleared, Empty);

    // States the operations cannot produce are rejected by IsValidState.
    FCLWorldState Duplicated;
    Duplicated.DiscoveredLocations = {TEXT("JailOffice"), TEXT("JailOffice")};
    TestFalse(TEXT("Duplicate ids invalid"), Duplicated.IsValidState());
    FCLWorldState BadId;
    BadId.DiscoveredLocations = {TEXT("Bend Lateral")};
    TestFalse(TEXT("Malformed id invalid"), BadId.IsValidState());
    FCLWorldState NoneId;
    NoneId.DiscoveredLocations = {NAME_None};
    TestFalse(TEXT("NAME_None entry invalid"), NoneId.IsValidState());
    FCLWorldState BadSafe;
    BadSafe.LastSafeLocation = TEXT("JailOffice");
    BadSafe.LastSafeTransform = WithTranslation(FVector(Nan, 0, 0));
    TestFalse(TEXT("Non-finite safe transform invalid"), BadSafe.IsValidState());
    BadSafe.LastSafeLocation = TEXT("Bend Lateral");
    BadSafe.LastSafeTransform = FTransform::Identity;
    TestFalse(TEXT("Malformed safe id invalid"), BadSafe.IsValidState());
    FCLWorldState StrayTransform;
    StrayTransform.LastSafeTransform = MakeTransform(5, 5, 5);
    TestFalse(TEXT("Transform without a location invalid"), StrayTransform.IsValidState());
    TestTrue(TEXT("Populated world is valid"), PopulatedWorld().IsValidState());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLWorldStateSaveTest,"CountyLine.World.SavePersistence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLWorldStateSaveTest::RunTest(const FString& Parameters)
{
    using namespace CLWorldStateTestUtil;
    auto SameReport = [](const FCLReportState& A, const FCLReportState& B) { return FCLReportState::StaticStruct()->CompareScriptStruct(&A, &B, 0); };

    // World state rides along with report, evidence, follow-up, and copy preference.
    const FCLReportState Report = FollowedUpReport();
    TestTrue(TEXT("Fixture filed a supplement"), Report.FollowupOutcome == ECLFollowupOutcome::FileSupplement && !Report.SupplementFacts.IsEmpty());
    const FCLWorldState World = PopulatedWorld();
    for (const bool bTyped : {true, false})
    {
        const FString Ctx = bTyped ? TEXT("Typed copy") : TEXT("Handwritten copy");
        UCLPrototypeSave* Loaded = ThroughMemory(*this, Ctx, MakeSave(Report, World, bTyped));
        if (!Loaded) continue;
        FCLReportState OutReport; bool bOutTyped = !bTyped; FCLWorldState OutWorld;
        ECLSaveRejection Reason = ECLSaveRejection::InvalidWorldState;
        TestTrue(Ctx + TEXT(" accepted"), CLSaveValidation::CopyIfValid(Loaded, OutReport, bOutTyped, OutWorld, &Reason));
        TestTrue(Ctx + TEXT(" no rejection"), Reason == ECLSaveRejection::None);
        TestTrue(Ctx + TEXT(" report preserved"), SameReport(OutReport, Report));
        TestTrue(Ctx + TEXT(" copy preference preserved"), bOutTyped == bTyped);
        ExpectSameWorld(*this, Ctx + TEXT(" world"), OutWorld, World);
        TestTrue(Ctx + TEXT(" discoveries queryable after load"), OutWorld.IsLocationDiscovered(TEXT("Courthouse_Annex2")));
        TestTrue(Ctx + TEXT(" loaded world is valid"), OutWorld.IsValidState());
        // Discovery continues from the loaded list.
        TestFalse(Ctx + TEXT(" known location is not rediscovered"), OutWorld.DiscoverLocation(TEXT("JailOffice")));
        TestTrue(Ctx + TEXT(" new location still discoverable"), OutWorld.DiscoverLocation(TEXT("PruittFarm")));
    }

    // A save with no world state, which is what older saves deserialize to.
    {
        UCLPrototypeSave* Untouched = MakeSave(Report, FCLWorldState{}, true);
        UCLPrototypeSave* Loaded = ThroughMemory(*this, TEXT("Save without world state"), Untouched);
        if (Loaded)
        {
            FCLReportState OutReport; bool bOutTyped = false; FCLWorldState OutWorld = PopulatedWorld();
            TestTrue(TEXT("Save without world state accepted"), CLSaveValidation::CopyIfValid(Loaded, OutReport, bOutTyped, OutWorld));
            TestTrue(TEXT("Report still preserved"), SameReport(OutReport, Report));
            ExpectSameWorld(*this, TEXT("Defaulted world"), OutWorld, FCLWorldState{});
            TestEqual(TEXT("Defaulted world has no discoveries"), OutWorld.NumDiscoveredLocations(), 0);
            TestFalse(TEXT("Defaulted world has no safe position"), OutWorld.HasLastSafePosition());
        }
    }

    // The three-argument overload Codex already calls is unchanged and leaves world state alone.
    {
        UCLPrototypeSave* Save = MakeSave(Report, World, false);
        FCLReportState OutReport; bool bOutTyped = true;
        TestTrue(TEXT("Report-only overload accepts"), CLSaveValidation::CopyIfValid(Save, OutReport, bOutTyped));
        TestTrue(TEXT("Report-only overload copies report"), SameReport(OutReport, Report));
        TestFalse(TEXT("Report-only overload copies copy preference"), bOutTyped);
    }

    // Invalid world state is rejected, and nothing is written.
    {
        FCLWorldState Broken = World;
        Broken.DiscoveredLocations.Add(TEXT("Bend Lateral"));
        TArray<FCLWorldState> BadWorlds;
        BadWorlds.Add(Broken);
        FCLWorldState Duplicated = World; Duplicated.DiscoveredLocations.Add(TEXT("JailOffice"));
        BadWorlds.Add(Duplicated);
        FCLWorldState NonFinite = World; NonFinite.LastSafeTransform = WithTranslation(FVector(0, FMath::Sqrt(-1.0), 0));
        BadWorlds.Add(NonFinite);
        FCLWorldState Stray; Stray.LastSafeTransform = MakeTransform(1, 2, 3);
        BadWorlds.Add(Stray);

        for (int32 I = 0; I < BadWorlds.Num(); ++I)
        {
            const FString Ctx = FString::Printf(TEXT("Invalid world %d"), I);
            UCLPrototypeSave* Save = MakeSave(Report, BadWorlds[I], true);
            TestTrue(Ctx + TEXT(" validation reason"), CLSaveValidation::Validate(Save) == ECLSaveRejection::InvalidWorldState);
            TestEqual(Ctx + TEXT(" rejection text"), FString(CLSaveValidation::RejectionText(CLSaveValidation::Validate(Save))), FString(TEXT("invalid world state")));

            const FCLReportState KeptReport = FCLReportState{};
            const FCLWorldState KeptWorld = PopulatedWorld();
            FCLReportState OutReport = KeptReport; FCLWorldState OutWorld = KeptWorld; bool bOutTyped = false;
            ECLSaveRejection Reason = ECLSaveRejection::None;
            TestFalse(Ctx + TEXT(" rejected"), CLSaveValidation::CopyIfValid(Save, OutReport, bOutTyped, OutWorld, &Reason));
            TestTrue(Ctx + TEXT(" reports world-state rejection"), Reason == ECLSaveRejection::InvalidWorldState);
            TestTrue(Ctx + TEXT(" report untouched"), SameReport(OutReport, KeptReport));
            TestFalse(Ctx + TEXT(" copy preference untouched"), bOutTyped);
            ExpectSameWorld(*this, Ctx + TEXT(" world untouched"), OutWorld, KeptWorld);
        }
    }

    // Existing rejection rules still come first, and still write nothing.
    {
        FCLReportState BadLine = Report; BadLine.ClosingLine = 4;
        UCLPrototypeSave* Save = MakeSave(BadLine, World, true);
        FCLWorldState OutWorld; FCLReportState OutReport; bool bOutTyped = true;
        ECLSaveRejection Reason = ECLSaveRejection::None;
        TestFalse(TEXT("Bad closing line still rejected"), CLSaveValidation::CopyIfValid(Save, OutReport, bOutTyped, OutWorld, &Reason));
        TestTrue(TEXT("Closing line reported before world state"), Reason == ECLSaveRejection::InvalidClosingLine);
        ExpectSameWorld(*this, TEXT("World after report rejection"), OutWorld, FCLWorldState{});

        UCLPrototypeSave* OldVersion = MakeSave(Report, World, true);
        OldVersion->Version = 2;
        TestTrue(TEXT("Unsupported version still rejected"), CLSaveValidation::Validate(OldVersion) == ECLSaveRejection::UnsupportedVersion);
        TestFalse(TEXT("Null save still rejected"), CLSaveValidation::CopyIfValid(nullptr, OutReport, bOutTyped, OutWorld));
    }

    // Transform values survive serialization exactly.
    {
        FCLWorldState Precise;
        Precise.DiscoverLocation(TEXT("BendLateral"));
        const FTransform Exact = MakeTransform(-12345.678901, 0.000123456, 7654.321098, 123.456, 0.375);
        TestTrue(TEXT("Precise safe position recorded"), Precise.SetLastSafePosition(TEXT("BendLateral"), Exact));
        UCLPrototypeSave* Loaded = ThroughMemory(*this, TEXT("Precise world"), MakeSave(FCLReportState{}, Precise, true));
        if (Loaded)
        {
            ExpectSameWorld(*this, TEXT("Precise world"), Loaded->World, Precise);
            TestTrue(TEXT("Exact transform survives"), SameTransform(Loaded->World.LastSafeTransform, Exact));
        }
    }
    return true;
}
#endif
