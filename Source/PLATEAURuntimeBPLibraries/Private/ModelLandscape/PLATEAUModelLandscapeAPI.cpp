// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "ModelLandscape/PLATEAUModelLandscapeAPI.h"
#include "PLATEAURuntime/Public/PLATEAUInstancedCityModel.h"
#include "Reconstruct/PLATEAUMeshLoaderForHeightmap.h"
#include "Misc/MessageDialog.h"


void UPLATEAUModelLandscapeAPI::CreateLandscape(APLATEAUInstancedCityModel* TargetCityModel, TArray<USceneComponent*> TargetComponents, bool bDestroyOriginal, FPLATEAULandscapeParam Param) {
#if WITH_EDITOR
    TargetCityModel->CreateLandscape(TargetComponents, Param, bDestroyOriginal);
#else
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("この機能は、エディタのみでご利用いただけます。")));
#endif  
}
