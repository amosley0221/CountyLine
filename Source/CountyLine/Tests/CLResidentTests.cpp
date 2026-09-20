#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Paper/CLCaseState.h"
#include "Paper/CLSaveValidation.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLResidentAccountTest,"CountyLine.Report.ResidentAccount",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLResidentAccountTest::RunTest(const FString& Parameters)
{
    const FName Note(TEXT("ResidentAccount"));
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
