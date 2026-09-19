#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Paper/CLCaseState.h"
#include "Player/CLPlayerController.h"
#include "Kismet/GameplayStatics.h"

// Test names are fixed: Scripts/Test-Prototype.ps1 expects exactly three CountyLine tests.
namespace CLReportTestUtil
{
    static const TCHAR* Finder = TEXT("SalazarFoundBody");
    static const TCHAR* Bottle = TEXT("BottleReported");
    static const ECLReportStatus Submissions[] = {ECLReportStatus::Signed, ECLReportStatus::Held};

    static FString Describe(bool bFinder, bool bBottle, int32 Line, ECLReportStatus Status)
    {
        return FString::Printf(TEXT("[finder=%s bottle=%s line=%d %s]"),
            bFinder ? TEXT("in") : TEXT("out"), bBottle ? TEXT("in") : TEXT("out"), Line, *UCLCaseState::StatusText(Status));
    }

    static FCLReportState ReadDraft(bool bFinder, bool bBottle, int32 Line)
    {
        FCLReportState R;
        R.bRead = true; R.bIncludeFinder = bFinder; R.bIncludeBottle = bBottle; R.ClosingLine = Line;
        return R;
    }

    // Field-by-field comparison so a failure names the field that drifted.
    static void ExpectSameReport(FAutomationTestBase& Test, const FString& Ctx, const FCLReportState& Actual, const FCLReportState& Expected)
    {
        Test.TestTrue(Ctx + TEXT(" complete reflected state"), FCLReportState::StaticStruct()->CompareScriptStruct(&Actual,&Expected,0));
        Test.TestTrue(Ctx + TEXT(" status"), Actual.Status == Expected.Status);
        Test.TestTrue(Ctx + TEXT(" read flag"), Actual.bRead == Expected.bRead);
        Test.TestTrue(Ctx + TEXT(" Pruitt introduction flag"), Actual.bBriefedByPruitt == Expected.bBriefedByPruitt);
        Test.TestTrue(Ctx + TEXT(" finder selection"), Actual.bIncludeFinder == Expected.bIncludeFinder);
        Test.TestTrue(Ctx + TEXT(" bottle selection"), Actual.bIncludeBottle == Expected.bIncludeBottle);
        Test.TestEqual(Ctx + TEXT(" closing line"), Actual.ClosingLine, Expected.ClosingLine);
        Test.TestTrue(Ctx + TEXT(" included facts"), Actual.IncludedFacts == Expected.IncludedFacts);
        Test.TestTrue(Ctx + TEXT(" omitted facts"), Actual.OmittedFacts == Expected.OmittedFacts);
        Test.TestEqual(Ctx + TEXT(" courthouse delta"), Actual.CourthouseDelta, Expected.CourthouseDelta);
    }

    static void ExpectRejected(FAutomationTestBase& Test, const FString& Ctx, FCLReportState& R, ECLReportStatus Attempt)
    {
        const FCLReportState Before = R;
        Test.TestFalse(Ctx + TEXT(" is rejected"), R.Submit(Attempt));
        ExpectSameReport(Test, Ctx + TEXT(" leaves"), R, Before);
    }

    static UCLPrototypeSave* RoundTrip(FAutomationTestBase& Test, const FString& Ctx, const FCLReportState& Report, bool bTyped, TArray<uint8>* OutBytes = nullptr)
    {
        UCLPrototypeSave* Save = NewObject<UCLPrototypeSave>();
        Save->Report = Report; Save->bTypedCopy = bTyped;
        TArray<uint8> Bytes;
        // Memory only: never touches the player's CountyLine_JailPrototype slot.
        if (!Test.TestTrue(Ctx + TEXT(" serializes"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return nullptr;
        if (OutBytes) *OutBytes = Bytes;
        UCLPrototypeSave* Loaded = Cast<UCLPrototypeSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
        Test.TestNotNull(Ctx + TEXT(" deserializes as UCLPrototypeSave"), Loaded);
        return Loaded;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLReportTest,"CountyLine.Report.SubmissionAndCarbon",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLReportTest::RunTest(const FString& Parameters)
{
    using namespace CLReportTestUtil;

    for(bool Include : {false,true})
    {
        FCLReportState Field=ReadDraft(true,true,2);
        Field.FieldNotes={TEXT("SalazarStatement"),TEXT("BottleObserved"),TEXT("BankExamined")};
        Field.bIncludeFieldNotes=Include;
        TestTrue(TEXT("Field report submits"),Field.Submit(ECLReportStatus::Held));
        TestTrue(TEXT("Discovered facts have explicit carbon disposition"),(Include?Field.IncludedFacts:Field.OmittedFacts).Contains(TEXT("SalazarStatement")));
        TestTrue(TEXT("Heard witness changes closing text"),Field.CarbonClosingLine.Contains(TEXT("finder was heard")));
        if(UCLPrototypeSave* Loaded=RoundTrip(*this,TEXT("Field evidence round trip"),Field,false)) ExpectSameReport(*this,TEXT("Field evidence preserved"),Loaded->Report,Field);
        const FString Carbon=Field.CarbonClosingLine;
        Field.FieldNotes.Reset();
        TestEqual(TEXT("Submitted closing text remains frozen"),Field.ClosingText(2),Carbon);
        ExpectRejected(*this,TEXT("Field report cannot be resubmitted"),Field,ECLReportStatus::Signed);
    }

    // Authored closing lines and status labels.
    for (int32 Line = 0; Line < 3; ++Line)
    {
        TestFalse(FString::Printf(TEXT("Closing line %d has text"), Line), FString(UCLCaseState::ClosingLines[Line]).IsEmpty());
        for (int32 Other = Line + 1; Other < 3; ++Other)
            TestNotEqual(FString::Printf(TEXT("Closing lines %d and %d differ"), Line, Other), FString(UCLCaseState::ClosingLines[Line]), FString(UCLCaseState::ClosingLines[Other]));
    }
    TestEqual(TEXT("Draft label"), UCLCaseState::StatusText(ECLReportStatus::Draft), FString(TEXT("UNSIGNED")));
    TestEqual(TEXT("Signed label"), UCLCaseState::StatusText(ECLReportStatus::Signed), FString(TEXT("SIGNED / S. REED")));
    TestEqual(TEXT("Held label"), UCLCaseState::StatusText(ECLReportStatus::Held), FString(TEXT("HELD FOR INQUIRY")));

    // Fresh report defaults: unread draft, both facts selected, first closing line, no carbon.
    const FCLReportState Fresh;
    TestTrue(TEXT("Fresh report is a draft"), Fresh.Status == ECLReportStatus::Draft);
    TestFalse(TEXT("Fresh report is unread"), Fresh.bRead);
    TestFalse(TEXT("Fresh report has not heard Pruitt"), Fresh.bBriefedByPruitt);
    TestTrue(TEXT("Fresh report includes finder"), Fresh.bIncludeFinder);
    TestTrue(TEXT("Fresh report includes bottle"), Fresh.bIncludeBottle);
    TestEqual(TEXT("Fresh report uses first closing line"), Fresh.ClosingLine, 0);
    TestEqual(TEXT("Fresh report has no included facts"), Fresh.IncludedFacts.Num(), 0);
    TestEqual(TEXT("Fresh report has no omitted facts"), Fresh.OmittedFacts.Num(), 0);
    TestEqual(TEXT("Fresh report has no consequence"), Fresh.CourthouseDelta, 0);

    // Every included/omitted combination x every closing line x SIGN/HOLD.
    for (const bool bFinder : {true, false})
    for (const bool bBottle : {true, false})
    for (int32 Line = 0; Line < 3; ++Line)
    for (const ECLReportStatus Status : Submissions)
    {
        const FString Ctx = Describe(bFinder, bBottle, Line, Status);
        FCLReportState R = ReadDraft(bFinder, bBottle, Line);
        // Stale carbon entries must be replaced, not appended to.
        R.IncludedFacts.Add(TEXT("StaleIncluded")); R.OmittedFacts.Add(TEXT("StaleOmitted"));
        if (!TestTrue(Ctx + TEXT(" submission succeeds"), R.Submit(Status))) continue;

        TestTrue(Ctx + TEXT(" status recorded"), R.Status == Status);
        TestEqual(Ctx + TEXT(" courthouse consequence"), R.CourthouseDelta, Status == ECLReportStatus::Signed ? 2 : -3);
        TestEqual(Ctx + TEXT(" closing line kept"), R.ClosingLine, Line);
        TestTrue(Ctx + TEXT(" still read"), R.bRead);
        TestTrue(Ctx + TEXT(" finder selection kept"), R.bIncludeFinder == bFinder);
        TestTrue(Ctx + TEXT(" bottle selection kept"), R.bIncludeBottle == bBottle);

        TArray<FName> ExpectedIncluded, ExpectedOmitted;
        (bFinder ? ExpectedIncluded : ExpectedOmitted).Add(Finder);
        (bBottle ? ExpectedIncluded : ExpectedOmitted).Add(Bottle);
        TestTrue(Ctx + TEXT(" included facts exact"), R.IncludedFacts == ExpectedIncluded);
        TestTrue(Ctx + TEXT(" omitted facts exact"), R.OmittedFacts == ExpectedOmitted);
        TestEqual(Ctx + TEXT(" every known fact is accounted for once"), R.IncludedFacts.Num() + R.OmittedFacts.Num(), 2);

        // Once submitted the report is locked: no second consequence, no status change,
        // and later edits to the selections do not rewrite the carbon.
        const FCLReportState Submitted = R;
        for (const ECLReportStatus Again : Submissions)
            ExpectRejected(*this, Ctx + TEXT(" resubmit as ") + UCLCaseState::StatusText(Again), R, Again);
        ExpectRejected(*this, Ctx + TEXT(" revert to draft"), R, ECLReportStatus::Draft);
        R.bIncludeFinder = !bFinder; R.bIncludeBottle = !bBottle; R.ClosingLine = (Line + 1) % 3;
        const FCLReportState Edited = R;
        for (const ECLReportStatus Again : Submissions)
            ExpectRejected(*this, Ctx + TEXT(" resubmit after edits as ") + UCLCaseState::StatusText(Again), R, Again);
        TestTrue(Ctx + TEXT(" carbon included facts frozen"), R.IncludedFacts == Submitted.IncludedFacts);
        TestTrue(Ctx + TEXT(" carbon omitted facts frozen"), R.OmittedFacts == Submitted.OmittedFacts);
        TestEqual(Ctx + TEXT(" consequence frozen"), R.CourthouseDelta, Submitted.CourthouseDelta);
        ExpectSameReport(*this, Ctx + TEXT(" after rejected edits"), R, Edited);
    }

    // Rejected drafts keep every field, including any existing carbon arrays and delta.
    for (const ECLReportStatus Status : Submissions)
    {
        const FString S = UCLCaseState::StatusText(Status);
        FCLReportState Unread = ReadDraft(false, false, 2);
        Unread.bRead = false;
        ExpectRejected(*this, TEXT("Unread draft as ") + S, Unread, Status);
        FCLReportState Default;
        ExpectRejected(*this, TEXT("Untouched default report as ") + S, Default, Status);

        for (const int32 BadLine : {-1, 3, 4, MAX_int32, MIN_int32})
        {
            FCLReportState R = ReadDraft(true, false, BadLine);
            R.IncludedFacts.Add(Finder); R.OmittedFacts.Add(Bottle); R.CourthouseDelta = 7;
            ExpectRejected(*this, FString::Printf(TEXT("Closing line %d as %s"), BadLine, *S), R, Status);
        }
    }
    for (const bool bRead : {true, false})
    {
        FCLReportState R = ReadDraft(false, true, 1);
        R.bRead = bRead;
        ExpectRejected(*this, bRead ? TEXT("Read draft submitted as Draft") : TEXT("Unread draft submitted as Draft"), R, ECLReportStatus::Draft);
        ExpectRejected(*this, bRead ? TEXT("Read draft with unknown status") : TEXT("Unread draft with unknown status"), R, static_cast<ECLReportStatus>(3));
        ExpectRejected(*this, bRead ? TEXT("Read draft with max status") : TEXT("Unread draft with max status"), R, static_cast<ECLReportStatus>(255));
    }

    // A rejected attempt does not consume the draft: fixing the problem lets it submit once.
    FCLReportState Recover = ReadDraft(true, false, 3);
    ExpectRejected(*this, TEXT("Recoverable draft with bad line"), Recover, ECLReportStatus::Held);
    Recover.ClosingLine = 2;
    TestTrue(TEXT("Corrected draft submits"), Recover.Submit(ECLReportStatus::Held));
    TestEqual(TEXT("Corrected draft gets one held consequence"), Recover.CourthouseDelta, -3);
    FCLReportState Unlock = ReadDraft(true, true, 0);
    Unlock.bRead = false;
    ExpectRejected(*this, TEXT("Draft before reading"), Unlock, ECLReportStatus::Signed);
    Unlock.bRead = true;
    TestTrue(TEXT("Draft submits after reading"), Unlock.Submit(ECLReportStatus::Signed));
    TestEqual(TEXT("Read draft gets one signed consequence"), Unlock.CourthouseDelta, 2);

    // Start Again replaces the report with a default-constructed one; that must be submittable again.
    FCLReportState Restarted = FCLReportState{};
    ExpectSameReport(*this, TEXT("Start Again state"), Restarted, Fresh);
    Restarted.bRead = true;
    TestTrue(TEXT("Restarted report can be signed"), Restarted.Submit(ECLReportStatus::Signed));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLSaveTest,"CountyLine.Report.SaveRoundTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLSaveTest::RunTest(const FString& Parameters)
{
    using namespace CLReportTestUtil;

    const UCLPrototypeSave* Defaults = GetDefault<UCLPrototypeSave>();
    TestEqual(TEXT("New saves are version 1"), Defaults->Version, 1);
    TestTrue(TEXT("New saves default to typed copy"), Defaults->bTypedCopy);

    // Submitted reports: every combination x closing line x SIGN/HOLD x copy preference x introduction state.
    for (const bool bFinder : {true, false})
    for (const bool bBottle : {true, false})
    for (int32 Line = 0; Line < 3; ++Line)
    for (const ECLReportStatus Status : Submissions)
    for (const bool bTyped : {true, false})
    for (const bool bBriefed : {true, false})
    {
        const FString Ctx = Describe(bFinder, bBottle, Line, Status) + (bTyped ? TEXT("[typed]") : TEXT("[handwritten]")) + (bBriefed ? TEXT("[briefed]") : TEXT("[not briefed]"));
        FCLReportState R = ReadDraft(bFinder, bBottle, Line);
        R.bBriefedByPruitt = bBriefed;
        if (!TestTrue(Ctx + TEXT(" submits before save"), R.Submit(Status))) continue;

        TArray<uint8> Bytes;
        UCLPrototypeSave* Loaded = RoundTrip(*this, Ctx, R, bTyped, &Bytes);
        if (!Loaded) continue;
        TestEqual(Ctx + TEXT(" version survives"), Loaded->Version, 1);
        TestTrue(Ctx + TEXT(" copy preference survives"), Loaded->bTypedCopy == bTyped);
        ExpectSameReport(*this, Ctx + TEXT(" loaded"), Loaded->Report, R);

        // Loaded submissions stay locked and rejected attempts leave the loaded state unchanged.
        for (const ECLReportStatus Again : Submissions)
            ExpectRejected(*this, Ctx + TEXT(" loaded resubmit as ") + UCLCaseState::StatusText(Again), Loaded->Report, Again);
        TArray<uint8> Resaved;
        TestTrue(Ctx + TEXT(" re-serializes after rejections"), UGameplayStatics::SaveGameToMemory(Loaded, Resaved));
        TestTrue(Ctx + TEXT(" rejected attempts do not change saved bytes"), Resaved == Bytes);
    }

    // Draft reports: unread, and read with every pending selection. Saving mid-edit keeps the
    // selections and the loaded draft submits to the same result as the original would.
    {
        const FCLReportState Unread;
        UCLPrototypeSave* Loaded = RoundTrip(*this, TEXT("Unread draft"), Unread, true);
        if (Loaded)
        {
            ExpectSameReport(*this, TEXT("Unread draft loaded"), Loaded->Report, Unread);
            ExpectRejected(*this, TEXT("Loaded unread draft sign"), Loaded->Report, ECLReportStatus::Signed);
        }
    }
    for (const bool bFinder : {true, false})
    for (const bool bBottle : {true, false})
    for (int32 Line = 0; Line < 3; ++Line)
    for (const ECLReportStatus Status : Submissions)
    {
        const FString Ctx = TEXT("Draft ") + Describe(bFinder, bBottle, Line, Status);
        const FCLReportState Draft = ReadDraft(bFinder, bBottle, Line);
        UCLPrototypeSave* Loaded = RoundTrip(*this, Ctx, Draft, false);
        if (!Loaded) continue;
        ExpectSameReport(*this, Ctx + TEXT(" loaded"), Loaded->Report, Draft);
        FCLReportState Original = Draft;
        TestTrue(Ctx + TEXT(" original submits"), Original.Submit(Status));
        TestTrue(Ctx + TEXT(" loaded draft submits"), Loaded->Report.Submit(Status));
        ExpectSameReport(*this, Ctx + TEXT(" loaded submission matches original"), Loaded->Report, Original);
    }

    // A rejected submission before saving must not leak into the save.
    for (const ECLReportStatus Status : Submissions)
    {
        const FString S = UCLCaseState::StatusText(Status);
        FCLReportState Signed = ReadDraft(false, true, 1);
        Signed.Submit(ECLReportStatus::Signed);
        TArray<uint8> Before, After;
        RoundTrip(*this, TEXT("Signed before rejected ") + S, Signed, true, &Before);
        ExpectRejected(*this, TEXT("Signed report then ") + S, Signed, Status);
        RoundTrip(*this, TEXT("Signed after rejected ") + S, Signed, true, &After);
        TestTrue(TEXT("Rejected ") + S + TEXT(" leaves saved signed bytes identical"), Before == After);

        FCLReportState BadLine = ReadDraft(true, false, 3);
        RoundTrip(*this, TEXT("Bad-line draft before rejected ") + S, BadLine, false, &Before);
        ExpectRejected(*this, TEXT("Bad-line draft then ") + S, BadLine, Status);
        RoundTrip(*this, TEXT("Bad-line draft after rejected ") + S, BadLine, false, &After);
        TestTrue(TEXT("Rejected ") + S + TEXT(" leaves saved draft bytes identical"), Before == After);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLInteractionTest,"CountyLine.Interaction.RangeAndView",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLInteractionTest::RunTest(const FString& Parameters)
{
    const FVector Origin(0,0,0),Forward(1,0,0);
    TestTrue(TEXT("In reach and ahead"),ACLPlayerController::WithinInteractionGate(Origin,Origin,Forward,FVector(200,0,0)));
    TestFalse(TEXT("Beyond 2.5m"),ACLPlayerController::WithinInteractionGate(Origin,Origin,Forward,FVector(251,0,0)));
    TestFalse(TEXT("Behind camera"),ACLPlayerController::WithinInteractionGate(Origin,Origin,Forward,FVector(-100,0,0)));
    TestFalse(TEXT("Outside view cone"),ACLPlayerController::WithinInteractionGate(Origin,Origin,Forward,FVector(100,100,0)));
    TestTrue(TEXT("Range uses pawn, not shoulder camera"),ACLPlayerController::WithinInteractionGate(FVector(200,0,0),FVector(-100,0,0),Forward,FVector(400,0,0)));
    return true;
}
#endif
