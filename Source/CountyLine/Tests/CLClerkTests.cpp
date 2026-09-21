#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Paper/CLCaseState.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLClerkReviewTest,"CountyLine.Report.ClerkReview",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLClerkReviewTest::RunTest(const FString& Parameters)
{
    FCLReportState Draft;
    TestTrue(TEXT("Clerk refuses unsigned report"),Draft.ClerkResponse().Contains(TEXT("unsigned")));
    for(const ECLReportStatus Status:{ECLReportStatus::Signed,ECLReportStatus::Held})
    for(const bool bInclude:{false,true})
    for(const bool bBefore:{false,true})
    {
        FCLReportState R;R.bRead=true;R.bIncludeFieldNotes=bInclude;
        if(bBefore) R.FieldNotes.Add(TEXT("ResidentAccount"));
        TestTrue(TEXT("Report submits"),R.Submit(Status));
        R.FieldNotes.AddUnique(TEXT("ResidentAccount"));
        const FCLReportState Before=R;
        const FString Reply=R.ClerkResponse();
        TestTrue(TEXT("Clerk recognizes disposition"),Reply.Contains(Status==ECLReportStatus::Signed?TEXT("Signed by S. Reed"):TEXT("Held for inquiry")));
        TestTrue(TEXT("Clerk reads the actual carbon"),Reply.Contains(bBefore?(bInclude?TEXT("account is included"):TEXT("account was omitted")):TEXT("later account")));
        TestTrue(TEXT("Repeated review is stable"),Reply==R.ClerkResponse());
        TestTrue(TEXT("Review changes no report field"),FCLReportState::StaticStruct()->CompareScriptStruct(&R,&Before,0));
    }
    return true;
}
#endif
