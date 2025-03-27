// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "FileHelpers.h"
#include "PLATEAUTests/Tests/PLATEAUAutomationTestBase.h"
#include "Util/PLATEAUComponentUtil.h"
#include "Component/PLATEAUSceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include <PLATEAURuntime.h>


IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_Util_Component_Util, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Util.ComponentUtil", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPLATEAUTest_Util_Component_Util::RunTest(const FString& Parameters) {
    InitializeTest("ComponentUtil");
    if (!OpenNewMap())
        AddError("Failed to OpenNewMap");
    
    {
        FActorSpawnParameters SpawnParam;
        const auto& Actor = GetWorld()->SpawnActor<AActor>(SpawnParam);

        const auto& SceneRoot = NewObject<UPLATEAUSceneComponent>(Actor,
            USceneComponent::GetDefaultSceneRootVariableName());
        const auto& Comp = NewObject<UPLATEAUSceneComponent>(Actor,
            FName(TEXT("Test__1")));

        Actor->AddInstanceComponent(SceneRoot);
        Actor->SetRootComponent(SceneRoot);
        SceneRoot->RegisterComponent();

        Actor->AddInstanceComponent(Comp);
        Comp->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
        Comp->RegisterComponent();

        FString OrigName = FPLATEAUComponentUtil::GetOriginalComponentName(Comp);
        TestEqual("Original Name", OrigName, "Test");
    }

    {
        FActorSpawnParameters SpawnParam;
        const auto& Actor = GetWorld()->SpawnActor<AActor>(SpawnParam);

        const auto& SceneRoot = NewObject<UPLATEAUSceneComponent>(Actor,
            USceneComponent::GetDefaultSceneRootVariableName());
        const auto& Comp = NewObject<UPLATEAUSceneComponent>(Actor,
            FName(TEXT("LOD3__4")));

        Actor->AddInstanceComponent(SceneRoot);
        Actor->SetRootComponent(SceneRoot);
        SceneRoot->RegisterComponent();

        Actor->AddInstanceComponent(Comp);
        Comp->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
        Comp->RegisterComponent();

        int Lod = FPLATEAUComponentUtil::ParseLodComponent(Comp);
        TestEqual("Parse Lod", Lod, 3);
    }

    return true;
}
