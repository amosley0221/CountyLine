#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CLCountyRoad.generated.h"

// Compact authored route, not the final county geography or streaming system.
UCLASS()
class COUNTYLINE_API ACLCountyRoad : public AActor
{
    GENERATED_BODY()
public:
    ACLCountyRoad();
    static FName LocationAt(FVector Position);
    static bool SafeCheckpoint(FName Location, FTransform& OutTransform);
};
