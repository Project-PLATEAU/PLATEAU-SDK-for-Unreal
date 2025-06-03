// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManagerGeneric.h"
#include "PLATEAUCityModelLoader.h"
#include "Import/PLATEAUImportModelBtn.h"
#include "Import/PLATEAUImportAreaSelectBtn.h"
#include "PLATEAUInstancedCityModel.h"
#include "PLATEAUEditor/Public/PLATEAUEditor.h"
#include "PLATEAUEditor/Public/ExtentEditor/PLATEAUExtentEditor.h"
#include <plateau/dataset/dataset_source.h>
#include <PLATEAURuntime.h>
#include <FileHelpers.h>
#include "PLATEAUAutomationTestUtil.h"

/*　連続テストする際は、 Project Settings / Hardwar Ray Tracing / Support Hardwar Ray Tracing のチェックを外す必要あり
*/

class FPLATEAUAutomationTestBase : public FAutomationTestBase {
    FString MyTestName;

    static FString GetTestDataPath() {
        return FPLATEAURuntimeModule::GetContentDir().Append("/TestData/data");
    }

    bool WriteToFile(const FString& Path, const FString& Text) const {
        const FString& DirectoryPath = FPaths::GetPath(Path);
        if (!FPaths::DirectoryExists(DirectoryPath)) {
            FFileManagerGeneric::Get().MakeDirectory(*DirectoryPath, true);
        }

        return FFileHelper::SaveStringToFile(Text, *(DirectoryPath + "/" + FPaths::GetBaseFilename(Path) + ".txt"));
    }

    static APLATEAUCityModelLoader* GetLocalCityModelLoader(const int ZoneId, const FVector& ReferencePoint, const int64 PackageMask, const FString& SourcePath, const TMap<int64, FPackageInfoSettings>& PackageInfoSettingsData) {
        const auto& ExtentEditor = IPLATEAUEditorModule::Get().GetExtentEditor();
        ExtentEditor->SetImportFromServer(false);
        ExtentEditor->SetSourcePath(SourcePath);
        ExtentEditor->SetLocalPackageMask(static_cast<plateau::dataset::PredefinedCityModelPackage>(PackageMask));
    
        const auto DatasetSource = plateau::dataset::DatasetSource::createLocal(TCHAR_TO_UTF8(*SourcePath));
        const std::shared_ptr<plateau::dataset::IDatasetAccessor> DatasetAccessor = DatasetSource.getAccessor();
        if (DatasetAccessor == nullptr || DatasetAccessor->getGridCodes().size() == 0)
            return nullptr;
    
        const plateau::geometry::GeoReference RawGeoReference(ZoneId, {}, 1, plateau::geometry::CoordinateSystem::ESU);
        ExtentEditor->SetGeoReference(RawGeoReference);

        const auto& GridCodes = DatasetAccessor->getGridCodes();
        auto GeoReference = ExtentEditor->GetGeoReference();
        TArray<FPLATEAUGridCodeGizmo> MeshCodeGizmos;
        TArray<bool> bSelectedArray;
        MeshCodeGizmos.Reset();
        for (const auto& GridCode : GridCodes) {
            MeshCodeGizmos.AddDefaulted();
            MeshCodeGizmos.Last().Init(GridCode, GeoReference.GetData());
            if ("53392642" != MeshCodeGizmos.Last().GetRegionGridCodeID())
                continue;
            
            bSelectedArray.Reset();
            for (int i = 0; i < MeshCodeGizmos.Last().GetbSelectedArray().Num(); i++) {
                bSelectedArray.Emplace(true);
            }
            MeshCodeGizmos.Last().SetbSelectedArray(bSelectedArray);
            IPLATEAUEditorModule::Get().GetExtentEditor()->SetGridCodeMap(TCHAR_TO_UTF8(GridCode->get().c_str()), MeshCodeGizmos.Last());
        }
        
        return UPLATEAUImportModelBtn::GetCityModelLoader(ZoneId, ReferencePoint, PackageInfoSettingsData, false);
    }
public:
	FPLATEAUAutomationTestBase(const FString& InName, const bool bInComplexTask)
		: FAutomationTestBase(InName, bInComplexTask) {
	}
protected:
    void InitializeTest(const FString& TargetTestName) {
        MyTestName = TargetTestName;
        const FString TestLogPath = FPaths::ProjectDir().Append("TestLogs/" + MyTestName + ".log");
        if (FPaths::FileExists(TestLogPath)) {
            if (!FFileManagerGeneric::Get().Delete(*TestLogPath, true, true)) AddError("Failed to DeleteFile");
        }
    }
    
    UWorld* GetWorld() {
        for (auto WorldContext : GEngine->GetWorldContexts()) {
            if (WorldContext.World() != nullptr) {
                return WorldContext.World();
            }
        }
        AddError("World == nullptr");
        return nullptr;
    }
    
    bool OpenNewMap() const {
        const FString TemplateMap = FPaths::EngineContentDir().Append("Maps/Templates/Template_Default.umap");
        return FEditorFileUtils::LoadMap(TemplateMap, true, true);
    }

    bool OpenMap(FString MapName) const {
        const FString Map = FPLATEAURuntimeModule::GetContentDir().Append("/TestData/umap/").Append(MapName).Append(".umap");
        return FEditorFileUtils::LoadMap(Map, false, true);
    }
    
    APLATEAUCityModelLoader* GetInstancedCityLoader(const UWorld& World) {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(&World, APLATEAUInstancedCityModel::StaticClass(), FoundActors);
        if (0 < FoundActors.Num()) {
            AddError(TEXT("0 < FoundActors.Num()"));
        } else {
            constexpr int ZoneId = 9;
            const FVector ReferencePoint = FVector(-472281.96875, 5131018, 0);
            constexpr int64 PackageMask = static_cast<int64>(plateau::dataset::PredefinedCityModelPackage::Building);
            const FString SourcePath = GetTestDataPath();
            const auto defaultMat = UPLATEAUImportAreaSelectBtn::GetDefaultFallbackMaterial(static_cast<int64>(plateau::dataset::PredefinedCityModelPackage::Building));
            const FPackageInfoSettings PackageInfoSettings(true, true, true, true, EPLATEAUTexturePackingResolution::H4096W4096, 0, 4, 1, defaultMat, false, "", 7);
            TMap<int64, FPackageInfoSettings> PackageInfoSettingsData;
            PackageInfoSettingsData.Add(static_cast<int64>(plateau::dataset::PredefinedCityModelPackage::Building), PackageInfoSettings);
            const auto& Loader = GetLocalCityModelLoader(ZoneId, ReferencePoint, PackageMask, SourcePath, PackageInfoSettingsData);
            if (Loader) {
                return Loader;
            }
        }
        
        AddError(TEXT("Loader is nullptr"));
        return nullptr;
    }

    void FinishTest(const bool bSuccess, const FString Message) {
        const FString TestLogPath = FPaths::ProjectDir().Append("TestLogs/" + MyTestName + ".log");
        if (!WriteToFile(TestLogPath, bSuccess ? "Succeeded" : FString::Format(TEXT("Failed: {0}"), {Message}))) AddError("Failed to WriteToFile");
    }
};
