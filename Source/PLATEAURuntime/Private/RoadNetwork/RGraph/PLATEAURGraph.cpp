// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/RGraph/PLATEAURGraph.h"

#include "Algo/Count.h"
#include "RoadNetwork/RGraph/RGraphEx.h"
#include "RoadNetwork/Util/PLATEAURnDebugEx.h"


struct FDrawWork {
    URGraph* Graph;
    TSet<RGraphRef_t<URVertex>> VisitedVertices;
    TSet<RGraphRef_t<UREdge>>VisitedEdges;
    TSet<RGraphRef_t<URFace>>VisitedFaces;    
};

UPLATEAURGraph::UPLATEAURGraph() {
    bShowNormal = true;
    ShowFaceType = (int32)FRRoadTypeMaskEx::All();
    RemoveFaceType = (int32)ERRoadTypeMask::Empty;
    ShowId = ERPartsFlag::None;
    RoadColor = FLinearColor::White;
    HighWayColor = FLinearColor::Blue;
    SideWalkColor = FLinearColor::Green;
    MedianColor = FLinearColor::Yellow;
    UndefinedColor = FLinearColor::Red;
    VertexOption.bVisible = false;
    EdgeOption.ShowTypeMask = (int32)FRRoadTypeMaskEx::All();
}

FLinearColor UPLATEAURGraph::GetColor(ERRoadTypeMask RoadType) {

    if(FRRoadTypeMaskEx::IsHighWay(RoadType))
        return HighWayColor;
    if (FRRoadTypeMaskEx::IsSideWalk(RoadType))
        return SideWalkColor;
    if (FRRoadTypeMaskEx::IsMedian(RoadType))
        return MedianColor;
    if (FRRoadTypeMaskEx::IsRoad(RoadType))
        return RoadColor;
    return UndefinedColor;
}

void UPLATEAURGraph::Draw(const FRGraphDrawVertexOption& Op, RGraphRef_t<URVertex> Vertex, FDrawWork& Work) {
    if (!Op.bVisible || Work.VisitedVertices.Contains(Vertex))
        return;

    Work.VisitedVertices.Add(Vertex);

    FString Text = TEXT("●");
    if (Op.bShowEdgeCount)
        Text += FString::Printf(TEXT("%d"), Vertex->GetEdges().Num());

    //if (EnumHasAnyFlags(ShowId, ERPartsFlag::Vertex))
    //    Text += FString::Printf(TEXT("[%d]"), Vertex->GetDebugMyId());

    if (Op.bShowPos)
        Text += FString::Printf(TEXT("(%.2f,%.2f)"), Vertex->GetPosition().X, Vertex->GetPosition().Z);

    // Draw vertex visualization
    FPLATEAURnDebugEx::DrawString(Text, Vertex->GetPosition(),   GetColor(Vertex->GetTypeMaskOrDefault(Op.bUseAnyFaceVertexColor)));

    //if (TargetVertices.Contains(Vertex) && Op.NeighborOption.bVisible) {
    //    for (const auto& Edge : Vertex->GetEdges()) {
    //        FPLATEAURnDebugEx::DrawLine(Edge->GetV0()->GetPosition(),
    //            Edge->GetV1()->GetPosition(), Op.NeighborOption.Color);
    //    }
    //}
}

void UPLATEAURGraph::Draw(const FRGraphDrawEdgeOption& Op, RGraphRef_t<UREdge> Edge, FDrawWork& Work)
{
    if (Work.VisitedEdges.Contains(Edge))
        return;

    Work.VisitedEdges.Add(Edge);

    if (Op.bVisible) {

        auto TypeMask = Edge->GetTypeMaskOrDefault(Op.bUseAnyFaceVertexColor);
        if (TypeMask != ERRoadTypeMask::Road)
            TypeMask &= ~ERRoadTypeMask::Road;

        if( (Op.ShowTypeMask & (int32)TypeMask) != 0)
        {
            if (Op.bShowNeighborCount) {
                const FVector MidPoint = (Edge->GetV0()->GetPosition() + Edge->GetV1()->GetPosition()) * 0.5f;
                TSet<UPLATEAUCityObjectGroup*> NeighborCityObjectGroups;
                for (auto&& Face : Edge->GetFaces()) {
                    if (Face->GetCityObjectGroup().IsValid())
                        NeighborCityObjectGroups.Add(Face->GetCityObjectGroup().Get());
                }
                if (NeighborCityObjectGroups.Num() <= 1)
                    return;
                //FPLATEAURnDebugEx::DrawString(FString::Printf(TEXT("%d"), Edge->GetFaces().Num()), MidPoint);
            }

            const FLinearColor Color = GetColor(TypeMask);
            FPLATEAURnDebugEx::DrawLine(Edge->GetV0()->GetPosition(), Edge->GetV1()->GetPosition(), Color);

            if (EnumHasAnyFlags(ShowId, ERPartsFlag::Edge)) {
                const FVector MidPoint = (Edge->GetV0()->GetPosition() + Edge->GetV1()->GetPosition()) * 0.5f;
                FPLATEAURnDebugEx::DrawString(FString::Printf(TEXT("E[%s]"), *Edge->GetName()), MidPoint);
            }
        }
    }

    Draw(VertexOption, Edge->GetV0(), Work);
    Draw(VertexOption, Edge->GetV1(), Work);
}

void UPLATEAURGraph::Draw(const FRGraphDrawFaceOption& Op, RGraphRef_t<URFace> Face, FDrawWork& Work)
{
    if (!Op.bVisible)
        return;

    if (!EnumHasAnyFlags(Face->GetRoadTypes(), (ERRoadTypeMask) ShowFaceType))
        return;

    if (EnumHasAnyFlags(Face->GetRoadTypes(), (ERRoadTypeMask)RemoveFaceType))
        return;

    if (Work.VisitedFaces.Contains(Face))
        return;

    Work.VisitedFaces.Add(Face);

    if(Op.bShowOnlyTargets)
    {
        if (Op.ShowTargetNames.Contains(Face->GetName()) == false)
            return;
    }

    TArray<URVertex*> Vertices;
    if (Op.bShowCityObjectOutline) 
    {
        Vertices = FRGraphEx::ComputeOutlineVerticesByCityObjectGroup(Work.Graph, Face->GetCityObjectGroup().Get(),
            Op.ShowOutlineMask, Op.ShowOutlineRemoveMask);
    }
    else {
        Vertices = FRGraphEx::ComputeOutlineVertices(Face);
    }


    if (EnumHasAnyFlags(ShowId, ERPartsFlag::Face)) {
        FVector Center = FVector::ZeroVector;
        for (const auto& Vertex : Vertices) {
            Center += Vertex->GetPosition();
        }
        Center /= Vertices.Num();
        FPLATEAURnDebugEx::DrawString(FString::Printf(TEXT("F[%s]"), *Face->GetName()), Center, FLinearColor::White, 0.f, FontScale );
    }

    if (Op.bShowOutline) {
        for(auto i = 0; i < Vertices.Num(); ++i)
        {
            auto V0 = Vertices[i];
            auto V1 = Vertices[(i + 1) % Vertices.Num()];

            auto Mask = V0->GetTypeMaskOrDefault(false) | V1->GetTypeMaskOrDefault(false);
            const FLinearColor Color = GetColor(Mask);
            FPLATEAURnDebugEx::DrawLine(V0->GetPosition(), V1->GetPosition(), Color);
        }
    }
    else {
        for (const auto& Edge : Face->GetEdges()) {
            Draw(EdgeOption, Edge, Work);
        }
    }
}
void UPLATEAURGraph::DrawSideWalk(URGraph* Graph, FDrawWork& Work) {
    auto DrawEdges = [this](const TArray<UREdge*>& Edges, const FPLATEAURnDrawOption& Option) {
        if (!Option.bVisible)
            return;

        for (const auto& Edge : Edges) {
            FPLATEAURnDebugEx::DrawLine(
                Edge->GetV0()->GetPosition(),
                Edge->GetV1()->GetPosition(),
                Option.Color
            );
        }
        };
    if (SideWalkOption.bVisible == false)
        return;
    for (const auto& Face : Graph->GetFaces()) 
    {
        auto RoadType = Face->GetRoadTypes();
        if (!FRRoadTypeMaskEx::IsSideWalk(RoadType))
            continue;

        TArray<UREdge*> OutsideEdges, InsideEdges, StartEdges, EndEdges;
        if (!FRGraphEx::CreateSideWalk(Face, OutsideEdges, InsideEdges, StartEdges, EndEdges))
            continue;

        DrawEdges(OutsideEdges, SideWalkOption.OutsideWay);
        DrawEdges(InsideEdges, SideWalkOption.InsideWay);
        DrawEdges(StartEdges, SideWalkOption.StartEdgeWay);
        DrawEdges(EndEdges, SideWalkOption.EndEdgeWay);
    }
}

void UPLATEAURGraph::DrawNormal(URGraph* Graph, FDrawWork& Work)
{
    for (const auto& Face : Graph->GetFaces()) {
        Draw(FaceOption, Face, Work);
    }
}

void UPLATEAURGraph::TickComponent(float DeltaTime, ELevelTick TickType,
                                   FActorComponentTickFunction* ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (IsVisible() == false)
        return;
    if (bVisibility == false)
        return;
    if (!RGraph)
        return;

    FDrawWork Work;
    Work.Graph = RGraph;

    if (bShowNormal) 
    {
        DrawNormal(RGraph, Work);
    }
    else
    {
        DrawSideWalk(RGraph, Work);
    }
}
