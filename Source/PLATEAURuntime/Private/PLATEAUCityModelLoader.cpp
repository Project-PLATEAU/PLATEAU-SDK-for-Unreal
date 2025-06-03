// Copyright © 2023 Ministry of Land, Infrastructure and Transport


#include "PLATEAUCityModelLoader.h"

#include "PLATEAUDllLoggerUnreal.h"
#include "PLATEAUInstancedCityModel.h"
#include "plateau/dataset/dataset_source.h"
#include "plateau/dataset/city_model_package.h"
#include "plateau/polygon_mesh/mesh_extractor.h"
#include "plateau/polygon_mesh/mesh_extract_options.h"
#include "plateau/dataset/grid_code.h"
#include "PLATEAUMeshLoader.h"
#include "citygml/citygml.h"
#include "Component/PLATEAUSceneComponent.h"


#define LOCTEXT_NAMESPACE "PLATEAUCityModelLoader"

using namespace plateau::udx;


class FCityModelLoaderImpl {
public:
    static TArray<FLoadInputData> PrepareInputData(
        const UPLATEAUImportSettings* ImportSettings, const FString& Source,
        const TArray<FString>& StrGridCodes, FPLATEAUGeoReference& GeoReference, const bool bImportFromServer, const plateau::network::Client ClientRef) {
        // ファイル検索
        const auto DatasetSource = LoadDataset(bImportFromServer, Source, ClientRef);
        TArray<FLoadInputData> LoadInputDataArray;

        for (const auto& Package : UPLATEAUImportSettings::GetAllPackages()) {
            const auto Settings = ImportSettings->GetFeatureSettings(Package);
            if (!Settings.bImport)
                continue;

            std::vector<std::shared_ptr<plateau::dataset::GridCode>> NativeGridCodes;
            for (const auto& StrGridCode : StrGridCodes) {
                NativeGridCodes.push_back(plateau::dataset::GridCode::create(TCHAR_TO_UTF8(*StrGridCode)));
            }

            const auto GmlFiles =
                DatasetSource.getAccessor()
                ->filterByGridCodes(NativeGridCodes)
                ->getGmlFiles(Package);

            for (const auto& GmlFile : *GmlFiles) {
                auto& LoadInputData = LoadInputDataArray.AddDefaulted_GetRef();
                LoadInputData.GmlPath = UTF8_TO_TCHAR(GmlFile.getPath().c_str());

                // グリッドコードからインポート範囲に変換
                for (const auto& GridCode : NativeGridCodes) {
                    LoadInputData.Extents.push_back(GridCode->getExtent());
                }

                LoadInputData.bIncludeAttrInfo = Settings.bIncludeAttrInfo;
                LoadInputData.FallbackMaterial = Settings.FallbackMaterial;
                auto& ExtractOptions = LoadInputData.ExtractOptions;
                ExtractOptions.reference_point = GeoReference.GetData().getReferencePoint();
                ExtractOptions.mesh_axes = plateau::geometry::CoordinateSystem::ESU;
                ExtractOptions.coordinate_zone_id = GeoReference.GetData().getZoneID();
                ExtractOptions.mesh_granularity = UPLATEAUImportSettings::ConvertGranularity(Settings.MeshGranularity);
                ExtractOptions.max_lod = Settings.MaxLod;
                ExtractOptions.min_lod = Settings.MinLod;
                ExtractOptions.export_appearance = Settings.bImportTexture;
                ExtractOptions.enable_texture_packing = Settings.bEnableTexturePacking;
                ExtractOptions.attach_map_tile = Settings.bAttachMapTile;
                ExtractOptions.epsg_code = GmlFile.getEpsg();

                // strcpyは非推奨という警告が出ますが、共通ライブラリを利用するために必要と思われるので警告を抑制します。
                // なお抑制しないとマーケットプレイスの審査で弾かれる可能性が高いです。
#pragma warning(push)
#pragma warning(disable:4996)
                std::strcpy(ExtractOptions.map_tile_url, TCHAR_TO_UTF8(*Settings.MapTileUrl));
#pragma warning(pop)
                ExtractOptions.map_tile_zoom_level = Settings.ZoomLevel;

                switch (Settings.TexturePackingResolution) {
                case EPLATEAUTexturePackingResolution::H2048W2048:
                    ExtractOptions.texture_packing_resolution = 2048;
                    break;
                case EPLATEAUTexturePackingResolution::H4096W4096:
                    ExtractOptions.texture_packing_resolution = 4096;
                    break;
                case EPLATEAUTexturePackingResolution::H8192W8192:
                    ExtractOptions.texture_packing_resolution = 8192;
                    break;
                }
                ExtractOptions.grid_count_of_side = 10;
                ExtractOptions.unit_scale = 0.01f;
                if (Package == plateau::dataset::PredefinedCityModelPackage::Relief || Package == plateau::dataset::PredefinedCityModelPackage::DisasterRisk) {
                    ExtractOptions.exclude_city_object_outside_extent = false;
                    ExtractOptions.exclude_polygons_outside_extent = true;
                }
                else {
                    ExtractOptions.exclude_city_object_outside_extent = true;
                    ExtractOptions.exclude_polygons_outside_extent = false;
                }
            }
        }
        return LoadInputDataArray;
    }

    static plateau::dataset::DatasetSource LoadDataset(bool bImportFromServer, FString Source, plateau::network::Client ClientRef) {
        if (bImportFromServer) {
            return plateau::dataset::DatasetSource::createServer(TCHAR_TO_UTF8(*Source), ClientRef);
        }
        else {
            return plateau::dataset::DatasetSource::createLocal(TCHAR_TO_UTF8(*Source));
        }
    }

    static FString CopyGmlFile(const FString& Source, const FString& GmlPath, const bool bImportFromServer) {
        const auto Destination = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()) + "PLATEAU/Datasets";

        // ファイルコピー
        try {
            const auto SourceGml = bImportFromServer
                ? plateau::dataset::GmlFile(TCHAR_TO_UTF8(*GmlPath), plateau::network::Client("", ""))
                : plateau::dataset::GmlFile(TCHAR_TO_UTF8(*GmlPath));
            const auto CopiedGml = SourceGml.fetch(TCHAR_TO_UTF8(*Destination));
            return FString(UTF8_TO_TCHAR(CopiedGml->getPath().c_str()));
        }
        catch (std::exception& e) {
            UE_LOG(LogTemp, Error, TEXT("Failed to copy %s"), *GmlPath);
            UE_LOG(LogTemp, Error, TEXT("%s"), UTF8_TO_TCHAR(e.what()));
            return TEXT("");
        }
    }

    static std::shared_ptr<const citygml::CityModel> ParseCityGml(const FString& GmlPath) {
        std::shared_ptr<const citygml::CityModel> CityModel = nullptr;
        try {
            citygml::ParserParams ParserParams;
            ParserParams.tesselate = true;
            const auto Logger = std::make_shared<PLATEAUDllLoggerUnreal>(
                citygml::CityGMLLogger::LOGLEVEL::LL_INFO);
            CityModel = citygml::load(TCHAR_TO_UTF8(*GmlPath), ParserParams, Logger->GetLogger());
        }
        catch (std::exception& e) {
            UE_LOG(LogTemp, Error, TEXT("Error parsing gml file. Path=%s, What=%s"), *GmlPath, *FString(e.what()));
            CityModel = nullptr;
        }
        if (CityModel == nullptr)
            UE_LOG(LogTemp, Error, TEXT("Failed to parse %s"), *GmlPath);

        return CityModel;
    }

    static USceneComponent* CreateComponentInGameThread(
        AActor* Actor, const FString& Name) {
        USceneComponent* Component;
        const FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady(
            [&Component, &Actor, &Name] {
                //Component = NewObject<USceneComponent>(Actor, NAME_None);
                Component = NewObject<UPLATEAUSceneComponent>(Actor, NAME_None);
                // コンポーネント名設定(拡張子無しgml名)
                FString NewUniqueName = Name;
                if (!Component->Rename(*NewUniqueName, nullptr, REN_Test)) {
                    NewUniqueName = MakeUniqueObjectName(Actor, USceneComponent::StaticClass(), FName(Name)).ToString();
                }
                Component->Rename(*NewUniqueName, nullptr, REN_DontCreateRedirectors);

                check(Component != nullptr);
                Component->Mobility = EComponentMobility::Static;
                Actor->AddInstanceComponent(Component);
                Component->RegisterComponent();
                Component->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

            }, TStatId(), nullptr, ENamedThreads::GameThread);
        Task->Wait();

        return Component;
    }

private:
    FCriticalSection SynchronizationObject;

};


APLATEAUCityModelLoader::APLATEAUCityModelLoader() {
    PrimaryActorTick.bCanEverTick = false;
    bCanceled.Store(false, EMemoryOrder::Relaxed);
    Phase = ECityModelLoadingPhase::Idle;
}

namespace {
    void ExecuteInGameThread(TWeakObjectPtr<APLATEAUCityModelLoader> Loader, TFunctionRef<void(TWeakObjectPtr<APLATEAUCityModelLoader>)> Lambda) {
        if (!Loader.IsValid())
            return;
        FFunctionGraphTask::CreateAndDispatchWhenReady(
            [Loader, &Lambda] {
                if (!Loader.IsValid())
                    return;
                Lambda(Loader);
            }, TStatId(), nullptr, ENamedThreads::GameThread)
            ->Wait();
    }


    void CreateRootComponent(AActor& Actor) {
#if WITH_EDITOR
        USceneComponent* ActorRootComponent = NewObject<UPLATEAUSceneComponent>(&Actor,
            USceneComponent::GetDefaultSceneRootVariableName());

        check(ActorRootComponent != nullptr);
        ActorRootComponent->Mobility = EComponentMobility::Static;
        ActorRootComponent->bVisualizeComponent = true;
        Actor.SetRootComponent(ActorRootComponent);
        Actor.AddInstanceComponent(ActorRootComponent);
        ActorRootComponent->RegisterComponent();
        Actor.SetFlags(RF_Transactional);
        ActorRootComponent->SetFlags(RF_Transactional);
        GEngine->BroadcastLevelActorListChanged();
#endif
    }
}

void APLATEAUCityModelLoader::LoadModel() {
    LoadAsync(false);
}

void APLATEAUCityModelLoader::LoadAsync(const bool bAutomationTest) {
#if WITH_EDITOR

    Phase = ECityModelLoadingPhase::Start;
    bCanceled.Exchange(false);

    // アクター生成
    APLATEAUInstancedCityModel* ModelActor = GetWorld()->SpawnActor<APLATEAUInstancedCityModel>();
    CreateRootComponent(*ModelActor);

    ModelActor->GeoReference = GeoReference;
    ModelActor->GridCodes = GridCodes;
    ModelActor->Loader = this;

    Async(EAsyncExecution::Thread,
        [
            ModelActor,
                Source = Source,
                GridCodes = GridCodes,
                GeoReference = GeoReference,
                ImportSettings = ImportSettings,
                bImportFromServer = bImportFromServer,
                Client = *ClientPtr,
                OwnerLoader = TWeakObjectPtr<APLATEAUCityModelLoader>(this),
                bAutomationTest = bAutomationTest,
                bCanceledRef = &bCanceled,
                Phase = &Phase,
                ImportGmlFilesDelegate = ImportGmlFilesDelegate,
                ImportGmlProgressDelegate = ImportGmlProgressDelegate,
                ImportFailedGmlFileDelegate = ImportFailedGmlFileDelegate,
                ImportFinishedDelegate = ImportFinishedDelegate,
                LoadMeshSection = &LoadMeshSection
        ]() mutable {

                auto LoadInputDataArray = FCityModelLoaderImpl::PrepareInputData(
                    ImportSettings, Source, GridCodes, GeoReference, bImportFromServer, Client);

                TArray<FString> GmlFiles;
                for (const auto& LoadInputData : LoadInputDataArray) {
                    const auto GmlName = FPaths::GetCleanFilename(LoadInputData.GmlPath);
                    GmlFiles.Add(GmlName);
                }

                FFunctionGraphTask::CreateAndDispatchWhenReady(
                    [ImportGmlFilesDelegate, GmlFiles] {
                        ImportGmlFilesDelegate.Broadcast(GmlFiles);
                    }, TStatId(), nullptr, ENamedThreads::GameThread);

                ExecuteInGameThread(OwnerLoader,
                    [GmlCount = LoadInputDataArray.Num()](auto Loader) {
                        Loader->Status.TotalGmlCount = GmlCount;
                    });

                TArray<TFuture<bool>> Futures;
                TArray<FString> GmlNames;

                bool bHasDatasetNameSet = false;
                FCriticalSection SetDatasetNameSection;

                for (int Index = 0; Index < LoadInputDataArray.Num(); ++Index) {
                    if (bCanceledRef->Load(EMemoryOrder::Relaxed)) {
                        FFunctionGraphTask::CreateAndDispatchWhenReady(
                            [Index, ImportGmlProgressDelegate] {
                                ImportGmlProgressDelegate.Broadcast(Index, 0, LOCTEXT("Cancel", "キャンセルされました"));
                            }, TStatId(), nullptr, ENamedThreads::GameThread);
                        continue;
                    }

                    FGenericPlatformProcess::ConditionalSleep(
                        [&Futures, &GmlNames, OwnerLoader, &bCanceledRef] {
                            TArray<FString> CurrentLoadingGmls;
                            int LoadCompletedCount = 0;
                            for (int i = 0; i < Futures.Num(); ++i) {

                                if (bCanceledRef->Load(EMemoryOrder::Relaxed))
                                    return true;

                                if (!Futures[i].IsReady())
                                    CurrentLoadingGmls.Add(GmlNames[i]);
                                else
                                    ++LoadCompletedCount;

                                ExecuteInGameThread(OwnerLoader,
                                    [&CurrentLoadingGmls, &LoadCompletedCount](TWeakObjectPtr<APLATEAUCityModelLoader> Loader) {
                                        Loader->Status.LoadedGmlCount = LoadCompletedCount;
                                        Loader->Status.LoadingGmls = CurrentLoadingGmls;
                                    });
                            }
                            return CurrentLoadingGmls.Num() < 4;
                        }, 3);

                    if (bCanceledRef->Load(EMemoryOrder::Relaxed)) {
                        FFunctionGraphTask::CreateAndDispatchWhenReady(
                            [Index, ImportGmlProgressDelegate] {
                                ImportGmlProgressDelegate.Broadcast(Index, 0, LOCTEXT("Cancel", "キャンセルされました"));
                            }, TStatId(), nullptr, ENamedThreads::GameThread);
                        continue;
                    }

                    FFunctionGraphTask::CreateAndDispatchWhenReady(
                        [Index, ImportGmlProgressDelegate] {
                            ImportGmlProgressDelegate.Broadcast(Index, 0, LOCTEXT("CopyGmlFile", "ファイル取得中..."));
                        }, TStatId(), nullptr, ENamedThreads::GameThread);

                    FLoadInputData InputData = LoadInputDataArray[Index];
                    const auto CopiedGmlPath = FCityModelLoaderImpl::CopyGmlFile(Source, InputData.GmlPath, bImportFromServer);
                    const auto GmlName = FPaths::GetCleanFilename(InputData.GmlPath);

                    {
                        FScopeLock Lock(&SetDatasetNameSection);
                        if (!bHasDatasetNameSet) {
                            bHasDatasetNameSet = true;

                            // データセット名をGMLファイルパスから取得
                            // TODO: libplateauに委譲。データセット名を取得するAPI実装
                            auto DatasetName =
                                CopiedGmlPath.RightChop((FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()) + "PLATEAU/Datasets/").Len());

                            // 最初のパスの区切りを探す。
                            int32 FirstSlashIndex, FirstBackSlashIndex;
                            if (!DatasetName.FindChar(static_cast<TCHAR>('/'), FirstSlashIndex)) {
                                FirstSlashIndex = TNumericLimits<int32>::Max();
                            }
                            if (!DatasetName.FindChar(static_cast<TCHAR>('\\'), FirstBackSlashIndex)) {
                                FirstBackSlashIndex = TNumericLimits<int32>::Max();
                            }
                            DatasetName = DatasetName.Left(FMath::Min(FirstSlashIndex, FirstBackSlashIndex));

                            // 3D都市モデルアクタにデータセット名を登録
                            FFunctionGraphTask::CreateAndDispatchWhenReady(
                                [ModelActor, DatasetName]() {
                                    ModelActor->DatasetName = DatasetName;
                                    ModelActor->SetActorLabel(DatasetName);
                                }, TStatId(), nullptr, ENamedThreads::GameThread);
                        }
                    }

                    if (bCanceledRef->Load(EMemoryOrder::Relaxed)) {
                        FFunctionGraphTask::CreateAndDispatchWhenReady(
                            [Index, ImportGmlProgressDelegate] {
                                ImportGmlProgressDelegate.Broadcast(Index, 0.25, LOCTEXT("Cancel", "キャンセルされました"));
                            }, TStatId(), nullptr, ENamedThreads::GameThread);
                        continue;
                    }

                    FFunctionGraphTask::CreateAndDispatchWhenReady(
                        [Index, ImportGmlProgressDelegate] {
                            ImportGmlProgressDelegate.Broadcast(Index, 0.25, LOCTEXT("ParseCityGml", "CityGMLパース中..."));
                        }, TStatId(), nullptr, ENamedThreads::GameThread);

                    // TODO: fldでgml名被る
                    GmlNames.Add(GmlName);
                    Futures.Add(Async(EAsyncExecution::Thread,
                        [InputData, &LoadInputDataArray, Source, ModelActor, GmlName, OwnerLoader,
                        CopiedGmlPath, &LoadMeshSection, bAutomationTest, &bCanceledRef, Index, ImportGmlProgressDelegate, ImportFailedGmlFileDelegate] {

                            if (bCanceledRef->Load(EMemoryOrder::Relaxed))
                                return false;

                            const auto CityModel = FCityModelLoaderImpl::ParseCityGml(CopiedGmlPath);
                            if (CityModel == nullptr) {
                                ExecuteInGameThread(OwnerLoader,
                                    [GmlName, Index, ImportFailedGmlFileDelegate](auto Loader) {
                                        ++Loader->Status.LoadedGmlCount;
                                        Loader->Status.LoadingGmls.Remove(GmlName);
                                        Loader->Status.FailedGmls.Add(GmlName);
                                        ImportFailedGmlFileDelegate.Broadcast(Index);
                                    });
                                return false;
                            }

                            if (bCanceledRef->Load(EMemoryOrder::Relaxed)) {
                                FFunctionGraphTask::CreateAndDispatchWhenReady(
                                    [Index, ImportGmlProgressDelegate] {
                                        ImportGmlProgressDelegate.Broadcast(Index, 0.5, LOCTEXT("Cancel", "キャンセルされました"));
                                    }, TStatId(), nullptr, ENamedThreads::GameThread);
                                return false;
                            }

                            FFunctionGraphTask::CreateAndDispatchWhenReady(
                                [Index, ImportGmlProgressDelegate] {
                                    ImportGmlProgressDelegate.Broadcast(Index, 0.5, LOCTEXT("MeshExtractorExtract", "ポリゴンメッシュ変換中..."));
                                }, TStatId(), nullptr, ENamedThreads::GameThread);

                            // 注: 名前空間plateau::polygonMeshをusingで省略しないこと。Packageビルドで問題となる。
                            const auto Model = plateau::polygonMesh::MeshExtractor::extractInExtents(*CityModel, InputData.ExtractOptions, InputData.Extents);

                            // 各GMLについて親Componentを作成
                            // コンポーネントは拡張子無しgml名に設定
                            const auto GmlRootComponentName = FPaths::GetBaseFilename(CopiedGmlPath);
                            const auto GmlRootComponent = FCityModelLoaderImpl::CreateComponentInGameThread(ModelActor, GmlRootComponentName);

                            if (bCanceledRef->Load(EMemoryOrder::Relaxed)) {
                                FFunctionGraphTask::CreateAndDispatchWhenReady(
                                    [Index, ImportGmlProgressDelegate] {
                                        ImportGmlProgressDelegate.Broadcast(Index, 0.75, LOCTEXT("Cancel", "キャンセルされました"));
                                    }, TStatId(), nullptr, ENamedThreads::GameThread);
                                return false;
                            }

                            FFunctionGraphTask::CreateAndDispatchWhenReady(
                                [Index, ImportGmlProgressDelegate] {
                                    ImportGmlProgressDelegate.Broadcast(Index, 0.75, LOCTEXT("LoadModel", "ワールドに読み込み中..."));
                                }, TStatId(), nullptr, ENamedThreads::GameThread);

                            {
                                FScopeLock Lock(LoadMeshSection);
                                FPLATEAUMeshLoader(bAutomationTest).LoadModel(ModelActor, GmlRootComponent, Model, InputData, CityModel, bCanceledRef);
                            }

                            FFunctionGraphTask::CreateAndDispatchWhenReady(
                                [bCanceledRef, Index, ImportGmlProgressDelegate] {
                                    if (!bCanceledRef->Load(EMemoryOrder::Relaxed)) {
                                        ImportGmlProgressDelegate.Broadcast(Index, 1.0, LOCTEXT("Finish", "完了"));
                                    }
                                    else {
                                        ImportGmlProgressDelegate.Broadcast(Index, 0.75, LOCTEXT("Cancel", "キャンセルされました"));
                                    }
                                }, TStatId(), nullptr, ENamedThreads::GameThread);

                            return true;
                        }));
                }

                FGenericPlatformProcess::ConditionalSleep(
                    [&Futures, &GmlNames, OwnerLoader, &bCanceledRef]() {
                        TArray<FString> CurrentLoadingGmls;
                        int LoadCompletedCount = 0;
                        for (int i = 0; i < Futures.Num(); ++i) {
                            if (!Futures[i].IsReady()) {
                                CurrentLoadingGmls.Add(GmlNames[i]);
                            }
                            else {
                                ++LoadCompletedCount;
                            }

                            ExecuteInGameThread(OwnerLoader,
                                [&CurrentLoadingGmls, &LoadCompletedCount](TWeakObjectPtr<APLATEAUCityModelLoader> Loader) {
                                    Loader->Status.LoadedGmlCount = LoadCompletedCount;
                                    Loader->Status.LoadingGmls = CurrentLoadingGmls;
                                });
                        }

                        return CurrentLoadingGmls.Num() == 0;
                    }, 3);

                *Phase = ECityModelLoadingPhase::Finished;
                FFunctionGraphTask::CreateAndDispatchWhenReady(
                    [ImportFinishedDelegate] {
                        ImportFinishedDelegate.Broadcast();
                    }, TStatId(), nullptr, ENamedThreads::GameThread);
            });

#endif
}

void APLATEAUCityModelLoader::LoadGmlAsync(const FString& GmlPath) {
#if WITH_EDITOR
    Phase = ECityModelLoadingPhase::Start;

    // アクター生成
    APLATEAUInstancedCityModel* ModelActor = GetWorld()->SpawnActor<APLATEAUInstancedCityModel>();
    CreateRootComponent(*ModelActor);

    ModelActor->GeoReference = GeoReference;

    // 最初のパスの区切りを探す。
    TArray<FString> Sections;
    FString CopiedGmlPath = GmlPath;
    CopiedGmlPath.ParseIntoArray(Sections, TEXT("\\"), true);

    // 3D都市モデルアクタにデータセット名を登録
    ModelActor->DatasetName = Sections.Last();
    ModelActor->SetActorLabel(ModelActor->DatasetName);
    ModelActor->Loader = this;

    Async(EAsyncExecution::Thread,
        [
            ModelActor, GmlPath,
            GeoReference = GeoReference,
            ImportFinishedDelegate = ImportFinishedDelegate,
            Phase = &Phase
        ]() mutable {

            const auto CityModel = FCityModelLoaderImpl::ParseCityGml(GmlPath);
            auto ExtractOptions = plateau::polygonMesh::MeshExtractOptions();
            ExtractOptions.reference_point = GeoReference.GetData().getReferencePoint();
            ExtractOptions.exclude_city_object_outside_extent = false;
            ExtractOptions.unit_scale = 0.01;
            ExtractOptions.mesh_axes = plateau::geometry::CoordinateSystem::ESU;
            ExtractOptions.coordinate_zone_id = GeoReference.GetData().getZoneID();

            const auto Model = plateau::polygonMesh::MeshExtractor::extract(*CityModel, ExtractOptions);

            FLoadInputData InputData;
            InputData.ExtractOptions = ExtractOptions;
            InputData.bIncludeAttrInfo = true;
            InputData.FallbackMaterial = nullptr;
            // GMLについて親Componentを作成
            // コンポーネントは拡張子無しgml名に設定
            const auto GmlRootComponentName = FPaths::GetBaseFilename(GmlPath);
            const auto GmlRootComponent = FCityModelLoaderImpl::CreateComponentInGameThread(ModelActor, GmlRootComponentName);
            TAtomic<bool> Canceled = false;
            FPLATEAUMeshLoader(false).LoadModel(ModelActor, GmlRootComponent, Model, InputData, CityModel, &Canceled);

            *Phase = ECityModelLoadingPhase::Finished;

            FFunctionGraphTask::CreateAndDispatchWhenReady(
                [ImportFinishedDelegate] {
                    ImportFinishedDelegate.Broadcast();
                }, TStatId(), nullptr, ENamedThreads::GameThread);

            return true;
        });
#endif
}

void APLATEAUCityModelLoader::Cancel() {
    if (Phase == ECityModelLoadingPhase::Start) {
        bCanceled.Store(true);
        Phase = ECityModelLoadingPhase::Cancelling;
    }
}

void APLATEAUCityModelLoader::BeginPlay() {
    Super::BeginPlay();
}

void APLATEAUCityModelLoader::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
}

#undef LOCTEXT_NAMESPACE
