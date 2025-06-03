// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "PLATEAUGeometry.h"
#include "GameFramework/Actor.h"
#include "Component/PLATEAUCityObjectGroup.h"
#include <plateau/polygon_mesh/model.h>
#include <plateau/dataset/city_model_package.h>
#include <PLATEAUImportSettings.h>
#include "Tasks/Task.h"
#include "Reconstruct/PLATEAUMeshLoaderForHeightmap.h"
#include "PLATEAUInstancedCityModel.generated.h"


struct FPLATEAUCityObject;
class FPLATEAUModelReconstruct;
class FPLATEAUModelClassification;
struct FPLATEAUMinMaxLod {
    int MinLod = 0;
    int MaxLod = 0;
};

USTRUCT(BlueprintType)
struct PLATEAURUNTIME_API FPLATEAUCityObjectInfo {
    GENERATED_USTRUCT_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
        FString DatasetName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
        FString GmlName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU")
        FString ID;
};

UENUM(BlueprintType)
enum class EPLATEAULandscapeCreationResult : uint8 {
    Success = 0,
    Fail = 1
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReconstructFinishedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClassifyFinishedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLandscapeCreationFinishedDelegate, EPLATEAULandscapeCreationResult, Result);

/**
 * @brief インポートされた3D都市モデルを表します。
 * 各地物のLod、CityGMLファイル名が分かるように、コンポーネント構造が以下のようになっています。
 *
 * RootComponent
 * |-{CityGMLファイル名}
 *  |-{Lod}
 *   |-{地物ID}
 * ...
 */
UCLASS()
class PLATEAURUNTIME_API APLATEAUInstancedCityModel : public AActor {
    GENERATED_BODY()

public:

    /**
     * @brief 分割・結合処理終了イベント
     */
    UPROPERTY(BlueprintAssignable, Category = "PLATEAU|BPLibraries")
    FOnReconstructFinishedDelegate OnReconstructFinished;

    /**
     * @brief マテリアル分け処理終了イベント
     */
    UPROPERTY(BlueprintAssignable, Category = "PLATEAU|BPLibraries")
    FOnClassifyFinishedDelegate OnClassifyFinished;

    /**
     * @brief ランドスケープ生成処理終了イベント
     */
    UPROPERTY(BlueprintAssignable, Category = "PLATEAU|BPLibraries")
    FOnLandscapeCreationFinishedDelegate OnLandscapeCreationFinished;

    // Sets default values for this actor's properties
    APLATEAUInstancedCityModel();

    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        FPLATEAUGeoReference GeoReference;

    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        FString DatasetName;

    UPROPERTY(VisibleDefaultsOnly, Category = "PLATEAU", BlueprintGetter = GetLatitude)
        double Latitude;

    UPROPERTY(VisibleDefaultsOnly, Category = "PLATEAU", BlueprintGetter = GetLongitude)
        double Longitude;

    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        TObjectPtr<class APLATEAUCityModelLoader> Loader;

    UPROPERTY(EditAnywhere, Category = "PLATEAU")
        TArray<FString> GridCodes;

    UFUNCTION(BlueprintGetter)
        double GetLatitude();

    UFUNCTION(BlueprintGetter)
        double GetLongitude();

    UFUNCTION(BlueprintCallable, meta = (Category = "PLATEAU|CityGML"))
        FPLATEAUCityObjectInfo GetCityObjectInfo(USceneComponent* Component);

    UFUNCTION(BlueprintCallable, meta = (Category = "PLATEAU|CityGML"))
        TArray<FPLATEAUCityObject>& GetAllRootCityObjects();

    /**
     * @brief パッケージ種を含むコンポーネントを返します
     */
    UFUNCTION(BlueprintCallable, meta = (Category = "PLATEAU|CityGML"))
        TArray<UActorComponent*> GetComponentsByPackage(EPLATEAUCityModelPackage Pkg) const;

    /**
     * @brief 3D都市モデル内に含まれるパッケージ種を返します。
     */
    plateau::dataset::PredefinedCityModelPackage GetCityModelPackages() const;

    /**
     * @brief 3D都市モデル内の各地物について、引数に従って可視化・非可視化します。
     * @param InPackage 可視化するパッケージ
     * @param PackageToLodRangeMap パッケージごとのLodに関してのユーザー選択結果
     * @param bOnlyMaxLod trueの場合各地物について提供されている最大のLodのみ可視化します。
     * @return thisを返します。
     */
    APLATEAUInstancedCityModel* FilterByLods(const plateau::dataset::PredefinedCityModelPackage InPackage, const TMap<plateau::dataset::PredefinedCityModelPackage, FPLATEAUMinMaxLod>& PackageToLodRangeMap, const bool bOnlyMaxLod);

    /**
     * @brief 3D都市モデル内の各地物について、引数に従って可視化・非可視化します。
     * @param InCityObjectType 可視化する地物タイプ
     * @return thisを返します。
     */
    APLATEAUInstancedCityModel* FilterByFeatureTypes(const citygml::CityObject::CityObjectsType InCityObjectType);
    APLATEAUInstancedCityModel* FilterByFeatureTypesLegacy(const citygml::CityObject::CityObjectsType InCityObjectType); //属性情報がない場合Modelを取得して判定

    /**
     * @brief 3D都市モデル内に含まれるLodを取得します。
     * @param InPackage 検索対象のパッケージ。フラグによって複数指定可能です。
     * @return 存在するLodの最小最大値
     */
    FPLATEAUMinMaxLod GetMinMaxLod(const plateau::dataset::PredefinedCityModelPackage InPackage) const;

    /**
     * @brief フィルタリング処理を実行中かどうかを返します。
     */
    bool IsFiltering();

    /**
     * @brief 選択されたComponentの結合・分割処理を行います。
     * @param 
     */
    UE::Tasks::TTask<TArray<USceneComponent*>> ReconstructModel(const TArray<USceneComponent*>& TargetComponents, const EPLATEAUMeshGranularity ReconstructType, bool bDestroyOriginal);


    /**
     * @brief 選択されたComponentのMaterialをCityObjectのTypeごとに分割します
     * @param
     */
    UE::Tasks::TTask<TArray<USceneComponent*>> ClassifyModel(const TArray<USceneComponent*>& TargetComponents, TMap<EPLATEAUCityObjectsType, UMaterialInterface*> Materials, const EPLATEAUMeshGranularity ReconstructType, bool bDestroyOriginal, UMaterialInterface* DefaultMaterial = nullptr);

    /**
     * @brief 選択されたComponentのMaterialを属性情報のKeyに紐づく値で分割します
     * @param
     */
    UE::Tasks::TTask<TArray<USceneComponent*>> ClassifyModel(const TArray<USceneComponent*>& TargetComponents, const FString& AttributeKey, TMap<FString, UMaterialInterface*> Materials, const EPLATEAUMeshGranularity ReconstructType, bool bDestroyOriginal, UMaterialInterface* DefaultMaterial = nullptr);

    /**
     * @brief 選択されたComponentからLandscapeを生成します
     * @param
     */
	UE::Tasks::FTask CreateLandscape(const TArray<USceneComponent*>& TargetComponents, FPLATEAULandscapeParam Param, bool bDestroyOriginal);

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;
    
    /**
     * @brief 3D都市モデル内のGMLファイルComponentの一覧を取得します。
     */
    const TArray<TObjectPtr<USceneComponent>>& GetGmlComponents() const;

    /**
     * @brief 結合分離　共通処理
     */
    UE::Tasks::TTask<TArray<USceneComponent*>> ReconstructTask(FPLATEAUModelReconstruct& ModelReconstruct, const TArray<UPLATEAUCityObjectGroup*>& TargetCityObjects, bool bDestroyOriginal);

    /**
     * @brief マテリアル分け　共通処理
     */
    UE::Tasks::TTask<TArray<USceneComponent*>> ClassifyTask(FPLATEAUModelClassification& ModelClassification, const TArray<UPLATEAUCityObjectGroup*>& TargetCityObjects, const EPLATEAUMeshGranularity ReconstructType, bool bDestroyOriginal);

    /**
     * @brief 特定パッケージを地形に合わせて高さ合わせ
     */
    TArray<UPLATEAUCityObjectGroup*> AlignLand(TArray<HeightmapCreationResult>& Results, const FPLATEAULandscapeParam& Param, bool bDestroyOriginal);

    /**
     * @brief 属性情報の有無を取得します。
     */
    bool HasAttributeInfo();

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

private:
    TAtomic<bool> bIsFiltering;
    TArray<FPLATEAUCityObject> RootCityObjects;
};
