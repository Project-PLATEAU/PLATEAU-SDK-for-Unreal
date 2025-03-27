// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "FileHelpers.h"
#include "PLATEAUTests/Tests/PLATEAUAutomationTestBase.h"
#include "Reconstruct/PLATEAUMeshLoaderForReconstruct.h"
#include "PLATEAUInstancedCityModel.h"
#include "Component/PLATEAUSceneComponent.h"
#include "Util/PLATEAUComponentUtil.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include <PLATEAURuntime.h>
#include "PLATEAUModelLandscapeTestEventListener.h"
#include "Materials/MaterialInstance.h"

namespace FPLATEAUTest_Reconstruct_ModelClassification_Local {

    TMap<EPLATEAUCityObjectsType, UMaterialInterface*> CreateMaterialMapForType() {
        TMap<EPLATEAUCityObjectsType, UMaterialInterface*> Materials;
        //Wall
        FString SourcePath = TEXT("/PLATEAU-SDK-for-Unreal/Materials/Fallback/PlateauDefaultDisasterMaterialInstance");
        UMaterialInstance* Material = Cast<UMaterialInstance>(
            StaticLoadObject(UMaterialInstance::StaticClass(), nullptr, *SourcePath));
        Materials.Add(EPLATEAUCityObjectsType::COT_WallSurface, Material);
        //Roof
        SourcePath = TEXT("/PLATEAU-SDK-for-Unreal/Materials/Fallback/PlateauDefaultUrbanPlanningDecisionMaterialInstance");
        Material = Cast<UMaterialInstance>(
            StaticLoadObject(UMaterialInstance::StaticClass(), nullptr, *SourcePath));
        Materials.Add(EPLATEAUCityObjectsType::COT_RoofSurface, Material);
        return Materials;
    }
   
    const FString AttrKey = TEXT("bldg:measuredheight");
    const FString AttrValue1 = TEXT("32");
    const FString AttrValue2 = TEXT("33.7");

    TMap<FString, UMaterialInterface*> CreateMaterialMapForAttr() {
        TMap<FString, UMaterialInterface*> Materials;

        FString SourcePath = TEXT("/PLATEAU-SDK-for-Unreal/Materials/Fallback/PlateauDefaultDisasterMaterialInstance");
        UMaterialInstance* Material = Cast<UMaterialInstance>(
            StaticLoadObject(UMaterialInstance::StaticClass(), nullptr, *SourcePath));
        Materials.Add(AttrValue1, Material);

        SourcePath = TEXT("/PLATEAU-SDK-for-Unreal/Materials/Fallback/PlateauDefaultUrbanPlanningDecisionMaterialInstance");
        Material = Cast<UMaterialInstance>(
            StaticLoadObject(UMaterialInstance::StaticClass(), nullptr, *SourcePath));
        Materials.Add(AttrValue2, Material);

        return Materials;
    }
}


/// <summary>
/// マテリアル分けテスト(Type)
/// umap使用
/// </summary>
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_Reconstruct_ModelClassification_Type, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Reconstruct.Classification.Static.ClassificationByType", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPLATEAUTest_Reconstruct_ModelClassification_Type::RunTest(const FString& Parameters) {
    InitializeTest("Classification.Static.ClassificationByType");
    if (!OpenMap("SampleBldg"))
        AddError("Failed to OpenMap");

    ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(1.0f)); //Map読込待機

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), "ModelActor", FoundActors);

    if (FoundActors.Num() <= 0) {
        AddError(TEXT("0 < FoundActors.Num()"));
        return false;
    }

    APLATEAUInstancedCityModel* ModelActor = (APLATEAUInstancedCityModel*)FoundActors[0];
    
    ADD_LATENT_AUTOMATION_COMMAND(FThreadedAutomationLatentCommand([&, ModelActor] {

        const auto& TargetComponent = ModelActor->FindComponentByTag<UPLATEAUCityObjectGroup>("TargetComponent");
        TMap<EPLATEAUCityObjectsType, UMaterialInterface*> MaterialMap = FPLATEAUTest_Reconstruct_ModelClassification_Local::CreateMaterialMapForType();
        auto Task = ModelActor->ClassifyModel({ TargetComponent }, MaterialMap, EPLATEAUMeshGranularity::PerMaterialInPrimary, false);
        Task.Wait();

        //Classify By Type Assertions
        FString OriginalName = FPLATEAUComponentUtil::GetOriginalComponentName(TargetComponent);
        const auto CreatedComponents = Task.GetResult();

        TestTrue("TargetComponent has Children", TargetComponent->GetNumChildrenComponents() > 0);

        UMaterialInterface* WallMat = *MaterialMap.Find(EPLATEAUCityObjectsType::COT_WallSurface);
        UMaterialInterface* RoofMat = *MaterialMap.Find(EPLATEAUCityObjectsType::COT_RoofSurface);
        const FString WallMatName = WallMat->GetName();
        const FString RoofMatName = RoofMat->GetName();
        AddInfo("WallMatName: [" + WallMatName + "]");
        AddInfo("RoofMatName: [" + RoofMatName + "]");

        ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([&, this, CreatedComponents, WallMatName, RoofMatName] {

            //StaticMesh生成待機
            TArray<FString> ResultMaterialNames;
            for (auto CreatedComp : CreatedComponents) {
                UPLATEAUCityObjectGroup* CreatedAsCOG = StaticCast<UPLATEAUCityObjectGroup*>(CreatedComp);
                if (CreatedAsCOG->GetStaticMesh() == nullptr)
                    return false;

                AddInfo("Created: " + CreatedComp->GetName());
                UMaterialInstanceDynamic* DynMat = StaticCast<UMaterialInstanceDynamic*>(CreatedAsCOG->GetStaticMesh()->GetMaterial(0));
                if (DynMat) {
                    //ResultMaterialNames.Add(DynMat->Parent.GetName());
                    ResultMaterialNames.Add(DynMat->GetName());
                    AddInfo("Mat Added: [" + (DynMat->GetName()) + "]");
                }
            }

            AddInfo("ResultMaterialNames: " + FString::FromInt(ResultMaterialNames.Num()));
            TestTrue("Material has Wall", ResultMaterialNames.Contains(WallMatName));
            TestTrue("Material has Roof", ResultMaterialNames.Contains(RoofMatName));

            AddInfo("Primary => Atomic  Reconstruct Task Finish");
            return true;
            }));

        }));

    return true;
}

/// <summary>
/// マテリアル分けテスト(Attr)
/// umap使用
/// </summary>
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_Reconstruct_ModelClassification_Attr, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Reconstruct.Classification.Static.ClassificationByAttr", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPLATEAUTest_Reconstruct_ModelClassification_Attr::RunTest(const FString& Parameters) {
    InitializeTest("Classification.Static.ClassificationByAttr");
    if (!OpenMap("SampleBldg"))
        AddError("Failed to OpenMap");

    ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(1.0f)); //Map読込待機

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), "ModelActor", FoundActors);

    if (FoundActors.Num() <= 0) {
        AddError(TEXT("0 < FoundActors.Num()"));
        return false;
    }

    APLATEAUInstancedCityModel* ModelActor = (APLATEAUInstancedCityModel*)FoundActors[0];

    ADD_LATENT_AUTOMATION_COMMAND(FThreadedAutomationLatentCommand([&, ModelActor] {

        const auto& TargetComponents = FPLATEAUComponentUtil::ConvertArrayToSceneComponentArray(ModelActor->GetComponentsByTag(UPLATEAUCityObjectGroup::StaticClass(), "TargetComponentAttr"));

        FString AttrKey = FPLATEAUTest_Reconstruct_ModelClassification_Local::AttrKey;
        TMap<FString, UMaterialInterface*> MaterialMap = FPLATEAUTest_Reconstruct_ModelClassification_Local::CreateMaterialMapForAttr();
        auto Task = ModelActor->ClassifyModel(TargetComponents, AttrKey, MaterialMap, EPLATEAUMeshGranularity::PerPrimaryFeatureObject, false);
        Task.Wait();

        const auto CreatedComponents = Task.GetResult();
        UMaterialInterface* AttrMat1 = *MaterialMap.Find(FPLATEAUTest_Reconstruct_ModelClassification_Local::AttrValue1);
        UMaterialInterface* AttrMat2 = *MaterialMap.Find(FPLATEAUTest_Reconstruct_ModelClassification_Local::AttrValue2);
        const FString AttrMat1Name = AttrMat1->GetName();
        const FString AttrMat2Name = AttrMat2->GetName();
        AddInfo("AttrMat1Name: [" + AttrMat1Name + "]");
        AddInfo("AttrMat2Name: [" + AttrMat2Name + "]");

        //Classify By Attr Assertions
        ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([&, this, CreatedComponents, AttrMat1Name, AttrMat2Name] {

            //StaticMesh生成待機
            TArray<FString> ResultMaterialNames;
            for (auto CreatedComp : CreatedComponents) {
                UPLATEAUCityObjectGroup* CreatedAsCOG = StaticCast<UPLATEAUCityObjectGroup*>(CreatedComp);
                if (CreatedAsCOG->GetStaticMesh() == nullptr)
                    return false;

                AddInfo("Created: " + CreatedComp->GetName());
                UMaterialInstanceDynamic* DynMat = StaticCast<UMaterialInstanceDynamic*>(CreatedAsCOG->GetStaticMesh()->GetMaterial(0));
                if (DynMat) {
                    //ResultMaterialNames.Add(DynMat->Parent.GetName());
                    ResultMaterialNames.Add(DynMat->GetName());
                    AddInfo("Mat Added: [" + (DynMat->GetName()) + "]");
                }
            }

            AddInfo("ResultMaterialNames: " + FString::FromInt(ResultMaterialNames.Num()));
            TestTrue("Material has Attr1", ResultMaterialNames.Contains(AttrMat1Name));
            TestTrue("Material has Attr2", ResultMaterialNames.Contains(AttrMat2Name));
            for (auto MatName : ResultMaterialNames) {
                AddInfo("ResultMaterialName: " + MatName);
            }

            AddInfo("Primary => Atomic  Reconstruct Task Finish");
            return true;
            }));

        }));

    return true;
}
