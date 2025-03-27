// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "RoadNetwork/CityObject/PLATEAUSubDividedCityObjectGroup.h"

#include "RoadNetwork/Util/PLATEAURnDebugEx.h"
#include "Component/PLATEAUCityObjectGroup.h"
#include "RoadNetwork/Util/PLATEAURnEx.h"

UPLATEAUSubDividedCityObjectGroup::UPLATEAUSubDividedCityObjectGroup()
{
    MeshColorNum = 16;
    bShowVertexIndex = false;
    bShowOutline = false;
    ShowVertexIndexFontSize = 8;
}

void UPLATEAUSubDividedCityObjectGroup::DrawMesh(const FSubDividedCityObjectMesh& Mesh,
    const FSubDividedCityObjectSubMesh& SubMesh, const FMatrix& Mat, const FLinearColor& Color, float Duration) {
    for (int32 j = 0; j < SubMesh.Triangles.Num(); j += 3) {
        FVector V0 = Mat.TransformPosition(Mesh.Vertices[SubMesh.Triangles[j]]);
        FVector V1 = Mat.TransformPosition(Mesh.Vertices[SubMesh.Triangles[j + 1]]);
        FVector V2 = Mat.TransformPosition(Mesh.Vertices[SubMesh.Triangles[j + 2]]);

        TArray<FVector> Vertices = { V0, V1, V2 };
        FPLATEAURnDebugEx::DrawLines(Vertices, true, Color, Duration);
    }
}

void UPLATEAUSubDividedCityObjectGroup::DrawCityObjects() {
    if (!IsVisible())
        return;
    TArray<USceneComponent*> Children;
    GetChildrenComponents(false, Children);
    int32 Index = 0;
    for (const auto& Child : Children)
    {
        auto Co = Cast<UPLATEAUSubDividedCityObject>(Child);
        if (Co == nullptr)
            continue;
        auto& Item =  Co->CityObject;
        bool bCoVisible = true; // Implement visibility check based on selection

        if (!bCoVisible) {
            Index += Item.Meshes.Num();
            continue;
        }

        for (const auto& Mesh : Item.Meshes) {
            if (bShowOutline) {
                for (const auto& SubMesh : Mesh.SubMeshes) {
                    auto PolyIndices = SubMesh.CreateOutlineIndices();
                    for (const auto& Indices : PolyIndices) {
                        for (int32 i = 0; i < Indices.Num(); i++) {
                            FVector V0 = Mesh.Vertices[Indices[i]];
                            FVector V1 = Mesh.Vertices[Indices[(i + 1) % Indices.Num()]];

                            FPLATEAURnDebugEx::DrawLine(V0, V1,
                                FPLATEAURnDebugEx::GetDebugColor(Index, MeshColorNum));
                        }
                    }
                    Index++;
                }
                continue;
            }

            if (bShowVertexIndex) {
                for (int32 i = 0; i < Mesh.Vertices.Num(); ++i) {
                    // Implement vertex index visualization if needed
                }
            }

            for (const auto& SubMesh : Mesh.SubMeshes) {
                FMatrix Mat = Item.CityObjectGroup->GetComponentTransform().ToMatrixWithScale();
                DrawMesh(Mesh, SubMesh, Mat,
                    FPLATEAURnDebugEx::GetDebugColor(Index, MeshColorNum));
                Index++;
            }
        }
    }
}

void UPLATEAUSubDividedCityObjectGroup::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if(bVisibility)
        DrawCityObjects();
}

TArray<UPLATEAUSubDividedCityObject*> UPLATEAUSubDividedCityObjectGroup::GetCityObjects()
{
    return FPLATEAURnEx::GetChildrenComponents<UPLATEAUSubDividedCityObject>(this, true, false);
}
