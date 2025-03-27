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
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_Reconstruct_ModelLandscapeAlign, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Reconstruct.Terrain.ModelLandscapeAlign", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPLATEAUTest_Reconstruct_ModelLandscapeAlign::RunTest(const FString& Parameters) {
    InitializeTest("ModelLandscapeAlign");
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
    ULandscapeLoadEventListener* Listener = NewObject<ULandscapeLoadEventListener>();
    Listener->AddToRoot();
    Listener->TestBase = this;

    ADD_LATENT_AUTOMATION_COMMAND(FThreadedAutomationLatentCommand([&, this, ModelActor, DemComponent, Param, Listener] {

        ModelActor->CreateLandscape({ DemComponent }, Param, false);

        AddInfo("CreateLandscape");
        //ModelActor->OnLandscapeCreationFinished.AddDynamic(Listener, &ULandscapeLoadEventListener::OnLandscapeLoaded);
        FScriptDelegate Delegate;
        Delegate.BindUFunction(Listener, FName(TEXT("OnLandscapeLoaded")));
        ModelActor->OnLandscapeCreationFinished.Add(Delegate);

        ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(1.0f));


        GEngine->BroadcastLevelActorListChanged();

        ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([&, this, ModelActor, DemComponent, Param, Listener] {
            if (!Listener->OnCalled)
                return false;
            AddInfo("Listener->OnCalled");

            //Assertions
            const auto& FldComponents = ModelActor->GetComponentsByTag(UPLATEAUCityObjectGroup::StaticClass(), "FldComponent");
            const auto& LsldComponents = ModelActor->GetComponentsByTag(UPLATEAUCityObjectGroup::StaticClass(), "LsldComponent");
            const auto& TranComponents = ModelActor->GetComponentsByTag(UPLATEAUCityObjectGroup::StaticClass(), "TranComponent");

            TestEqual("Fld Components Num", FldComponents.Num() * 2, ((USceneComponent*)FldComponents[0])->GetAttachParent()->GetNumChildrenComponents());
            TestEqual("Lsld Components Num", LsldComponents.Num() * 2, ((USceneComponent*)LsldComponents[0])->GetAttachParent()->GetNumChildrenComponents());
            //Lod3 Road
            TestEqual("Tran Components Num", TranComponents.Num(), ((USceneComponent*)TranComponents[0])->GetAttachParent()->GetNumChildrenComponents());

            FString OriginalName = FPLATEAUComponentUtil::GetOriginalComponentName((USceneComponent*)FldComponents[0]);
            TArray< USceneComponent*> FldChildren;
            ((USceneComponent*)FldComponents[0])->GetAttachParent()->GetChildrenComponents(false, FldChildren);
            const auto& Found = FldChildren.FilterByPredicate([&](UActorComponent* Item) {
                return FPLATEAUComponentUtil::GetOriginalComponentName((USceneComponent*)Item) == OriginalName;
                });
            TestEqual("New Fld Component Created", Found.Num(), 2);

            AddInfo("Finish Test");
            return true;;
            }));

    }));

    return true;
}
