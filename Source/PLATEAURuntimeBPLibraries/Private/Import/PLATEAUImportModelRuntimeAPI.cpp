// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "Import/PLATEAUImportModelRuntimeAPI.h"
#include "PLATEAUCityModelLoader.h"
#include "plateau/dataset/dataset_source.h"
#include "Engine/World.h"
#include "Misc/MessageDialog.h"

namespace {

    APLATEAUCityModelLoader* GetCityModelLoader(UWorld* World, const TArray<FString> MeshCodes, const int ZoneID, const FVector& ReferencePoint, const TMap<EPLATEAUCityModelPackage, FPackageInfoSettings>& PackageInfoSettingsData, plateau::dataset::DatasetSource InDatasetSource) {
        FActorSpawnParameters SpawnParam;
        const auto Actor = World->SpawnActor<APLATEAUCityModelLoader>(SpawnParam);
        const auto Loader = Cast<APLATEAUCityModelLoader>(Actor);
        Loader->MeshCodes = MeshCodes;
        Loader->GeoReference.ZoneID = ZoneID;
        Loader->GeoReference.ReferencePoint = ReferencePoint;
        Loader->GeoReference.UpdateNativeData();

        std::vector<plateau::dataset::MeshCode> NativeSelectedMeshCodes;
        for (const auto& Code : MeshCodes) {
            NativeSelectedMeshCodes.emplace_back(TCHAR_TO_UTF8(*Code));
        }
        const auto FilteredDatasetAccessor = InDatasetSource.getAccessor()->filterByMeshCodes(NativeSelectedMeshCodes);
        const auto PackageMask = FilteredDatasetAccessor->getPackages();
        const auto ImportSettings = DuplicateObject(GetMutableDefault<UPLATEAUImportSettings>(), Loader);
        for (const auto& Package : UPLATEAUImportSettings::GetAllPackages()) {
            if ((Package & PackageMask) == plateau::dataset::PredefinedCityModelPackage::None) continue;
            if (!PackageInfoSettingsData.Contains(UPLATEAUImportSettings::GetPLATEAUCityModelPackageFromPredefinedCityModelPackage(Package))) {
                auto& Feature = ImportSettings->GetFeatureSettingsRef(Package);
                Feature.bImport = false;
                continue;
            }

            const auto& PackageInfoSettings = PackageInfoSettingsData[UPLATEAUImportSettings::GetPLATEAUCityModelPackageFromPredefinedCityModelPackage(Package)];
            auto& Feature = ImportSettings->GetFeatureSettingsRef(Package);
            Feature.bImport = PackageInfoSettings.bImport;
            Feature.bImportTexture = PackageInfoSettings.bTextureImport;
            Feature.bIncludeAttrInfo = PackageInfoSettings.bIncludeAttrInfo;
            Feature.bEnableTexturePacking = PackageInfoSettings.bEnableTexturePacking;
            Feature.TexturePackingResolution = PackageInfoSettings.TexturePackingResolution;
            Feature.MeshGranularity = static_cast<EPLATEAUMeshGranularity>(PackageInfoSettings.Granularity);
            Feature.MinLod = PackageInfoSettings.MinLod;
            Feature.MaxLod = PackageInfoSettings.MaxLod;
            Feature.FallbackMaterial = PackageInfoSettings.FallbackMaterial;
            if (Package == plateau::dataset::PredefinedCityModelPackage::Relief) {
                Feature.bAttachMapTile = PackageInfoSettings.bAttachMapTile;
                Feature.MapTileUrl = PackageInfoSettings.MapTileUrl;
                Feature.ZoomLevel = PackageInfoSettings.ZoomLevel;
            }
        }
        Loader->ImportSettings = ImportSettings;
        return Loader;
    }
}

APLATEAUCityModelLoader* UPLATEAUImportModelRuntimeAPI::GetCityModelLoaderLocal(const UObject* Context, const FString& SourcePath, const TArray<FString> MeshCodes, const int ZoneID, const FVector& ReferencePoint, const TMap<EPLATEAUCityModelPackage, FPackageInfoSettings>& PackageInfoSettingsData) {

#if !WITH_EDITOR
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("この機能は、エディタのみでご利用いただけます。")));
    return nullptr;
#endif  

    try {
        const auto World = Context->GetWorld();
        const auto InDatasetSource = plateau::dataset::DatasetSource::createLocal(TCHAR_TO_UTF8(*SourcePath));
        const auto Loader = GetCityModelLoader(World, MeshCodes, ZoneID, ReferencePoint, PackageInfoSettingsData, InDatasetSource);

        // ClientPtrは何か設定しないとクラッシュします
        Loader->ClientPtr = std::make_shared<plateau::network::Client>("", "");
        Loader->Source = SourcePath;
        Loader->bImportFromServer = false;
        return Loader;
    }
    catch (const std::exception& e) {
        UE_LOG(LogTemp, Error, TEXT("GetCityModelLoader Error : %s"), *FString(e.what()));
        return nullptr;
    }       
}

APLATEAUCityModelLoader* UPLATEAUImportModelRuntimeAPI::GetCityModelLoaderServer(const UObject* Context, const FString& InServerURL, const FString& InToken, const FString& DatasetID, const TArray<FString> MeshCodes, const int ZoneID, const FVector& ReferencePoint, const TMap<EPLATEAUCityModelPackage, FPackageInfoSettings>& PackageInfoSettingsData) {
 
#if !WITH_EDITOR
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("この機能は、エディタのみでご利用いただけます。")));
    return nullptr;
#endif  

    try {
        const auto World = Context->GetWorld();
        const auto ClientPtr = std::make_shared<plateau::network::Client>(TCHAR_TO_UTF8(*InServerURL), TCHAR_TO_UTF8(*InToken));
        const auto InDatasetSource = plateau::dataset::DatasetSource::createServer(TCHAR_TO_UTF8(*DatasetID), *ClientPtr);
        const auto Loader = GetCityModelLoader(World, MeshCodes, ZoneID, ReferencePoint, PackageInfoSettingsData, InDatasetSource);
  
        Loader->ClientPtr = ClientPtr;
        Loader->Source = DatasetID;
        Loader->bImportFromServer = true;
        return Loader;
    }
    catch (const std::exception& e) {
        UE_LOG(LogTemp, Error, TEXT("GetCityModelLoader Error : %s"), *FString(e.what()));
        return nullptr;
    }
}