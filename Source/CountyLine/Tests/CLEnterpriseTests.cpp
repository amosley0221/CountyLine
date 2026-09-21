#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Paper/CLCaseState.h"
#include "Paper/CLSaveValidation.h"
#include "Kismet/GameplayStatics.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLEnterpriseTest,"CountyLine.Report.Enterprise",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLEnterpriseTest::RunTest(const FString& Parameters)
{
    FCLReportState Draft;
    TestFalse(TEXT("Draft cannot be shared"),Draft.ShareWithEnterprise());
    TestFalse(TEXT("Legacy default has no handoff"),Draft.bEnterpriseReviewed);
    for(const ECLReportStatus Status:{ECLReportStatus::Signed,ECLReportStatus::Held})
    for(int32 Mask=0;Mask<8;++Mask)
    {
        FCLReportState R;R.bRead=true;R.bIncludeFinder=(Mask&1)!=0;R.bIncludeBottle=(Mask&2)!=0;R.bIncludeFieldNotes=(Mask&4)!=0;
        R.FieldNotes.Add(TEXT("ResidentAccount"));R.Submit(Status);const FCLReportState Carbon=R;
        TestTrue(TEXT("Submitted carbon can be shared"),R.ShareWithEnterprise());
        TestFalse(TEXT("Repeated handoff is rejected"),R.ShareWithEnterprise());
        const FString Copy=R.EnterpriseCopy();
        TestEqual(TEXT("Only signed included finder is printed"),Copy.Contains(TEXT("names Salazar")),Status==ECLReportStatus::Signed && (Mask&1)!=0);
        TestEqual(TEXT("Only signed included bottle is printed"),Copy.Contains(TEXT("bottle was reported")),Status==ECLReportStatus::Signed && (Mask&2)!=0);
        TestEqual(TEXT("Only signed included resident account is printed"),Copy.Contains(TEXT("families use")),Status==ECLReportStatus::Signed && (Mask&4)!=0);
        TestEqual(TEXT("Held story is explicitly withheld"),Copy.Contains(TEXT("I hold the story")),Status==ECLReportStatus::Held);
        R.bEnterpriseReviewed=false;
        TestTrue(TEXT("Handoff changed only the notice flag"),FCLReportState::StaticStruct()->CompareScriptStruct(&R,&Carbon,0));R.bEnterpriseReviewed=true;
        R.FieldNotes.AddUnique(TEXT("BottleObserved"));R.FollowupFacts.Add(TEXT("BottleSealed"));R.bIncludeBottle=!R.bIncludeBottle;
        TestEqual(TEXT("Later knowledge and changed choices cannot rewrite newspaper copy"),R.EnterpriseCopy(),Copy);
        for(bool bTyped:{false,true})
        {
            UCLPrototypeSave* Save=NewObject<UCLPrototypeSave>();Save->Report=R;Save->bTypedCopy=bTyped;TArray<uint8> Bytes;
            TestTrue(TEXT("Memory serialization succeeds"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
            USaveGame* Loaded=UGameplayStatics::LoadGameFromMemory(Bytes);FCLReportState Restored;bool Typed=!bTyped;
            TestTrue(TEXT("Notice survives production validator"),CLSaveValidation::CopyIfValid(Loaded,Restored,Typed));
            TestTrue(TEXT("Entire newspaper state survives save"),FCLReportState::StaticStruct()->CompareScriptStruct(&R,&Restored,0));
            TestEqual(TEXT("Newspaper copy survives save"),Restored.EnterpriseCopy(),Copy);
        }
    }
    UCLPrototypeSave* Bad=NewObject<UCLPrototypeSave>();Bad->Report.bEnterpriseReviewed=true;
    FCLReportState Out;Out.FieldNotes.Add(TEXT("Unchanged"));const auto Before=Out;bool Typed=false;
    TestTrue(TEXT("Draft handoff rejected"),CLSaveValidation::Validate(Bad)==ECLSaveRejection::InvalidEnterprise);
    TestFalse(TEXT("Invalid handoff cannot copy"),CLSaveValidation::CopyIfValid(Bad,Out,Typed));
    TestTrue(TEXT("Rejected handoff leaves destination untouched"),!Typed && FCLReportState::StaticStruct()->CompareScriptStruct(&Out,&Before,0));
    Bad->Report.Status=ECLReportStatus::Signed;
    TestTrue(TEXT("Unread handoff rejected"),CLSaveValidation::Validate(Bad)==ECLSaveRejection::InvalidEnterprise);
    return true;
}
#endif
