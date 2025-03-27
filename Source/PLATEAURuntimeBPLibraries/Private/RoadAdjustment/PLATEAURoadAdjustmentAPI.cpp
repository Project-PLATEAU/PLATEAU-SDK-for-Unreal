// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "RoadAdjustment/PLATEAURoadAdjustmentAPI.h"
#include "PLATEAURuntime/Public/PLATEAUInstancedCityModel.h"
#include "PLATEAUImportSettings.h"
#include "RoadNetwork/Structure/PLATEAURnStructureModel.h"
#include "Misc/MessageDialog.h"

void UPLATEAURoadAdjustmentAPI::CreateRnModel(APLATEAUInstancedCityModel* TargetCityModel,
                                              APLATEAURnStructureModel* DestActor)
{
#if WITH_EDITOR
    DestActor->CreateRnModelAsync(TargetCityModel);
#else
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("この機能は、エディタのみでご利用いただけます。")));
#endif
}
