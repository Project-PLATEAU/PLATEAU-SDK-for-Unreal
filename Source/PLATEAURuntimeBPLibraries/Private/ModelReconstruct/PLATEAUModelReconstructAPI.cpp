// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "ModelReconstruct/PLATEAUModelReconstructAPI.h"
#include "PLATEAURuntime/Public/PLATEAUInstancedCityModel.h"
#include "PLATEAUImportSettings.h"
#include "Misc/MessageDialog.h"

TArray<UActorComponent*> UPLATEAUModelReconstructAPI::GetSelectedComponents(AActor* Actor) {
    TArray<UActorComponent*> arr;
    if (Actor != nullptr) {
        for (auto& c : Actor->GetComponents())
            if (c->IsSelected())
                arr.Add(c);
    }
    return arr;
}

TArray<UActorComponent*> UPLATEAUModelReconstructAPI::GetSelectedComponentsByClass(AActor* Actor, UClass* Class) {
    TArray<UActorComponent*> arr;
    if (Actor != nullptr) {
        for (auto& c : Actor->GetComponents())
            if (c->IsSelected() && c->GetClass() == Class)
                arr.Add(c);
    }
    return arr;
}

void UPLATEAUModelReconstructAPI::ReconstructModel(APLATEAUInstancedCityModel* TargetCityModel, TArray<USceneComponent*> TargetComponents, const EPLATEAUMeshGranularity ReconstructType, bool bDestroyOriginal ) {
#if WITH_EDITOR
    TargetCityModel->ReconstructModel(TargetComponents, ReconstructType, bDestroyOriginal); 
#else
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("この機能は、エディタのみでご利用いただけます。")));
#endif
}