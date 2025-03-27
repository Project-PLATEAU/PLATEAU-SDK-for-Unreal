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
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_Reconstruct_ModelLandscape, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Reconstruct.Terrain.ModelLandscape", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPLATEAUTest_Reconstruct_ModelLandscape::RunTest(const FString& Parameters) {
    InitializeTest("ModelLandscape");
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

        auto Task = ModelActor->CreateLandscape({ DemComponent }, Param, false);

        AddInfo("CreateLandscape");
        FScriptDelegate Delegate;
        Delegate.BindUFunction(Listener, FName(TEXT("OnLandscapeLoaded")));
        ModelActor->OnLandscapeCreationFinished.Add(Delegate);

        ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(1.0f));

        ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([&, this, ModelActor, DemComponent, Param, Listener] {
            if (!Listener->OnCalled)
                return false;

            //Assertions
            TArray<AActor*> FoundLandscapes;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALandscape::StaticClass(), FoundLandscapes);
            TestEqual("Landscape Created ", FoundLandscapes.Num(), 1);

            ALandscape* Land = (ALandscape*)FoundLandscapes[0];
            TestEqual("Landscape Actor Name", Land->GetActorNameOrLabel(), FPLATEAUComponentUtil::GetOriginalComponentName(DemComponent));
            const auto& Info = Land->LandscapeComponents[0]->GetLandscapeInfo();
            TestEqual("SubsectionSizeQuads", Info->SubsectionSizeQuads, Param.SubsectionSizeQuads);
            TestEqual("NumSubsections", Info->ComponentNumSubsections, Param.NumSubsections);

            const auto& Parent = DemComponent->GetAttachParent();
            TestEqual("Ref Component Created ", Parent->GetNumChildrenComponents(), 2);
            TestEqual("Ref Component Name", Parent->GetChildComponent(1)->GetName(), "Ref_" + FPLATEAUComponentUtil::GetOriginalComponentName(DemComponent));

            AddInfo("Finish Test");

            return true;
            }));
        AddInfo("Listener->OnCalled");

    }));

    return true;
}
