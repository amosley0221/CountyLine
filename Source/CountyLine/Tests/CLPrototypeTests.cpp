#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Paper/CLCaseState.h"
#include "Player/CLPlayerController.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLReportTest,"CountyLine.Report.SubmissionAndCarbon",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLReportTest::RunTest(const FString& Parameters)
{
    FCLReportState R;
    TestFalse(TEXT("Cannot sign an unread report"),R.Submit(ECLReportStatus::Signed));
    R.bRead=true;R.bIncludeBottle=false;R.ClosingLine=1;
    TestTrue(TEXT("Sign succeeds"),R.Submit(ECLReportStatus::Signed));
    TestEqual(TEXT("Signed courthouse consequence"),R.CourthouseDelta,2);
    TestTrue(TEXT("Finder retained"),R.IncludedFacts.Contains(TEXT("SalazarFoundBody")));
    TestTrue(TEXT("Omission recorded"),R.OmittedFacts.Contains(TEXT("BottleReported")));
    TestFalse(TEXT("Repeated activation cannot reapply consequence"),R.Submit(ECLReportStatus::Signed));
    TestFalse(TEXT("Signed report cannot become held"),R.Submit(ECLReportStatus::Held));
    FCLReportState Held;Held.bRead=true;
    TestTrue(TEXT("Hold succeeds independently"),Held.Submit(ECLReportStatus::Held));
    TestEqual(TEXT("Held courthouse consequence"),Held.CourthouseDelta,-3);
    FCLReportState Invalid;Invalid.bRead=true;Invalid.ClosingLine=3;
    TestFalse(TEXT("Unknown closing line rejected"),Invalid.Submit(ECLReportStatus::Signed));
    Invalid.ClosingLine=0;
    TestFalse(TEXT("Draft is not a submission"),Invalid.Submit(ECLReportStatus::Draft));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLSaveTest,"CountyLine.Report.SaveRoundTrip",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLSaveTest::RunTest(const FString& Parameters)
{
    auto* Save=NewObject<UCLPrototypeSave>();
    Save->Report.bRead=true;Save->Report.bIncludeFinder=false;Save->Report.ClosingLine=2;
    Save->Report.Submit(ECLReportStatus::Held);Save->bTypedCopy=false;
    TArray<uint8> Bytes;
    TestTrue(TEXT("Serialize without touching user slot"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
    const auto* Loaded=Cast<UCLPrototypeSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
    TestNotNull(TEXT("Save deserializes"),Loaded);
    if(Loaded)
    {
        TestTrue(TEXT("Held status survives"),Loaded->Report.Status==ECLReportStatus::Held);
        TestEqual(TEXT("Closing line survives"),Loaded->Report.ClosingLine,2);
        TestTrue(TEXT("Omitted finder survives"),Loaded->Report.OmittedFacts.Contains(TEXT("SalazarFoundBody")));
        TestFalse(TEXT("Copy preference survives"),Loaded->bTypedCopy);
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
