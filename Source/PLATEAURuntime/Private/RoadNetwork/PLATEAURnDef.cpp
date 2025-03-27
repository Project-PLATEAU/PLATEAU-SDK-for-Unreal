// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/PLATEAURnDef.h"
#include "UObject/Package.h"


EPLATEAURnLaneBorderType FPLATEAURnLaneBorderTypeEx::GetOpposite(EPLATEAURnLaneBorderType dir)
{
    return dir == EPLATEAURnLaneBorderType::Prev ? EPLATEAURnLaneBorderType::Next : EPLATEAURnLaneBorderType::Prev;
}

EPLATEAURnDir FPLATEAURnDirEx::GetOpposite(EPLATEAURnDir dir)
{
    return dir == EPLATEAURnDir::Left ? EPLATEAURnDir::Right : EPLATEAURnDir::Left;
}

EPLATEAURnLaneBorderDir FPLATEAURnLaneBorderDirEx::GetOpposite(EPLATEAURnLaneBorderDir dir)
{
    return dir == EPLATEAURnLaneBorderDir::Left2Right ? EPLATEAURnLaneBorderDir::Right2Left : EPLATEAURnLaneBorderDir::Left2Right;
}

UObject* FPLATEAURnDef::GetNewObjectWorld()
{
    if (NewObjectWorld)
        return NewObjectWorld;
    return Cast<UObject>(GetTransientPackage());
}

void FPLATEAURnDef::SetNewObjectWorld(UObject* World)
{
    NewObjectWorld = World;
}

FVector2D FPLATEAURnDef::To2D(const FVector& Vector) {
    return FAxisPlaneEx::ToVector2D(Vector, Plane);
}

FRay2D FPLATEAURnDef::To2D(const FRay& Ray)
{
    return FRay2D(To2D(Ray.Origin), To2D(Ray.Direction));
}

FVector FPLATEAURnDef::To3D(const FVector2D& Vector, float A)
{
    return FAxisPlaneEx::ToVector3D(Vector, Plane, A);
}

int32 FPLATEAURnDef::Vector3Comparer::operator()(const FVector& A, const FVector& B) const {
    auto X = Compare(A.X, B.X);
    if (X != 0)
        return X;
    auto Y = Compare(A.Y, B.Y);
    if (Y != 0)
        return Y;
    return X = Compare(A.Z, B.Z);
}
