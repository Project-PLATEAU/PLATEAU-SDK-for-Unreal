// Copyright 2023 Ministry of Land, Infrastructure and Transport


#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PLATEAUExportModelAPI.generated.h"

class APLATEAUInstancedCityModel;

UCLASS()
class PLATEAURUNTIMEBPLIBRARIES_API UPLATEAUExportModelAPI : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "PLATEAU|BPLibraries|ExportAPI")
    static void ExportModel(APLATEAUInstancedCityModel* TargetCityModel, const FString& ExportPath, const FPLATEAUMeshExportOptions& Options);

};
