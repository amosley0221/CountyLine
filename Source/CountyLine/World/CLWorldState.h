#pragma once

#include "CoreMinimal.h"
#include "CLWorldState.generated.h"

// Persistent world state for future open-world exploration: which locations the
// player has discovered, and the last place they stood safely.
//
// This is storage only. Nothing here spawns the player, travels, streams a level,
// or changes a case consequence. Older saves have no world state at all; Unreal's
// tagged property serialization leaves this struct at its defaults, which is an
// empty, valid world state.
USTRUCT()
struct COUNTYLINE_API FCLWorldState
{
    GENERATED_BODY()

    // Stable identifiers, unique and in the order they were discovered.
    UPROPERTY() TArray<FName> DiscoveredLocations;

    // NAME_None until a safe position is recorded.
    UPROPERTY() FName LastSafeLocation;
    UPROPERTY() FTransform LastSafeTransform = FTransform::Identity;

    // A stable id: 1-64 characters, starting with a letter, then letters,
    // digits, or underscores. No spaces, punctuation, or NAME_None, so ids stay
    // usable as map, table, and telemetry keys.
    static bool IsValidLocationId(FName Id);
    static constexpr int32 MaxLocationIdLength = 64;

    // Every component of translation, rotation, and scale is finite, and the
    // rotation is a normalized quaternion.
    static bool IsFiniteTransform(const FTransform& Transform);

    // Records a discovery. Returns false, changing nothing, for an invalid id or
    // one already discovered.
    bool DiscoverLocation(FName Id);
    bool IsLocationDiscovered(FName Id) const;
    int32 NumDiscoveredLocations() const { return DiscoveredLocations.Num(); }

    // Records where the player was last safe. Returns false, changing nothing,
    // for an invalid id or a non-finite transform. Discovery is left alone, so
    // callers decide whether standing somewhere also discovers it.
    bool SetLastSafePosition(FName Id, const FTransform& Transform);
    bool HasLastSafePosition() const { return !LastSafeLocation.IsNone(); }

    // Rules a loaded save must satisfy: valid, unique ids; a last safe position
    // that is either unset with an identity transform, or a valid id with a
    // finite transform.
    bool IsValidState() const;

    void Reset() { *this = FCLWorldState{}; }
};
