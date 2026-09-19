#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Paper/CLCaseState.h"
#include "Paper/CLSaveValidation.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLFollowupTest,"CountyLine.Followup.ChoicesAndPersistence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLFollowupTest::RunTest(const FString& Parameters)
{
    auto Same=[](const FCLReportState& A,const FCLReportState& B){return FCLReportState::StaticStruct()->CompareScriptStruct(&A,&B,0);};
    auto RoundTrip=[this,&Same](const FCLReportState& R)
    {
        UCLPrototypeSave* Save=NewObject<UCLPrototypeSave>();Save->Report=R;Save->bTypedCopy=false;
        TArray<uint8> Bytes;TestTrue(TEXT("memory serialization"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
        USaveGame* Loaded=UGameplayStatics::LoadGameFromMemory(Bytes);
        FCLReportState Copy;bool Typed=true;
        TestTrue(TEXT("validated load"),CLSaveValidation::CopyIfValid(Loaded,Copy,Typed));
        TestTrue(TEXT("all reflected fields survive"),Same(Copy,R));TestFalse(TEXT("copy preference survives"),Typed);
        return Copy;
    };
    RoundTrip(FCLReportState{}); // Default fields are the compatibility state for older saves.
    const ECLFollowupLead Leads[]={ECLFollowupLead::Bottle,ECLFollowupLead::Bank,ECLFollowupLead::Finder};
    const int32 Actions[]={2,3,1};
    const FName Notes[]={TEXT("BottleObserved"),TEXT("BankExamined"),TEXT("SalazarStatement")};
    for(int32 I=0;I<3;++I)
    {
        FCLReportState R;const auto Empty=R;
        TestFalse(TEXT("unread blocked"),R.Pursue(Leads[I]));TestTrue(TEXT("unread unchanged"),Same(R,Empty));
        R.bRead=true;TestFalse(TEXT("missing evidence blocked"),R.Pursue(Leads[I]));
        R.FieldNotes.Add(Notes[I]);TestTrue(TEXT("known note unlocks lead"),R.Pursue(Leads[I]));
        const auto Pending=R;
        TestFalse(TEXT("repeat selection rejected"),R.Pursue(Leads[I]));
        TestFalse(TEXT("wrong field target rejected"),R.CompleteFollowup((Actions[I]%3)+1));
        TestTrue(TEXT("rejections leave pending unchanged"),Same(R,Pending));
        R=RoundTrip(R);
        TestTrue(TEXT("matching target completes loaded lead"),R.CompleteFollowup(Actions[I]));
        TestTrue(TEXT("only correct fact awarded"),R.FollowupFacts.Num()==1 && R.FollowupFacts[0]==FCLReportState::FollowupFact(Leads[I]));
        const auto Completed=R;
        TestFalse(TEXT("completion not farmable"),R.CompleteFollowup(Actions[I]));
        TestFalse(TEXT("completed lead cannot restart"),R.Pursue(Leads[I]));
        TestFalse(TEXT("draft cannot file supplement"),R.FileFollowup(ECLFollowupOutcome::FileSupplement));
        TestTrue(TEXT("completed rejections unchanged"),Same(R,Completed));
        RoundTrip(R);
    }
    for(ECLReportStatus Status:{ECLReportStatus::Signed,ECLReportStatus::Held})
    for(ECLFollowupOutcome Outcome:{ECLFollowupOutcome::FileSupplement,ECLFollowupOutcome::RequestInquiry})
    for(int32 Mask=1;Mask<8;++Mask)
    {
        FCLReportState R;R.bRead=true;R.Submit(Status);const auto Carbon=R;
        R.FieldNotes={Notes[0],Notes[1],Notes[2]};
        for(int32 I=0;I<3;++I) if(Mask&(1<<I)) {R.Pursue(Leads[I]);R.CompleteFollowup(Actions[I]);}
        TestTrue(TEXT("file once"),R.FileFollowup(Outcome));
        TestTrue(TEXT("snapshot contains observations"),R.SupplementFacts==R.FollowupFacts);
        TestTrue(TEXT("original carbon intact"),R.Status==Carbon.Status && R.IncludedFacts==Carbon.IncludedFacts && R.OmittedFacts==Carbon.OmittedFacts && R.CarbonClosingLine==Carbon.CarbonClosingLine && R.CourthouseDelta==Carbon.CourthouseDelta);
        FCLReportState Loaded=RoundTrip(R);const auto Before=Loaded;
        TestFalse(TEXT("cannot file twice"),Loaded.FileFollowup(Outcome));
        TestFalse(TEXT("cannot pursue after filing"),Loaded.Pursue(ECLFollowupLead::Bottle));
        TestTrue(TEXT("locked loaded state unchanged"),Same(Loaded,Before));
        TestFalse(TEXT("consequence exists"),Loaded.FollowupConsequence().IsEmpty());
    }
    FCLReportState Pending;Pending.bRead=true;Pending.FieldNotes={Notes[0],Notes[1]};
    Pending.Pursue(Leads[0]);Pending.CompleteFollowup(Actions[0]);Pending.Submit(ECLReportStatus::Held);Pending.Pursue(Leads[1]);
    TestFalse(TEXT("active lead blocks filing"),Pending.FileFollowup(ECLFollowupOutcome::RequestInquiry));
    TestTrue(TEXT("follow-up facts included in a later original submission"),Pending.IncludedFacts.Contains(TEXT("BottleSealed")));
    for(int32 Invalid=0;Invalid<5;++Invalid)
    {
        UCLPrototypeSave* Bad=NewObject<UCLPrototypeSave>();
        if(Invalid==0) Bad->Report.FollowupLead=static_cast<ECLFollowupLead>(255);
        if(Invalid==1) Bad->Report.FollowupOutcome=static_cast<ECLFollowupOutcome>(255);
        if(Invalid==2) Bad->Report.FollowupFacts={TEXT("UnknownFact")};
        if(Invalid==3) Bad->Report.FollowupFacts={TEXT("BottleSealed"),TEXT("BottleSealed")};
        if(Invalid==4) Bad->Report.FollowupOutcome=ECLFollowupOutcome::RequestInquiry;
        FCLReportState Dest=Pending;bool Typed=false;
        TestFalse(TEXT("malformed follow-up rejected"),CLSaveValidation::CopyIfValid(Bad,Dest,Typed));
        TestTrue(TEXT("rejected load preserves destination"),Same(Dest,Pending));TestFalse(TEXT("rejected load preserves preference"),Typed);
    }
    return true;
}
#endif
