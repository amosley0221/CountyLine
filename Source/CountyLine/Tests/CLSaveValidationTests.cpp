#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Paper/CLCaseState.h"
#include "Paper/CLSaveValidation.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

// All fixtures live in memory. Nothing here loads, writes, or deletes the
// CountyLine_JailPrototype slot.
namespace CLSaveValidationTestUtil
{
    static const ECLReportStatus ValidStatuses[] = {ECLReportStatus::Draft, ECLReportStatus::Signed, ECLReportStatus::Held};

    static UCLPrototypeSave* MakeSave(const FCLReportState& Report, bool bTyped = true, int32 Version = CLSaveValidation::SupportedVersion)
    {
        UCLPrototypeSave* Save = NewObject<UCLPrototypeSave>();
        Save->Version = Version; Save->Report = Report; Save->bTypedCopy = bTyped;
        return Save;
    }

    // Serializes and deserializes, so validation sees what a slot load would produce.
    static UCLPrototypeSave* ThroughMemory(FAutomationTestBase& Test, const FString& Ctx, UCLPrototypeSave* Save)
    {
        TArray<uint8> Bytes;
        if (!Test.TestTrue(Ctx + TEXT(" serializes"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return nullptr;
        UCLPrototypeSave* Loaded = Cast<UCLPrototypeSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
        Test.TestNotNull(Ctx + TEXT(" deserializes"), Loaded);
        return Loaded;
    }

    // A submitted report with every current field moved off its default value.
    static FCLReportState FullReport(ECLReportStatus Status)
    {
        FCLReportState R;
        R.bRead = true;
        R.bBriefedByPruitt = true;
        R.bIncludeFinder = false;
        R.bIncludeBottle = true;
        R.ClosingLine = 2;
        R.FieldNotes = {TEXT("SalazarStatement"), TEXT("BottleObserved"), TEXT("BankExamined")};
        R.bIncludeFieldNotes = false;
        if (Status != ECLReportStatus::Draft) R.Submit(Status);
        else
        {
            R.IncludedFacts = {TEXT("DraftIncluded")}; R.OmittedFacts = {TEXT("DraftOmitted")};
            R.CourthouseDelta = 5; R.CarbonClosingLine = TEXT("Draft carbon text");
        }
        // Selection flag changed after submission so it too is off default; validation
        // does not judge whether selections match the carbon.
        R.bIncludeBottle = false;
        // Follow-up data, built through the state's own rules so it stays a state the
        // game can actually reach. A draft cannot file, so it carries a completed fact
        // and a second pending lead instead; a filed report has no pending lead.
        R.Pursue(ECLFollowupLead::Bottle);
        R.CompleteFollowup(2);
        if (Status == ECLReportStatus::Draft) R.Pursue(ECLFollowupLead::Bank);
        else R.FileFollowup(ECLFollowupOutcome::FileSupplement);
        return R;
    }

    // Distinctive destination state, so an accidental write is visible.
    static FCLReportState Sentinel()
    {
        FCLReportState R;
        R.Status = ECLReportStatus::Signed; R.bRead = true; R.bBriefedByPruitt = true;
        R.bIncludeFinder = false; R.bIncludeBottle = false; R.ClosingLine = 1;
        R.IncludedFacts = {TEXT("SentinelIncluded")}; R.OmittedFacts = {TEXT("SentinelOmitted")};
        R.CourthouseDelta = 99; R.FieldNotes = {TEXT("SentinelNote")}; R.bIncludeFieldNotes = false;
        R.CarbonClosingLine = TEXT("Sentinel carbon");
        return R;
    }

    // Reflection-driven, so fields added to FCLReportState later are compared automatically
    // and a failure names the field.
    static void ExpectSameReport(FAutomationTestBase& Test, const FString& Ctx, const FCLReportState& Actual, const FCLReportState& Expected)
    {
        for (TFieldIterator<FProperty> It(FCLReportState::StaticStruct()); It; ++It)
            Test.TestTrue(FString::Printf(TEXT("%s field %s"), *Ctx, *It->GetName()), It->Identical_InContainer(&Actual, &Expected));
        Test.TestTrue(Ctx + TEXT(" whole struct"), FCLReportState::StaticStruct()->CompareScriptStruct(&Actual, &Expected, 0));
    }

    static void ExpectRejected(FAutomationTestBase& Test, const FString& Ctx, const USaveGame* Save, ECLSaveRejection Expected)
    {
        Test.TestEqual(Ctx + TEXT(" validation reason"), FString(CLSaveValidation::RejectionText(CLSaveValidation::Validate(Save))), FString(CLSaveValidation::RejectionText(Expected)));
        for (const bool bDestTyped : {true, false})
        {
            const FCLReportState Before = Sentinel();
            FCLReportState Report = Before;
            bool bTyped = bDestTyped;
            ECLSaveRejection Reason = ECLSaveRejection::None;
            const FString C = Ctx + (bDestTyped ? TEXT(" [dest typed]") : TEXT(" [dest handwritten]"));
            Test.TestFalse(C + TEXT(" copy rejected"), CLSaveValidation::CopyIfValid(Save, Report, bTyped, &Reason));
            Test.TestTrue(C + TEXT(" copy reports reason"), Reason == Expected);
            ExpectSameReport(Test, C + TEXT(" destination unchanged"), Report, Before);
            Test.TestTrue(C + TEXT(" copy preference unchanged"), bTyped == bDestTyped);
        }
    }

    static void ExpectAccepted(FAutomationTestBase& Test, const FString& Ctx, const UCLPrototypeSave* Save)
    {
        Test.TestTrue(Ctx + TEXT(" validates"), CLSaveValidation::Validate(Save) == ECLSaveRejection::None);
        FCLReportState Report = Sentinel();
        bool bTyped = !Save->bTypedCopy;
        ECLSaveRejection Reason = ECLSaveRejection::InvalidStatus;
        Test.TestTrue(Ctx + TEXT(" copies"), CLSaveValidation::CopyIfValid(Save, Report, bTyped, &Reason));
        Test.TestTrue(Ctx + TEXT(" copy reports no rejection"), Reason == ECLSaveRejection::None);
        ExpectSameReport(Test, Ctx + TEXT(" copied report"), Report, Save->Report);
        Test.TestTrue(Ctx + TEXT(" copy preference copied"), bTyped == Save->bTypedCopy);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLSaveValidationRejectTest,"CountyLine.Save.Validation.Rejections",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLSaveValidationRejectTest::RunTest(const FString& Parameters)
{
    using namespace CLSaveValidationTestUtil;

    ExpectRejected(*this, TEXT("Null save"), nullptr, ECLSaveRejection::NullSave);
    TestFalse(TEXT("Null save without reason out-param"), [] { FCLReportState R; bool T = true; return CLSaveValidation::CopyIfValid(nullptr, R, T); }());

    for (const int32 Version : {0, 2, -1, MAX_int32, MIN_int32})
    {
        const FString Ctx = FString::Printf(TEXT("Version %d"), Version);
        UCLPrototypeSave* Save = MakeSave(FullReport(ECLReportStatus::Held), false, Version);
        ExpectRejected(*this, Ctx, Save, ECLSaveRejection::UnsupportedVersion);
        if (UCLPrototypeSave* Loaded = ThroughMemory(*this, Ctx, Save))
            ExpectRejected(*this, Ctx + TEXT(" after memory round trip"), Loaded, ECLSaveRejection::UnsupportedVersion);
    }

    for (const int32 Line : {-1, 3, 4, MAX_int32, MIN_int32})
    for (const ECLReportStatus Status : ValidStatuses)
    {
        const FString Ctx = FString::Printf(TEXT("Closing line %d, %s"), Line, *UCLCaseState::StatusText(Status));
        FCLReportState R = FullReport(Status);
        R.ClosingLine = Line;
        UCLPrototypeSave* Save = MakeSave(R);
        ExpectRejected(*this, Ctx, Save, ECLSaveRejection::InvalidClosingLine);
        if (UCLPrototypeSave* Loaded = ThroughMemory(*this, Ctx, Save))
            ExpectRejected(*this, Ctx + TEXT(" after memory round trip"), Loaded, ECLSaveRejection::InvalidClosingLine);
    }

    for (const uint8 Raw : {uint8(3), uint8(4), uint8(128), uint8(255)})
    {
        const FString Ctx = FString::Printf(TEXT("Status value %d"), int32(Raw));
        FCLReportState R = FullReport(ECLReportStatus::Signed);
        R.Status = static_cast<ECLReportStatus>(Raw);
        ExpectRejected(*this, Ctx, MakeSave(R), ECLSaveRejection::InvalidStatus);
    }

    // Rules are checked in the order Initialize has always used; the first failure is reported.
    FCLReportState Bad = FullReport(ECLReportStatus::Held);
    Bad.ClosingLine = 7; Bad.Status = static_cast<ECLReportStatus>(9);
    ExpectRejected(*this, TEXT("Bad version, line and status"), MakeSave(Bad, true, 2), ECLSaveRejection::UnsupportedVersion);
    ExpectRejected(*this, TEXT("Bad line and status"), MakeSave(Bad), ECLSaveRejection::InvalidClosingLine);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLSaveValidationAcceptTest,"CountyLine.Save.Validation.Acceptance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLSaveValidationAcceptTest::RunTest(const FString& Parameters)
{
    using namespace CLSaveValidationTestUtil;

    ExpectAccepted(*this, TEXT("Default save"), MakeSave(FCLReportState{}));

    // Drafts and submissions, every closing line, both copy preferences, direct and deserialized.
    for (const ECLReportStatus Status : ValidStatuses)
    for (int32 Line = 0; Line < 3; ++Line)
    for (const bool bTyped : {true, false})
    {
        const FString Ctx = FString::Printf(TEXT("%s line %d %s"), *UCLCaseState::StatusText(Status), Line, bTyped ? TEXT("typed") : TEXT("handwritten"));
        FCLReportState R;
        R.bRead = true; R.ClosingLine = Line; R.bIncludeBottle = Line != 1; R.bBriefedByPruitt = Line != 2;
        if (Status != ECLReportStatus::Draft) TestTrue(Ctx + TEXT(" fixture submits"), R.Submit(Status));
        UCLPrototypeSave* Save = MakeSave(R, bTyped);
        ExpectAccepted(*this, Ctx, Save);
        if (UCLPrototypeSave* Loaded = ThroughMemory(*this, Ctx, Save))
            ExpectAccepted(*this, Ctx + TEXT(" after memory round trip"), Loaded);
    }

    // No narrative rules: states the book would not produce are still accepted, as before.
    FCLReportState Odd;
    Odd.Status = ECLReportStatus::Signed; Odd.bRead = false;
    ExpectAccepted(*this, TEXT("Signed but unread, no carbon"), MakeSave(Odd));
    Odd.Status = ECLReportStatus::Draft; Odd.IncludedFacts = {TEXT("Unexpected")}; Odd.CourthouseDelta = -40;
    ExpectAccepted(*this, TEXT("Draft with carbon data"), MakeSave(Odd, false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLSaveValidationCopyTest,"CountyLine.Save.Validation.CompleteCopy",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLSaveValidationCopyTest::RunTest(const FString& Parameters)
{
    using namespace CLSaveValidationTestUtil;

    // Guard for the fixture itself: every reflected field should differ from default,
    // otherwise a copy that dropped that field could not be detected.
    const FCLReportState Defaults;
    for (const ECLReportStatus Status : ValidStatuses)
    {
        const FCLReportState Full = FullReport(Status);
        for (TFieldIterator<FProperty> It(FCLReportState::StaticStruct()); It; ++It)
        {
            if (It->Identical_InContainer(&Full, &Defaults))
            {
                // Fields that cannot be set in a valid state for this status: a draft
                // cannot file a follow-up, and a filed report has no pending lead.
                const FName Field = It->GetFName();
                const bool bUnreachable = Status == ECLReportStatus::Draft
                    ? Field == GET_MEMBER_NAME_CHECKED(FCLReportState, Status) ||
                      Field == GET_MEMBER_NAME_CHECKED(FCLReportState, FollowupOutcome) ||
                      Field == GET_MEMBER_NAME_CHECKED(FCLReportState, SupplementFacts)
                    : Field == GET_MEMBER_NAME_CHECKED(FCLReportState, FollowupLead);
                if (bUnreachable) continue;
                AddWarning(FString::Printf(TEXT("FullReport(%s) leaves field %s at its default; add it to the fixture"), *UCLCaseState::StatusText(Status), *It->GetName()));
            }
        }
    }

    for (const ECLReportStatus Status : ValidStatuses)
    for (const bool bTyped : {true, false})
    {
        const FString Ctx = FString::Printf(TEXT("Full %s %s"), *UCLCaseState::StatusText(Status), bTyped ? TEXT("typed") : TEXT("handwritten"));
        UCLPrototypeSave* Save = MakeSave(FullReport(Status), bTyped);
        TestTrue(Ctx + TEXT(" keeps Pruitt introduction in fixture"), Save->Report.bBriefedByPruitt);
        UCLPrototypeSave* Loaded = ThroughMemory(*this, Ctx, Save);
        if (!Loaded) continue;
        ExpectSameReport(*this, Ctx + TEXT(" survives serialization"), Loaded->Report, Save->Report);

        FCLReportState Report = Sentinel();
        bool bDestTyped = !bTyped;
        if (!TestTrue(Ctx + TEXT(" copies"), CLSaveValidation::CopyIfValid(Loaded, Report, bDestTyped))) continue;
        ExpectSameReport(*this, Ctx + TEXT(" copied"), Report, Save->Report);
        TestTrue(Ctx + TEXT(" Pruitt introduction copied"), Report.bBriefedByPruitt);
        TestTrue(Ctx + TEXT(" field notes copied"), Report.FieldNotes == Save->Report.FieldNotes);
        TestTrue(Ctx + TEXT(" copy preference copied"), bDestTyped == bTyped);

        // The copy is independent of the save object.
        Loaded->Report.FieldNotes.Add(TEXT("LaterEdit"));
        Loaded->Report.bBriefedByPruitt = false;
        TestFalse(Ctx + TEXT(" copy does not alias field notes"), Report.FieldNotes.Contains(TEXT("LaterEdit")));
        TestTrue(Ctx + TEXT(" copy does not alias flags"), Report.bBriefedByPruitt);
    }

    // Unbriefed saves keep the flag false rather than inheriting destination state.
    FCLReportState Unbriefed = FullReport(ECLReportStatus::Draft);
    Unbriefed.bBriefedByPruitt = false;
    FCLReportState Report = Sentinel();
    bool bTyped = true;
    TestTrue(TEXT("Unbriefed save copies"), CLSaveValidation::CopyIfValid(MakeSave(Unbriefed), Report, bTyped));
    TestFalse(TEXT("Unbriefed flag overwrites briefed destination"), Report.bBriefedByPruitt);
    ExpectSameReport(*this, TEXT("Unbriefed copy"), Report, Unbriefed);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLSaveValidationRejectionStateTest,"CountyLine.Save.Validation.RejectionKeepsState",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLSaveValidationRejectionStateTest::RunTest(const FString& Parameters)
{
    using namespace CLSaveValidationTestUtil;

    // An accepted load followed by a rejected one must leave the first result intact.
    FCLReportState Report;
    bool bTyped = true;
    UCLPrototypeSave* Good = MakeSave(FullReport(ECLReportStatus::Held), false);
    TestTrue(TEXT("First save accepted"), CLSaveValidation::CopyIfValid(Good, Report, bTyped));
    const FCLReportState Accepted = Report;

    TArray<UCLPrototypeSave*> BadSaves;
    BadSaves.Add(MakeSave(FullReport(ECLReportStatus::Signed), true, 2));
    FCLReportState BadLine = FullReport(ECLReportStatus::Signed); BadLine.ClosingLine = 3;
    BadSaves.Add(MakeSave(BadLine, true));
    FCLReportState BadStatus = FullReport(ECLReportStatus::Signed); BadStatus.Status = static_cast<ECLReportStatus>(3);
    BadSaves.Add(MakeSave(BadStatus, true));
    for (int32 I = 0; I < BadSaves.Num(); ++I)
    {
        const FString Ctx = FString::Printf(TEXT("Rejected save %d after accepted save"), I);
        TestFalse(Ctx + TEXT(" is rejected"), CLSaveValidation::CopyIfValid(BadSaves[I], Report, bTyped));
        ExpectSameReport(*this, Ctx + TEXT(" keeps accepted report"), Report, Accepted);
        TestFalse(Ctx + TEXT(" keeps accepted copy preference"), bTyped);
    }
    TestFalse(TEXT("Null after accepted save is rejected"), CLSaveValidation::CopyIfValid(nullptr, Report, bTyped));
    ExpectSameReport(*this, TEXT("Null after accepted save keeps report"), Report, Accepted);
    TestFalse(TEXT("Null after accepted save keeps copy preference"), bTyped);

    // Validation never modifies the save it inspects.
    FCLReportState Source = FullReport(ECLReportStatus::Signed);
    Source.ClosingLine = 5;
    UCLPrototypeSave* Inspected = MakeSave(Source, false);
    CLSaveValidation::Validate(Inspected);
    FCLReportState Scratch; bool bScratch = true;
    CLSaveValidation::CopyIfValid(Inspected, Scratch, bScratch);
    ExpectSameReport(*this, TEXT("Inspected save report"), Inspected->Report, Source);
    TestEqual(TEXT("Inspected save version"), Inspected->Version, 1);
    TestFalse(TEXT("Inspected save copy preference"), Inspected->bTypedCopy);
    return true;
}
#endif
