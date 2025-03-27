// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "Roadnetwork/Structure/RnPoint.h"

URnPoint::URnPoint()
    : Vertex(FVector::ZeroVector) {
}

URnPoint::URnPoint(const FVector& InVertex)
{
    Init(InVertex);
}

void URnPoint::Init()
{}

void URnPoint::Init(const FVector& InVertex)
{
    Vertex = InVertex;
}

TRnRef_T<URnPoint> URnPoint::Clone() const {
    return RnNew<URnPoint>(Vertex);
}

bool URnPoint::Equals(const URnPoint& X, const URnPoint& Y, float SqrMagnitudeTolerance)
{
    // ポインタが同じなら同じ点とみなす
    if (&X == &Y) 
        return true;
    return (X.Vertex - Y.Vertex).SizeSquared() <= SqrMagnitudeTolerance;
}

bool URnPoint::Equals(TRnRef_T<const URnPoint> X, TRnRef_T<const URnPoint> Y, float SqrMagnitudeTolerance)
{
    if (X == Y) return true;
    if (!X || !Y) return false;
    if (SqrMagnitudeTolerance < 0.0f) return false;

    return Equals(*X, *Y, SqrMagnitudeTolerance);
}

bool URnPoint::IsSamePoint(const URnPoint* Other, float SqrMagnitudeTolerance) const {
    return Equals(this, Other, SqrMagnitudeTolerance);
}

bool URnPoint::IsSamePoint(const TRnRef_T<URnPoint>& Other, float SqrMagnitudeTolerance) const
{
    return Equals(this, Other, SqrMagnitudeTolerance);
}
