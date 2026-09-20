#include "World/CLWorldState.h"

bool FCLWorldState::IsValidLocationId(FName Id)
{
    if (Id.IsNone()) return false;
    // FName keeps the authored text; compare on the plain string so ids stay
    // readable in saves and logs.
    const FString Text = Id.ToString();
    if (Text.IsEmpty() || Text.Len() > MaxLocationIdLength) return false;
    if (!FChar::IsAlpha(Text[0])) return false;
    for (const TCHAR Character : Text)
        if (!FChar::IsAlnum(Character) && Character != TEXT('_')) return false;
    return true;
}

bool FCLWorldState::IsFiniteTransform(const FTransform& Transform)
{
    auto FiniteVector = [](const FVector& V) { return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z); };
    const FQuat Rotation = Transform.GetRotation();
    if (!FiniteVector(Transform.GetTranslation()) || !FiniteVector(Transform.GetScale3D())) return false;
    if (!FMath::IsFinite(Rotation.X) || !FMath::IsFinite(Rotation.Y) || !FMath::IsFinite(Rotation.Z) || !FMath::IsFinite(Rotation.W)) return false;
    // A denormalized rotation would skew anything later built on this transform.
    return Rotation.IsNormalized();
}

bool FCLWorldState::DiscoverLocation(FName Id)
{
    if (!IsValidLocationId(Id) || DiscoveredLocations.Contains(Id)) return false;
    DiscoveredLocations.Add(Id);
    return true;
}

bool FCLWorldState::IsLocationDiscovered(FName Id) const
{
    return !Id.IsNone() && DiscoveredLocations.Contains(Id);
}

bool FCLWorldState::SetLastSafePosition(FName Id, const FTransform& Transform)
{
    if (!IsValidLocationId(Id) || !IsFiniteTransform(Transform)) return false;
    LastSafeLocation = Id;
    LastSafeTransform = Transform;
    return true;
}

bool FCLWorldState::IsValidState() const
{
    TSet<FName> Seen;
    Seen.Reserve(DiscoveredLocations.Num());
    for (const FName Id : DiscoveredLocations)
    {
        if (!IsValidLocationId(Id) || Seen.Contains(Id)) return false;
        Seen.Add(Id);
    }
    if (LastSafeLocation.IsNone()) return LastSafeTransform.Equals(FTransform::Identity);
    return IsValidLocationId(LastSafeLocation) && IsFiniteTransform(LastSafeTransform);
}
