// Copyright © 2023 Ministry of Land, Infrastructure and Transport


#include "PLATEAUGridCodeGizmo.h"

#include "SceneManagement.h"

#include <plateau/dataset/mesh_code.h>
#include <plateau/geometry/geo_reference.h>

#include "CanvasTypes.h"
#include "Algo/AnyOf.h"
#include "Engine/Font.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace {

    int GetNumAreaColumnByGridCode(const std::shared_ptr<plateau::dataset::GridCode>& GridCode, const EGridCodeGizmoType GridCodeType) {
        if (GridCodeType == EGridCodeGizmoType::StandardMapCode)
            return 1;
        return GridCode->isSmallerThanNormalGml() ? 2 : 4;
    }

    int GetNumAreaRowByGridCode(const std::shared_ptr<plateau::dataset::GridCode>& GridCode, const EGridCodeGizmoType GridCodeType) {
        if (GridCodeType == EGridCodeGizmoType::StandardMapCode)
            return 1;
        return GridCode->isSmallerThanNormalGml() ? 2 : 4;
    }

    const TArray<FString> SuffixMeshIds = {
        TEXT("11"), TEXT("12"), TEXT("21"), TEXT("22"),
        TEXT("13"), TEXT("14"), TEXT("23"), TEXT("24"),
        TEXT("31"), TEXT("32"), TEXT("41"), TEXT("42"),
        TEXT("33"), TEXT("34"), TEXT("43"), TEXT("44")};

    // 選択色
    constexpr FColor SelectedColor = FColor(255, 204, 153);
    constexpr FColor UnselectedColor = FColor(0, 0, 0, 0);
    constexpr FColor SelectedStandardMapColor = FColor(255, 250, 203);

    int GetRowIndex(const double InMinX, const double InMaxX, const int InNumGrid, const double InValue) {
        const double GridSize = (InMaxX - InMinX) / InNumGrid;
        for (int i = 0; i < InNumGrid; i++) {
            if (InValue <= InMinX + GridSize * (i + 1)) {
                return i;
            }
        }
        return InNumGrid - 1;
    }

    int GetColumnIndex(const double InMinY, const double InMaxY, const int InNumGrid, const double InValue) {
        const double GridSize = (InMaxY - InMinY) / InNumGrid;
        for (int i = 0; i < InNumGrid; i++) {
            if (InMaxY - GridSize * (i + 1) <= InValue) {
                return i;
            }
        }
        return InNumGrid - 1;
    }
}

FPLATEAUGridCodeGizmo::FPLATEAUGridCodeGizmo() : GridCode(), Width(0), Height(0), MinX(-500), MinY(-500), MaxX(500), MaxY(500), LineThickness(1.0f) {
    AreaSelectedMaterial = DuplicateObject(GEngine->ConstraintLimitMaterialPrismatic, nullptr);
    AreaSelectedMaterial.Get()->SetScalarParameterValue(FName("Desaturation"), 1.0f);
    AreaSelectedMaterial.Get()->SetVectorParameterValue(FName("Color"), SelectedColor);
    AreaUnSelectedMaterial = DuplicateObject(GEngine->ConstraintLimitMaterialPrismatic, nullptr);
    AreaUnSelectedMaterial.Get()->SetScalarParameterValue(FName("Desaturation"), 1.0f);
    AreaUnSelectedMaterial.Get()->SetVectorParameterValue(FName("Color"), UnselectedColor);
    GridCodeType = EGridCodeGizmoType::MeshCode;
}

bool FPLATEAUGridCodeGizmo::IsSelectable() const {
    if (!ensure(GridCode))
    {
        return false;
    }
    return GridCode->isNormalGmlLevel() || GridCode->isSmallerThanNormalGml();
}

void FPLATEAUGridCodeGizmo::ResetSelectedArea() {
    for (int i = 0; i < bSelectedArray.Num(); i++) {
        bSelectedArray[i] = false;
    }
}

void FPLATEAUGridCodeGizmo::DrawExtent(const FSceneView* View, FPrimitiveDrawInterface* PDI) const {

    const FBox Box(FVector(MinX, MinY, 0), FVector(MaxX, MaxY, 0));
    constexpr FColor kMeshCodeGridColor(10, 10, 130);
    constexpr FColor kStandardMapGridColor(10, 130, 10);
    const auto Color = IsStandardMapGrid() ? kStandardMapGridColor : kMeshCodeGridColor;
    const int NumAreaColumn = GetNumAreaColumnByGridCode(GridCode, GridCodeType);
    const int NumAreaRow = GetNumAreaRowByGridCode(GridCode, GridCodeType);

    // エリア枠線
    DrawWireBox(PDI, Box, Color, SDPG_World, LineThickness, 0, true);

    if (!bShowLevel5Mesh)
        return;
   
    // 格子状のライン
    for (int i = 1; i <= NumAreaColumn -1; ++i) {
        const auto X1 = (Box.Min.X * i + Box.Max.X * (NumAreaColumn - i)) / NumAreaColumn;
        const auto Py1 = Box.Min.Y;
        const auto Qy1 = Box.Max.Y;
        constexpr auto Z = 0.0;
        const FVector P1(X1, Py1, Z);
        const FVector Q1(X1, Qy1, Z);
        PDI->DrawLine(P1, Q1, Color, SDPG_World, 1, 0, true);
    }

    for (int i = 1; i <= NumAreaRow - 1; ++i) {
        const auto Y2 = (Box.Min.Y * i + Box.Max.Y * (NumAreaRow - i)) / NumAreaRow;
        const auto Px2 = Box.Min.X;
        const auto Qx2 = Box.Max.X;
        constexpr auto Z = 0.0;
        const FVector P2(Px2, Y2, Z);
        const FVector Q2(Qx2, Y2, Z);
        PDI->DrawLine(P2, Q2, Color, SDPG_World, 1, 0, true);
    }

    // エリア塗りつぶし
    TArray<FMatrix> CellMatrices;
    GetCellMatrices(CellMatrices, true);
    for (const auto& CellMatrix : CellMatrices) {
        DrawPlane10x10(PDI, CellMatrix, 1.0f, FVector2D::Zero(), FVector2D::One(), AreaSelectedMaterial->GetRenderProxy(), SDPG_Foreground);
    }
}

void FPLATEAUGridCodeGizmo::DrawRegionGridCodeID(const FViewport& InViewport, const FSceneView& View, FCanvas& Canvas, const FString& GridCodeID,
                                             double CameraDistance, int IconCount) const {
    if (GridCode->isSmallerThanNormalGml()) return;
    
    const auto CenterX = MinX + (MaxX - MinX) / 2;
    const auto Coef = 4 < IconCount ? 1.52 : 1.28;
    const auto CenterY = MinY + (MaxY - MinY) / 2 * Coef;

    const auto dpi = Canvas.GetDPIScale();
    const auto ViewPlane = View.Project(FVector(CenterX, CenterY, 0));

    const auto HalfX = InViewport.GetSizeXY().X / 2 / dpi;
    const auto HalfY = InViewport.GetSizeXY().Y / 2 / dpi;

    const auto XPos = HalfX + (HalfX * ViewPlane.X);
    const auto YPos = HalfY + (HalfY * -ViewPlane.Y);

    const UFont* ViewFont = GEngine->GetLargeFont();

    const auto HalfWidth = ViewFont->GetStringSize(*GridCodeID) / 2;
    const auto HalfHeight = ViewFont->GetStringHeightSize(*GridCodeID) / 2;

    const auto Color = IsStandardMapGrid() ? FColor::Green : FColor::Blue;

    if (ViewPlane.W > 0.f) {
        if (CameraDistance < plateau::geometry::ShowGridCodeIdCameraDistance) {
            Canvas.DrawShadowedText(XPos - HalfWidth, YPos - HalfHeight, FText::FromString(GridCodeID), ViewFont, Color);
        }
    }
}

FVector2D FPLATEAUGridCodeGizmo::GetMin() const {
    return FVector2D(MinX, MinY);
}

FVector2D FPLATEAUGridCodeGizmo::GetMax() const {
    return FVector2D(MaxX, MaxY);
}

FVector2D FPLATEAUGridCodeGizmo::GetSize() const {
    return FVector2D(Width, Height);
}

std::shared_ptr<plateau::dataset::GridCode> FPLATEAUGridCodeGizmo::GetGridCode() const {
    return GridCode;
}

FString FPLATEAUGridCodeGizmo::GetRegionGridCodeID() const {
    return GridCodeString;
}

TArray<bool> FPLATEAUGridCodeGizmo::GetbSelectedArray() const {
    return bSelectedArray;
}

bool FPLATEAUGridCodeGizmo::bSelectedArea() const {
    return Algo::AnyOf(bSelectedArray, [](const bool bSelected) {
        return bSelected;
    });
}

void FPLATEAUGridCodeGizmo::SetbSelectedArray(const TArray<bool>& InbSelectedArray) {
    bSelectedArray = InbSelectedArray;
}

EGridCodeGizmoType FPLATEAUGridCodeGizmo::GetGridCodeType() const {
    return GridCodeType;
}

void FPLATEAUGridCodeGizmo::Init(const std::shared_ptr<plateau::dataset::GridCode>& InGridCode, const plateau::geometry::GeoReference& InGeoReference) {
    const auto Extent = InGridCode->getExtent();
    const auto RawMin = InGeoReference.project(Extent.min);
    const auto RawMax = InGeoReference.project(Extent.max);
    GridCode = InGridCode;
    GridCodeString = UTF8_TO_TCHAR(InGridCode->get().c_str());
    GridCodeType = GetGridCodeTypeByGridCodeString(GridCodeString);
    MinX = FGenericPlatformMath::Min(RawMin.x, RawMax.x);
    MinY = FGenericPlatformMath::Min(RawMin.y, RawMax.y);
    MaxX = FGenericPlatformMath::Max(RawMin.x, RawMax.x);
    MaxY = FGenericPlatformMath::Max(RawMin.y, RawMax.y);
    Width = MaxX - MinX;
    Height = MaxY - MinY;
    LineThickness = 2.0f;

    const int NumAreaColumn = GetNumAreaColumnByGridCode(GridCode, GridCodeType);
    const int NumAreaRow = GetNumAreaRowByGridCode(GridCode, GridCodeType);
    bSelectedArray.Reset();
    for (int i = 0; i < NumAreaRow * NumAreaColumn; i++) {
        bSelectedArray.Emplace(false);
    }

    if(IsStandardMapGrid())
        AreaSelectedMaterial.Get()->SetVectorParameterValue(FName("Color"), SelectedStandardMapColor);
}

void FPLATEAUGridCodeGizmo::ToggleSelectArea(const double X, const double Y) {

    if (!IsSelectable()) 
        return;

    if ((MinX <= X && X <= MaxX && MinY <= Y && Y <= MaxY) == false) {
        return;
    }

    int NumAreaColumn = GetNumAreaColumnByGridCode(GridCode, GridCodeType);
    int NumAreaRow = GetNumAreaRowByGridCode(GridCode, GridCodeType);
    const auto RowIndex = GetRowIndex(MinX, MaxX, NumAreaRow, X);
    const auto ColumnIndex = GetColumnIndex(MinY, MaxY, NumAreaColumn, Y);
    bSelectedArray[RowIndex + ColumnIndex * NumAreaColumn] = IsStandardMapGrid() ? true : !bSelectedArray[RowIndex + ColumnIndex * NumAreaColumn];
}

void FPLATEAUGridCodeGizmo::SetSelectArea(const FVector2d InMin, const FVector2d InMax, const bool bSelect) {

    if (!IsSelectable()) 
        return;

    const int NumAreaColumn = GetNumAreaColumnByGridCode(GridCode, GridCodeType);
    const int NumAreaRow = GetNumAreaRowByGridCode(GridCode, GridCodeType);
    const auto CellWidth = (MaxX - MinX) / NumAreaRow;
    const auto CellHeight = (MaxY - MinY) / NumAreaColumn;
    for (int Col = 0; Col < NumAreaColumn; Col++) {
        for (int Row = 0; Row < NumAreaRow; Row++) {
            const auto RectMinX = MinX + CellWidth * Row;
            const auto RectMaxX = MinX + CellWidth * (Row + 1);
            const auto RectMinY = MaxY - CellHeight * (Col + 1);
            const auto RectMaxY = MaxY - CellHeight * Col;
            if (RectMaxX < InMin.X || RectMinX > InMax.X || RectMaxY < InMin.Y || RectMinY > InMax.Y) {
                continue;
            }
            bSelectedArray[Row + Col * NumAreaColumn] = bSelect;
        }
    }
}

void FPLATEAUGridCodeGizmo::SetSelectArea(const double X, const double Y, const bool bSelect) {

    if (!IsSelectable()) 
        return;
    
    if ((MinX <= X && X <= MaxX && MinY <= Y && Y <= MaxY) == false) {
        return;
    }

    const int NumAreaColumn = GetNumAreaColumnByGridCode(GridCode, GridCodeType);
    const int NumAreaRow = GetNumAreaRowByGridCode(GridCode, GridCodeType);
    const auto RowIndex = GetRowIndex(MinX, MaxX, NumAreaRow, X);
    const auto ColumnIndex = GetColumnIndex(MinY, MaxY, NumAreaColumn, Y);
    bSelectedArray[RowIndex + ColumnIndex * NumAreaColumn] = bSelect;
}

void FPLATEAUGridCodeGizmo::GetCellMatrices(TArray<FMatrix>& OutMatrices, const bool bSelectedOnly) const {
    const FBox Box(FVector(MinX, MinY, 0), FVector(MaxX, MaxY, 0));
    const int NumAreaColumn = GetNumAreaColumnByGridCode(GridCode, GridCodeType);
    const int NumAreaRow = GetNumAreaRowByGridCode(GridCode, GridCodeType);

    const auto CellWidth = (Box.Max.X - Box.Min.X) / NumAreaRow;
    const auto CellHalfWidth = (Box.Max.X - Box.Min.X) / (NumAreaRow * 2);
    const auto CellHeight = (Box.Max.Y - Box.Min.Y) / NumAreaColumn;
    const auto CellHalfHeight = (Box.Max.Y - Box.Min.Y) / (NumAreaColumn * 2);

    TArray<FMatrix> CellMatrices;
    for (int Col = 0; Col < NumAreaColumn; Col++) {
        for (int Row = 0; Row < NumAreaRow; Row++) {
            if (!bSelectedOnly || bSelectedArray[Row + Col * NumAreaColumn]) {
                FMatrix CellMatrix = FMatrix::Identity;
                CellMatrix.SetAxis(0, FVector(CellHalfWidth, 0, 0));
                CellMatrix.SetAxis(1, FVector(0, CellHalfHeight, 0));

                //Level4は1Gridずらす / 国土基本図郭は1.5Gridずらす
                float AdjustedRow = IsStandardMapGrid() ? Row + 1.5 : GridCode->isSmallerThanNormalGml() ? Row + 1 : Row;
                float AdjustedCol = IsStandardMapGrid() ? Col + 1.5 : GridCode->isSmallerThanNormalGml() ? Col + 1 : Col;
                CellMatrix.SetOrigin(FVector((Box.Min.X + Box.Max.X) / 2 - CellHalfWidth - CellWidth + CellWidth * AdjustedRow,
                    (Box.Min.Y + Box.Max.Y) / 2 + CellHalfHeight + CellHeight - CellHeight * AdjustedCol, 0));

                CellMatrices.Add(CellMatrix);
            }
        }
    }
    OutMatrices = CellMatrices;
}

void FPLATEAUGridCodeGizmo::GetCellBoxes(TArray<FBox>& OutBoxes, const bool bSelectedOnly) const {
    TArray<FBox> CellBoxes;
    const auto DefaultBox = FBox(FVector(-1, -1, 0), FVector(1, 1, 0));

    TArray<FMatrix> CellMatrices;
    GetCellMatrices(CellMatrices, bSelectedOnly);

    for (const auto& CellMatrix : CellMatrices) {
        // MatrixをBoxに変換    
        FBox CellBox = DefaultBox.TransformBy(CellMatrix);
        CellBoxes.Add(CellBox);
    }
    OutBoxes =  CellBoxes;
}

void FPLATEAUGridCodeGizmo::GetSelectedBoxes(TArray<FBox>& OutBoxes) const {
    GetCellBoxes(OutBoxes, true);
}

void FPLATEAUGridCodeGizmo::SetOverlapSelection(const TArray<FBox>& InBoxes) {
    if (!IsSelectable())
        return;

    // 一旦選択を解除
    ResetSelectedArea();

    TArray<FBox> CellBoxes;
    GetCellBoxes(CellBoxes, false);

    // Box同志の重なりを調べて重なっているCellを選択状態にする
    for (const auto& InBox : InBoxes) {    
        for (int i = 0; i < CellBoxes.Num(); i++) {
            if (CellBoxes[i].IntersectXY(InBox)) {
                bSelectedArray[i] = true;
            }
        }
    }
}

TArray<FString> FPLATEAUGridCodeGizmo::GetSelectedGridCodeIDs() {

    const int NumAreaColumn = GetNumAreaColumnByGridCode(GridCode, GridCodeType);
    const int NumAreaRow = GetNumAreaRowByGridCode(GridCode, GridCodeType);
    TSet<FString> GridCodeIDArray;

    if (bSelectedArray.Num() < SuffixMeshIds.Num()) { //Level4
        for (int i = 0; i < bSelectedArray.Num(); i++) {
            if (bSelectedArray[i]){
                if (IsStandardMapGrid()) {
                    GridCodeIDArray.Emplace(GridCodeString);
                }
                else {
                    GridCodeIDArray.Emplace(FString::Format(TEXT("{0}{1}"), { GridCodeString, i + 1 }));
                }
            }
        }
    } 
    else {
        for (int Col = 0; Col < NumAreaColumn; Col++) {
            for (int Row = 0; Row < NumAreaRow; Row++) {
                if (bSelectedArray[Row + Col * NumAreaColumn]) {
                    if (IsStandardMapGrid()) {
                        GridCodeIDArray.Emplace(GridCodeString);
                    }
                    else {
                        GridCodeIDArray.Emplace(FString::Format(TEXT("{0}{1}"), { GridCodeString, SuffixMeshIds[Row + Col * NumAreaColumn] }));
                    }
                }
            }
        }
    }
    return GridCodeIDArray.Array();
}

void FPLATEAUGridCodeGizmo::SetShowLevel5Mesh(const bool bValue) {
    bShowLevel5Mesh = bValue;
}

bool FPLATEAUGridCodeGizmo::IsStandardMapGrid() const {
    return GridCodeType == EGridCodeGizmoType::StandardMapCode;
}

EGridCodeGizmoType FPLATEAUGridCodeGizmo::GetGridCodeTypeByGridCodeString(const FString& GridCodeStr) const {
    if(!GridCodeStr.IsNumeric())   
        return EGridCodeGizmoType::StandardMapCode;
    return EGridCodeGizmoType::MeshCode;
}
