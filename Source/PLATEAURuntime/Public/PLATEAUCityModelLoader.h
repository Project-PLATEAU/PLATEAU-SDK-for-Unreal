// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include <vector>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PLATEAUGeometry.h"
#include "PLATEAUImportSettings.h"
#include <plateau/network/client.h>

#include "PLATEAUCityModelLoader.generated.h"

#define LOCTEXT_NAMESPACE "APLATEAUCityModelLoader"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FImportFinishedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FImportGmlFilesDelegate, const TArray<FString>&, ImportGmlFIles);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FImportGmlProgressDelegate, int, GmlIndex, float, GmlProgress, FText, GmlStatusText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FImportFailedGmlFileDelegate, int, GmlIndex);

namespace citygml {
    class CityModel;
}

class FPLATEAUMeshLoader;
enum class MeshGranularity;
struct FLoadInputData {
    plateau::polygonMesh::MeshExtractOptions ExtractOptions;
    std::vector<plateau::geometry::Extent> Extents;
    FString GmlPath;
    bool bIncludeAttrInfo;
    UMaterialInterface* FallbackMaterial;
};

UENUM(BlueprintType)
enum class EBuildingTypeMask : uint8 {
    Door = 0,
    Window = 1,
    WallSurface,
    RoofSurface,
    GroundSurface,
    ClosureSurface,
    OuterFloorSurface,
    OuterCeilingSurface
};

UENUM(BlueprintType)
enum class ECityModelLoadingPhase : uint8 {
    Idle = 0,
    Start = 1,
    Cancelling = 2,
    Finished = 3
};

namespace plateau::udx {
    enum class PredefinedCityModelPackage : uint32_t;
}

USTRUCT()
struct FPLATEAUCityModelLoadStatus {
    GENERATED_USTRUCT_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        int TotalGmlCount = 0;

    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        int LoadedGmlCount = 0;

    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        TArray<FString> LoadingGmls;

    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        TArray<FString> FailedGmls;

};

UCLASS()
class PLATEAURUNTIME_API APLATEAUCityModelLoader : public AActor {
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    APLATEAUCityModelLoader();

    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        FString Source;

    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        TArray<FString> GridCodes;

    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        FPLATEAUGeoReference GeoReference;

    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        UPLATEAUImportSettings* ImportSettings;

    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        bool bIncludeAttrInfo;
    
    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        FPLATEAUCityModelLoadStatus Status;

    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        bool bImportFromServer;

    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        ECityModelLoadingPhase Phase;

    UPROPERTY(BlueprintAssignable, Category = "PLATEAU")
        FImportGmlFilesDelegate ImportGmlFilesDelegate;

    UPROPERTY(BlueprintAssignable, Category = "PLATEAU")
        FImportGmlProgressDelegate ImportGmlProgressDelegate;
    
    UPROPERTY(BlueprintAssignable, Category = "PLATEAU")
        FImportFinishedDelegate ImportFinishedDelegate;

    UPROPERTY(BlueprintAssignable, Category = "PLATEAU")
        FImportFailedGmlFileDelegate ImportFailedGmlFileDelegate;
    
    std::shared_ptr<plateau::network::Client> ClientPtr;

    UFUNCTION(BlueprintCallable, Category = "PLATEAU")
        void LoadAsync(const bool bAutomationTest=false);

    UFUNCTION(BlueprintCallable, Category = "PLATEAU")
        void LoadModel();

    UFUNCTION(BlueprintCallable, Category = "PLATEAU")
        void LoadGmlAsync(const FString& GmlPath);

    UFUNCTION(BlueprintCallable, Category = "PLATEAU")
        void Cancel();



protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    TAtomic<bool> bCanceled;

    FCriticalSection LoadMeshSection;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;
};

#undef LOCTEXT_NAMESPACE
