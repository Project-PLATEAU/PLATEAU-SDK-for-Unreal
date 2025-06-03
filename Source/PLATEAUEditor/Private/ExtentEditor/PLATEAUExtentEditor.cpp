// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "ExtentEditor/PLATEAUExtentEditor.h"

#include "Algo/AnyOf.h"

#include "PLATEAUEditor.h"
#include "PLATEAUGridCodeGizmo.h"
#include "PLATEAUWindow.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/PLATEAUSDKEditorUtilityWidget.h"
#include "ExtentEditor/SPLATEAUExtentEditorViewport.h"


#define LOCTEXT_NAMESPACE "FPLATEUExtentEditor"

const FName FPLATEAUExtentEditor::TabId(TEXT("PLATEAUExtentEditor"));

void FPLATEAUExtentEditor::RegisterTabSpawner(const TSharedRef<class FTabManager>& InTabManager) {
    InTabManager->RegisterTabSpawner(TabId, FOnSpawnTab::CreateRaw(this, &FPLATEAUExtentEditor::SpawnTab), FCanSpawnTab::CreateRaw(this, &FPLATEAUExtentEditor::CanSpawnTab))
        .SetDisplayName(LOCTEXT("ViewportTab", "Viewport"))
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));
}

void FPLATEAUExtentEditor::UnregisterTabSpawner(const TSharedRef<class FTabManager>& InTabManager) {
    InTabManager->UnregisterTabSpawner(TabId);
}

FPLATEAUExtentEditor::FPLATEAUExtentEditor() {
    LocalAreaMeshCodeMap.Reset();
    ServerAreaMeshCodeMap.Reset();
}

FPLATEAUExtentEditor::~FPLATEAUExtentEditor() {}

TSharedRef<SDockTab> FPLATEAUExtentEditor::SpawnTab(const FSpawnTabArgs& Args) {
    TWeakPtr<FPLATEAUExtentEditor> WeakSharedThis(SharedThis(this));

    const auto Viewport = SNew(SPLATEAUExtentEditorViewport).ExtentEditor(WeakSharedThis);
    TSharedRef<SDockTab> DockableTab = SNew(SDockTab).TabRole(NomadTab)[Viewport];
    Viewport->SetOwnerTab(DockableTab);
    DockableTab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateLambda([](TSharedRef<SDockTab> DockTab) {
        const auto& Window = IPLATEAUEditorModule::Get().GetWindow();
        if (const auto& Euw = Window->GetEditorUtilityWidget(); Euw != nullptr) {
            if (const auto& PlateauEuw = dynamic_cast<UPLATEAUSDKEditorUtilityWidget*>(Euw); PlateauEuw != nullptr) {
                PlateauEuw->CloseAreaSelectionWindowInvoke();
            }            
        }
    }));

    return DockableTab;
}

bool FPLATEAUExtentEditor::CanSpawnTab(const FSpawnTabArgs& Args) const {
    const auto& UseSourcePath = IsImportFromServer() ? UTF8_TO_TCHAR(GetServerDatasetID().c_str()) : GetSourcePath();
    return 0 < UseSourcePath.Len();
}

const FString& FPLATEAUExtentEditor::GetSourcePath() const {
    return SourcePath;
}

void FPLATEAUExtentEditor::SetSourcePath(const FString& Path) {
    SourcePath = Path;
}

const FString& FPLATEAUExtentEditor::GetAreaSourcePath() const {
    return AreaSourcePath;
}

void FPLATEAUExtentEditor::SetAreaSourcePath(const FString& InAreaSourcePath) {
    AreaSourcePath = InAreaSourcePath;
}

bool FPLATEAUExtentEditor::IsSelectedArea() const {
    return Algo::AnyOf(GetGridCodeMap(), [](const TTuple<FString, FPLATEAUGridCodeGizmo>& MeshCodeGizmoTuple) {
        return MeshCodeGizmoTuple.Value.bSelectedArea();
    });
}

TArray<FString> FPLATEAUExtentEditor::GetSelectedCodes(const bool InbImportFromServer) const {
    TArray<FString> Codes;

    if (InbImportFromServer) {
        for (auto [_, Value] : ServerAreaMeshCodeMap) {
            if (Value.bSelectedArea()) {
                Codes.Append(Value.GetSelectedGridCodeIDs());
            }
        }
    } else {
        for (auto [_, Value] : LocalAreaMeshCodeMap) {
            if (Value.bSelectedArea()) {
                Codes.Append(Value.GetSelectedGridCodeIDs());
            }
        }
    }

    return Codes;
}

TMap<FString, FPLATEAUGridCodeGizmo> FPLATEAUExtentEditor::GetGridCodeMap() const {
    return IsImportFromServer() ? ServerAreaMeshCodeMap : LocalAreaMeshCodeMap;
}

void FPLATEAUExtentEditor::SetGridCodeMap(const FString& MeshCode, const FPLATEAUGridCodeGizmo& MeshCodeGizmo) {
    if (IsImportFromServer()) {
        ServerAreaMeshCodeMap.Emplace(MeshCode, MeshCodeGizmo);
    } else {
        LocalAreaMeshCodeMap.Emplace(MeshCode, MeshCodeGizmo);
    }
}

void FPLATEAUExtentEditor::ResetGridCodeMap() {
    if (IsImportFromServer()) {
        ServerAreaMeshCodeMap.Reset();
    } else {
        LocalAreaMeshCodeMap.Reset();
    }
}

FPLATEAUGeoReference FPLATEAUExtentEditor::GetGeoReference() const {
    return GeoReference;
}

void FPLATEAUExtentEditor::SetGeoReference(const FPLATEAUGeoReference& InGeoReference) {
    GeoReference = InGeoReference;
}

bool FPLATEAUExtentEditor::IsImportFromServer() const {
    return bImportFromServer;
}

void FPLATEAUExtentEditor::SetImportFromServer(const bool InbImportFromServer) {
    bImportFromServer = InbImportFromServer;
}

std::shared_ptr<plateau::network::Client> FPLATEAUExtentEditor::GetClientPtr() const {
    return ClientPtr;
}

void FPLATEAUExtentEditor::SetClientPtr(const std::shared_ptr<plateau::network::Client>& InClientPtr) {
    ClientPtr = InClientPtr;
}

const std::string& FPLATEAUExtentEditor::GetServerDatasetID() const {
    return ServerDatasetID;
}

void FPLATEAUExtentEditor::SetServerDatasetID(const std::string& InID) {
    ServerDatasetID = InID;
}

const plateau::dataset::PredefinedCityModelPackage& FPLATEAUExtentEditor::GetLocalPackageMask() const {
    return LocalPackageMask;
}

void FPLATEAUExtentEditor::SetLocalPackageMask(const plateau::dataset::PredefinedCityModelPackage& InPackageMask) {
    LocalPackageMask = InPackageMask;
}

const plateau::dataset::PredefinedCityModelPackage& FPLATEAUExtentEditor::GetServerPackageMask() const {
    return ServerPackageMask;
}

plateau::geometry::GeoCoordinate FPLATEAUExtentEditor::GetSelectedCenterLatLon(const bool InbImportFromServer) const {
    const auto& SelectedCodes = GetSelectedCodes(InbImportFromServer);

    if (SelectedCodes.Num() == 0)
        return plateau::geometry::GeoCoordinate(0.0, 0.0, 0.0);

    // 選択された地域メッシュ全てを囲む範囲を計算
    plateau::geometry::Extent NativeExtent(
        plateau::geometry::GeoCoordinate(180.0, 180.0, 0.0),
        plateau::geometry::GeoCoordinate(-180.0, -180.0, 0.0));

    for (const auto& Code : SelectedCodes) {
        const auto NativePartialExtent = plateau::dataset::GridCode::create(TCHAR_TO_UTF8(*Code))->getExtent();
        NativeExtent.min.latitude = std::min(NativeExtent.min.latitude, NativePartialExtent.min.latitude);
        NativeExtent.min.longitude = std::min(NativeExtent.min.longitude, NativePartialExtent.min.longitude);
        NativeExtent.max.latitude = std::max(NativeExtent.max.latitude, NativePartialExtent.max.latitude);
        NativeExtent.max.longitude = std::max(NativeExtent.max.longitude, NativePartialExtent.max.longitude);
    }
    return NativeExtent.centerPoint();
}

FVector FPLATEAUExtentEditor::GetSelectedCenterPoint(const int InZoneID, const bool InbImportFromServer) const {
    // 中心点の緯度経度計算
    const auto CenterLatLon = GetSelectedCenterLatLon(InbImportFromServer);

    // 平面直角座標系への変換
    auto GeoReferenceWithoutOffset = FPLATEAUGeoReference();
    GeoReferenceWithoutOffset.ZoneID = InZoneID;
    GeoReferenceWithoutOffset.UpdateNativeData();

    const auto CenterPoint = GeoReferenceWithoutOffset.GetData().project(CenterLatLon);
    return FVector(CenterPoint.x, CenterPoint.y, CenterPoint.z);
}

void FPLATEAUExtentEditor::SetServerPackageMask(const plateau::dataset::PredefinedCityModelPackage& InPackageMask) {
    ServerPackageMask = InPackageMask;
}

const FVector3d FPLATEAUExtentEditor::GetCenterByExtent(const plateau::geometry::Extent Extent) const {
    const auto CenterLatLon = Extent.centerPoint();
    auto GeoRef = GetGeoReference();
    const auto CenterPoint = GeoRef.GetData().project(CenterLatLon);
    return FVector3d(CenterPoint.x, CenterPoint.y, CenterPoint.z);
}

const FBox FPLATEAUExtentEditor::GetBoxByExtent(const plateau::geometry::Extent Extent) const {
    auto GeoRef = GetGeoReference();
    const auto Min = GeoRef.GetData().project(Extent.min);
    const auto Max = GeoRef.GetData().project(Extent.max);
    return FBox(FVector3d(Min.x, Min.y, Min.z), FVector3d(Max.x, Max.y, Max.z));
}

#undef LOCTEXT_NAMESPACE
