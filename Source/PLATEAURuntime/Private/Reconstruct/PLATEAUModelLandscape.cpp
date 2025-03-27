// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include <Reconstruct/PLATEAUModelLandscape.h>
#include "Reconstruct/PLATEAUMeshLoaderForHeightmap.h"
#include <PLATEAUTextureLoader.h>
#include "Materials/MaterialInstanceConstant.h"
#include "UObject/SavePackage.h"
#include "Misc/EngineVersionComparison.h"

namespace {

    bool HasCityObjectsType(UPLATEAUCityObjectGroup* CityObj, EPLATEAUCityObjectsType Type) {
        const auto& found = CityObj->GetAllRootCityObjects().FindByPredicate([&](FPLATEAUCityObject obj) {
            return obj.Type == Type;
            });
        return found != nullptr;
    }
}

FPLATEAUModelLandscape::FPLATEAUModelLandscape() {}

FPLATEAUModelLandscape::FPLATEAUModelLandscape(APLATEAUInstancedCityModel* Actor) {
    CityModelActor = Actor;
}

TArray<HeightmapCreationResult> FPLATEAUModelLandscape::CreateHeightMap(std::shared_ptr<plateau::polygonMesh::Model> Model, FPLATEAULandscapeParam Param) {
    FPLATEAUMeshLoaderForHeightmap HMap = FPLATEAUMeshLoaderForHeightmap(false);
    return HMap.CreateHeightMap(CityModelActor, Model, Param);
}

/**
* @brief ComponentのChildrenからUPLATEAUCityObjectGroupを探してtypeがTINRelief || ReliefFeatureの場合のみリストに追加します
*/
TArray<UPLATEAUCityObjectGroup*> FPLATEAUModelLandscape::GetUPLATEAUCityObjectGroupsFromSceneComponents(TArray<USceneComponent*> TargetComponents) {
    TSet<UPLATEAUCityObjectGroup*> UniqueComponents;
    for (auto comp : TargetComponents) {
        if (comp->IsA(UActorComponent::StaticClass()) || comp->IsA(UStaticMeshComponent::StaticClass()) && StaticCast<UStaticMeshComponent*>(comp)->GetStaticMesh() == nullptr && comp->IsVisible()) {
            TArray<USceneComponent*> children;
            comp->GetChildrenComponents(true, children);
            for (auto child : children) {
                if (child->IsA(UPLATEAUCityObjectGroup::StaticClass()) && child->IsVisible()) {
                    auto childCityObj = StaticCast<UPLATEAUCityObjectGroup*>(child);
                    if (childCityObj->GetStaticMesh() != nullptr ) {
                       if(HasCityObjectsType(childCityObj, EPLATEAUCityObjectsType::COT_TINRelief) || HasCityObjectsType(childCityObj, EPLATEAUCityObjectsType::COT_ReliefFeature))
                            UniqueComponents.Add(childCityObj);
                    }
                }
            }
        }
        if (comp->IsA(UPLATEAUCityObjectGroup::StaticClass()) && comp->IsVisible()) {
            const auto& cityObj = StaticCast<UPLATEAUCityObjectGroup*>(comp);
            if (HasCityObjectsType(cityObj, EPLATEAUCityObjectsType::COT_TINRelief) || HasCityObjectsType(cityObj, EPLATEAUCityObjectsType::COT_ReliefFeature))
                UniqueComponents.Add(StaticCast<UPLATEAUCityObjectGroup*>(comp));
        }        
    }
    return UniqueComponents.Array();
}


ALandscape* FPLATEAUModelLandscape::CreateLandScape(UWorld* World, const int32 NumSubsections, const int32 SubsectionSizeQuads, const  int32 ComponentCountX, const int32 ComponentCountY, const  int32 SizeX, const int32 SizeY,
    const TVec3d Min, const TVec3d Max, const TVec2f MinUV, const TVec2f MaxUV, const FString TexturePath, TArray<uint16> HeightData, const FString ActorName) {

    // Weightmap is sized the same as the component
    const int32 WeightmapSize = (SubsectionSizeQuads + 1) * NumSubsections;
    // Should be power of two
    if (!FMath::IsPowerOfTwo(WeightmapSize)) {
        UE_LOG(LogTemp, Error, TEXT("WeightmapSize not POT:%d"), WeightmapSize);
        return nullptr;
    }

    double ActualHeight = abs(Max.z - Min.z);
    double ActualXSize = abs(Max.x - Min.x);
    double ActualYSize = abs(Max.y - Min.y);

    float HeightScale = ActualHeight / 511.992;

    float XScale = ActualXSize / (SizeX - 1);
    float YScale = ActualYSize / (SizeY - 1);

    FTransform LandscapeTransform;
    FVector LandscapeScale = FVector(XScale, YScale, HeightScale);
    FVector LandscapePosition = FVector(Min.x, Min.y, Min.z + ActualHeight / 2);
    LandscapeTransform.SetLocation(LandscapePosition);
    LandscapeTransform.SetScale3D(LandscapeScale);

    UE_LOG(LogTemp, Log, TEXT("Create Landscape SizeX:%d SizeY:%d SubsectionSizeQuads:%d  NumSubsections:%d ComponentCount(%d,%d)"), SizeX, SizeY, SubsectionSizeQuads, NumSubsections, ComponentCountX, ComponentCountY);

    TMap<FGuid, TArray<uint16>> HeightDataPerLayers;
    HeightDataPerLayers.Add(FGuid(), MoveTemp(HeightData));

    TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayers;
    TArray<FLandscapeImportLayerInfo> MaterialImportLayers;
    MaterialImportLayers.Reserve(0);
    MaterialLayerDataPerLayers.Add(FGuid(), MoveTemp(MaterialImportLayers));

#if WITH_EDITOR
    FActorSpawnParameters Param;
    ALandscape* Landscape = World->SpawnActor<ALandscape>(Param);
    Landscape->bCanHaveLayersContent = false;
    Landscape->SetActorTransform(LandscapeTransform);
#if UE_VERSION_NEWER_THAN(5, 5, 0)
    const TArrayView<const struct FLandscapeLayer> ImportLayers;
    Landscape->Import(FGuid::NewGuid(), 0, 0, SizeX - 1, SizeY - 1, NumSubsections, SubsectionSizeQuads, HeightDataPerLayers, nullptr, MaterialLayerDataPerLayers, ELandscapeImportAlphamapType::Additive, ImportLayers);
#else
    Landscape->Import(FGuid::NewGuid(), 0, 0, SizeX - 1, SizeY - 1, NumSubsections, SubsectionSizeQuads, HeightDataPerLayers, nullptr, MaterialLayerDataPerLayers, ELandscapeImportAlphamapType::Additive);
#endif

    //Create Package
    FString PackageName = TEXT("/Game/PLATEAU/Materials/");
    PackageName += FString::Format(*FString(TEXT("{0}_{1}_{2}")), { ActorName,FPaths::GetBaseFilename(TexturePath).Replace(TEXT("."), TEXT("_")), SizeX });
    UPackage* Package = CreatePackage(*PackageName);
    Package->FullyLoad();

    //Create Material (LandscapeはDynamicを使用するとうまく動作しないのでConstantを使用(Editorのみ動作))
    const auto SourceMaterialPath = TEXT("/PLATEAU-SDK-for-Unreal/Materials/PLATEAULandscapeMaterial");
    UMaterial* BaseMat = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), nullptr, SourceMaterialPath));
    UMaterialInstanceConstant* MatIns = NewObject<UMaterialInstanceConstant>(Package, NAME_None, RF_Public | RF_Standalone | RF_MarkAsRootSet);
    MatIns->SetParentEditorOnly(BaseMat, true);
    MatIns->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(FName("SizeX")), SizeX);
    MatIns->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(FName("SizeY")), SizeY);
    MatIns->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(FName("MinU")), MinUV.x);
    MatIns->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(FName("MinV")), MinUV.y);
    MatIns->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(FName("MaxU")), MaxUV.x);
    MatIns->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(FName("MaxV")), MaxUV.y);

    if (!TexturePath.IsEmpty()) {
        const auto& Texture = FPLATEAUTextureLoader::Load(TexturePath, false);
        MatIns->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(FName("MainTexture")), Texture);
    }

    //Save Material
    const FString PackageFileName = FPackageName::LongPackageNameToFilename(
        PackageName, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs Args;
    Args.SaveFlags = SAVE_NoError;
    Args.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
    Args.Error = GError;
    auto result = UPackage::Save(Package, MatIns, *PackageFileName, Args);
    if (result.Result != ESavePackageResult::Success)
        UE_LOG(LogTemp, Warning, TEXT("Save Material Failed: %s %s %d"), *PackageName, *PackageFileName, result.Result);

    Landscape->LandscapeMaterial = MatIns;

    Landscape->StaticLightingLOD = FMath::DivideAndRoundUp(FMath::CeilLogTwo((SizeX * SizeY) / (2048 * 2048) + 1), (uint32)2);

    ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
    LandscapeInfo->UpdateLayerInfoMap(Landscape);
    Landscape->RegisterAllComponents();

    FPropertyChangedEvent MaterialPropertyChangedEvent(FindFieldChecked< FProperty >(Landscape->GetClass(), FName("LandscapeMaterial")));
    Landscape->PostEditChangeProperty(MaterialPropertyChangedEvent);
    Landscape->PostEditChange();
    Landscape->SetActorLabel(FString(ActorName));

    return Landscape;
#endif   
    return nullptr;
}

void FPLATEAUModelLandscape::CreateLandScapeReference(ALandscape* Landscape, AActor* Actor, const FString ActorName) {
    FPLATEAUMeshLoaderForHeightmap MeshLoader = FPLATEAUMeshLoaderForHeightmap(false);
    MeshLoader.CreateReference(Landscape, Actor, ActorName);
}
