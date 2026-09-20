#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Paper/CLCaseState.h"
#include "Paper/CLSaveValidation.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLResidentAccountTest,"CountyLine.Report.ResidentAccount",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLResidentAccountTest::RunTest(const FString& Parameters)
{
    const FName Note(TEXT("ResidentAccount"));
    FCLReportState Unknown;
    TestTrue(TEXT("Pruitt does not know an unrecorded resident account"),Unknown.PruittResidentResponse().IsEmpty());
    Unknown.FieldNotes.Add(Note);
    const auto DraftBefore=Unknown;
    TestTrue(TEXT("Draft response points to unsigned report"),Unknown.PruittResidentResponse().Contains(TEXT("still unsigned")));
    TestTrue(TEXT("Discussing a draft changes no report fields"),FCLReportState::StaticStruct()->CompareScriptStruct(&Unknown,&DraftBefore,0));
    for(const ECLReportStatus Status:{ECLReportStatus::Signed,ECLReportStatus::Held})
    for(const bool bInclude:{false,true})
    for(const bool bBeforeSubmission:{false,true})
    {
        FCLReportState Report;Report.bRead=true;Report.bIncludeFieldNotes=bInclude;
        if(bBeforeSubmission) Report.FieldNotes.AddUnique(Note);
        TestTrue(TEXT("Report submits"),Report.Submit(Status));
        const auto Included=Report.IncludedFacts;const auto Omitted=Report.OmittedFacts;
        const FString Closing=Report.CarbonClosingLine;const int32 Delta=Report.CourthouseDelta;
        Report.FieldNotes.AddUnique(Note);Report.FieldNotes.AddUnique(Note);
        TestEqual(TEXT("Resident account is unique"),Report.FieldNotes.Num(),1);
        TestTrue(TEXT("Later or repeated account preserves carbon and disposition"),Report.IncludedFacts==Included && Report.OmittedFacts==Omitted && Report.CarbonClosingLine==Closing && Report.CourthouseDelta==Delta && Report.Status==Status);
        TestEqual(TEXT("Known account can be included at submission"),Report.IncludedFacts.Contains(Note),bBeforeSubmission && bInclude);
        TestEqual(TEXT("Known account can be omitted at submission"),Report.OmittedFacts.Contains(Note),bBeforeSubmission && !bInclude);
        const auto BeforeDiscussion=Report;
        const FString Reply=Report.PruittResidentResponse();
        TestTrue(TEXT("Pruitt acknowledges the actual disposition"),Reply.Contains(Status==ECLReportStatus::Signed?TEXT("signed the report"):TEXT("held the report")));
        TestTrue(TEXT("Pruitt distinguishes included omitted and later accounts"),Reply.Contains(bBeforeSubmission?(bInclude?TEXT("already included"):TEXT("left that account out")):TEXT("later account")));
        TestTrue(TEXT("Repeated discussion is stable"),Reply==Report.PruittResidentResponse());
        TestTrue(TEXT("Discussion preserves every report field"),FCLReportState::StaticStruct()->CompareScriptStruct(&Report,&BeforeDiscussion,0));
        for(const bool bTyped:{false,true})
        {
            UCLPrototypeSave* Save=NewObject<UCLPrototypeSave>();Save->Report=Report;Save->bTypedCopy=bTyped;
            TArray<uint8> Bytes;
            if(!TestTrue(TEXT("Memory save succeeds"),UGameplayStatics::SaveGameToMemory(Save,Bytes))) continue;
            USaveGame* Loaded=UGameplayStatics::LoadGameFromMemory(Bytes);
            FCLReportState Restored;bool bRestoredTyped=!bTyped;
            TestTrue(TEXT("Resident account save passes production validator"),CLSaveValidation::CopyIfValid(Loaded,Restored,bRestoredTyped));
            TestTrue(TEXT("Entire report survives memory round trip"),FCLReportState::StaticStruct()->CompareScriptStruct(&Report,&Restored,0));
            TestEqual(TEXT("Copy preference survives"),bRestoredTyped,bTyped);
        }
    }
    return true;
}
#endif
