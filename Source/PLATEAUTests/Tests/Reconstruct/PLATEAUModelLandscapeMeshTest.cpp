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
#include "Tasks/Task.h"
#include "PLATEAUModelLandscapeTestEventListener.h"

/// <summary>
/// 地形平滑化テスト
/// </summary>
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_Reconstruct_ModelLandscapeMesh, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Reconstruct.Terrain.ModelLandscapeMesh", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPLATEAUTest_Reconstruct_ModelLandscapeMesh::RunTest(const FString& Parameters) {
    InitializeTest("ModelLandscapeMesh");
    if (!OpenMap("SampleLand"))
        AddError("Failed to OpenMap");

    //ADD_LATENT_AUTOMATION_COMMAND(FWaitForMapToLoadCommand());
    ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(1.0f)); //Map読込待機

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), "ModelActor", FoundActors);

    if (FoundActors.Num() <= 0) {
        AddError(TEXT("0 < FoundActors.Num()"));
        return false;
    }

    APLATEAUInstancedCityModel* ModelActor = (APLATEAUInstancedCityModel*)FoundActors[0];
    UPLATEAUCityObjectGroup* DemComponent = ModelActor->FindComponentByTag<UPLATEAUCityObjectGroup>("DemComponent");
    FPLATEAULandscapeParam Param = PLATEAUAutomationTestUtil::LandscapeFixtures::CreateLandscapeParam();
    Param.ConvertToLandscape = false; //Mesh 生成
    ULandscapeLoadEventListener* Listener = NewObject<ULandscapeLoadEventListener>();
    Listener->AddToRoot();
    Listener->TestBase = this;

    ADD_LATENT_AUTOMATION_COMMAND(FThreadedAutomationLatentCommand([&, this, ModelActor, DemComponent, Param, Listener] {

        ModelActor->CreateLandscape({ DemComponent }, Param, false);

        AddInfo("CreateLandscape");
        FScriptDelegate Delegate;
        Delegate.BindUFunction(Listener, FName(TEXT("OnLandscapeLoaded")));
        ModelActor->OnLandscapeCreationFinished.Add(Delegate);

        ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(1.0f));

        ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([&, this, ModelActor, DemComponent, Param, Listener] {
            if (!Listener->OnCalled)
                return false;
            AddInfo("Listener->OnCalled");

            //Assertions
            const auto& Parent = DemComponent->GetAttachParent();
            TestEqual("Dem Component Created ", Parent->GetNumChildrenComponents(), 2);
            TestEqual("Dem Component Name", FPLATEAUComponentUtil::GetOriginalComponentName(Parent->GetChildComponent(1)), "Mesh_" + FPLATEAUComponentUtil::GetOriginalComponentName(DemComponent));

            AddInfo("Finish Test");
            return true;
            }));

    }));

    return true;
}
