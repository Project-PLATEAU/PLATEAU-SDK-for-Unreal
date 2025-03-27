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

/// <summary>
/// 結合分離テスト (umap読込）
/// Primary => Atomic 変換
/// </summary>
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_Reconstruct_ModelReconstruct_PrimaryAtomic, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Reconstruct.ModelReconstruct.PrimaryAtomic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPLATEAUTest_Reconstruct_ModelReconstruct_PrimaryAtomic::RunTest(const FString& Parameters) {
    InitializeTest("ModelReconstruct.PrimaryAtomic");
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

        //Primary => Atomic 変換
        const auto& TargetComponent = ModelActor->FindComponentByTag<UPLATEAUCityObjectGroup>("TargetComponent");

        auto Task = ModelActor->ReconstructModel({ TargetComponent }, EPLATEAUMeshGranularity::PerAtomicFeatureObject, false);
        Task.Wait();

        //Primary => Atomic Assertions
        FString OriginalName = FPLATEAUComponentUtil::GetOriginalComponentName(TargetComponent);
        const auto& Parent = TargetComponent->GetAttachParent();
        const auto AllBldgs = Parent->GetAttachChildren();

        TestFalse("Target Visibility ", TargetComponent->IsVisible());
        TestTrue("Target has Children Components", TargetComponent->GetNumChildrenComponents() > 0);

        const auto& CreatedComponents = Task.GetResult();
        UPLATEAUCityObjectGroup* CreatedItem = (UPLATEAUCityObjectGroup*)CreatedComponents[0];
        CreatedItem->GetAllRootCityObjects(); //JsonからPropertyへの値の反映

        TestEqual("Granularity is Atomic ", CreatedItem->GetConvertGranularity(), ConvertGranularity::PerAtomicFeatureObject);
        TestEqual("OutsideParent is Target", CreatedItem->OutsideParent, OriginalName);
            
        AddInfo("Primary => Atomic  Reconstruct Task Finish"); 

        }));
    return true;
}

/// <summary>
/// 結合分離テスト (umap読込）
/// Primary => Area 変換
/// </summary>
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_Reconstruct_ModelReconstruct_PrimaryArea, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Reconstruct.ModelReconstruct.PrimaryArea", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPLATEAUTest_Reconstruct_ModelReconstruct_PrimaryArea::RunTest(const FString& Parameters) {
    InitializeTest("ModelReconstruct.PrimaryArea");
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
   
        //Primary => Area 変換
        const auto& TargetComponents = FPLATEAUComponentUtil::ConvertArrayToSceneComponentArray(
            ModelActor->GetComponentsByTag(UPLATEAUCityObjectGroup::StaticClass(), "TargetComponent2"));

        auto Task = ModelActor->ReconstructModel(TargetComponents, EPLATEAUMeshGranularity::PerCityModelArea, false);
        Task.Wait();

        const auto& CreatedComponents = Task.GetResult();

        //Primary => Area Assertions
        TestTrue("Resulted Components art not Empty", CreatedComponents.Num() > 0);

        UPLATEAUCityObjectGroup* CreatedItem = (UPLATEAUCityObjectGroup*)CreatedComponents[0];
        TestEqual("Granularity is Area ", CreatedItem->GetConvertGranularity(), ConvertGranularity::PerCityModelArea);

        AddInfo("Primary => Area Reconstruct Task Finish");

        }));
    return true;
}

/// <summary>
/// 結合分離テスト (umap読込）
/// Atomic => Primary 変換
/// </summary>
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_Reconstruct_ModelReconstruct_AtomicPrimary, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Reconstruct.ModelReconstruct.AtomicPrimary", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPLATEAUTest_Reconstruct_ModelReconstruct_AtomicPrimary::RunTest(const FString& Parameters) {
    InitializeTest("ModelReconstruct.AtomicPrimary");
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

        //Atomic => Primary 変換
        const auto& AtomicTargetComponents = FPLATEAUComponentUtil::ConvertArrayToSceneComponentArray(
            ModelActor->GetComponentsByTag(UPLATEAUCityObjectGroup::StaticClass(), "AtomicTarget"));

        auto Task = ModelActor->ReconstructModel(AtomicTargetComponents, EPLATEAUMeshGranularity::PerPrimaryFeatureObject, false);
        Task.Wait();

        const auto& CreatedComponents = Task.GetResult();

        //Atomic => Primary Assertions
        TestTrue("Resulted Components art not Empty", CreatedComponents.Num() > 0);

        UPLATEAUCityObjectGroup* CreatedItem = (UPLATEAUCityObjectGroup*)CreatedComponents[0];
        TestEqual("Granularity is Primary ", CreatedItem->GetConvertGranularity(), ConvertGranularity::PerPrimaryFeatureObject);

        const auto& TargetParent = AtomicTargetComponents[0]->GetAttachParent();
        TestEqual("Atomic Item Name and Parent Name are the same", FPLATEAUComponentUtil::GetOriginalComponentName(CreatedItem), FPLATEAUComponentUtil::GetOriginalComponentName(TargetParent));

        AddInfo("Atomic => Primary Reconstruct Task Finish");

        }));
    return true;
}
