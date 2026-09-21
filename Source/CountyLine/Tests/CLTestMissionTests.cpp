#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "World/CLTestMission.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCLTrialRules,"CountyLine.Playtest.FreightRules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCLTrialRules::RunTest(const FString&)
{
    FCLTrialState S;
    TestFalse(TEXT("Cannot stop stationary courier before pursuit"),S.Catch());
    TestFalse(TEXT("Cannot resolve before confrontation"),S.Resolve(ECLTrialChoice::Recover));
    S.Advance(100,1000);TestEqual(TEXT("Exploration has no timer"),S.Distance,0.f);
    TestTrue(TEXT("Crate starts pursuit"),S.StartChase());
    TestFalse(TEXT("Crate cannot restart active pursuit"),S.StartChase());
    S.Advance(-2,1000);TestEqual(TEXT("Negative time ignored"),S.Distance,0.f);
    S.Advance(1,1000);TestEqual(TEXT("Runner advances at 260 cm/s"),S.Distance,260.f);
    TestTrue(TEXT("Catch ends pursuit"),S.Catch());
    const float Frozen=S.Distance;S.Advance(5,1000);TestEqual(TEXT("Conversation freezes pursuit"),S.Distance,Frozen);
    TestFalse(TEXT("Unverified shortcut resolution denied"),S.Resolve(ECLTrialChoice::Verify));
    TestFalse(TEXT("Unknown resolution rejected"),S.Resolve(static_cast<ECLTrialChoice>(255)));
    for(auto Choice:{ECLTrialChoice::Recover,ECLTrialChoice::Verify,ECLTrialChoice::Release})
    {
        FCLTrialState Branch=S;Branch.bManifest=true;
        TestTrue(TEXT("Each valid choice resolves"),Branch.Resolve(Choice));
        TestTrue(TEXT("Selected outcome recorded"),Branch.Choice==Choice && Branch.Stage==ECLTrialStage::Complete);
        TestFalse(TEXT("Outcome cannot be rewritten"),Branch.Resolve(ECLTrialChoice::Recover));
        Branch.Advance(100,1000);TestTrue(TEXT("Completed run cannot escape"),Branch.Stage==ECLTrialStage::Complete);
    }
    S=FCLTrialState();S.StartChase();S.Advance(100,1000);
    TestTrue(TEXT("Reaching exit fails pursuit"),S.Stage==ECLTrialStage::Escaped);
    TestEqual(TEXT("Runner stops at exit"),S.Distance,1000.f);
    TestFalse(TEXT("Escaped runner cannot be caught"),S.Catch());
    S=FCLTrialState();TestTrue(TEXT("Replay clears evidence, choice and timing"),!S.bManifest && !S.bGateOpen && S.Choice==ECLTrialChoice::None && S.ChaseSeconds==0 && S.Distance==0);
    return true;
}
#endif
