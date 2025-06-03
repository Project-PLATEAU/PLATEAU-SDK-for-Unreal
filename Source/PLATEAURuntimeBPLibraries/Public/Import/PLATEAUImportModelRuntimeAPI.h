// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PLATEAUImportModelRuntimeAPI.generated.h"

class APLATEAUCityModelLoader;
struct FPackageInfoSettings;
enum class EPLATEAUTexturePackingResolution : uint8;

UCLASS()
class PLATEAURUNTIMEBPLIBRARIES_API UPLATEAUImportModelRuntimeAPI : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "Context"), Category = "PLATEAU|BPLibraries|ImportAPI")
    static APLATEAUCityModelLoader* GetCityModelLoaderLocal(const UObject* Context, const FString& SourcePath, const TArray<FString> GridCodes, const int ZoneID, const FVector& ReferencePoint, const TMap<EPLATEAUCityModelPackage, FPackageInfoSettings>& PackageInfoSettingsData);

    UFUNCTION(BlueprintCallable, meta = (WorldContext = "Context"), Category = "PLATEAU|BPLibraries|ImportAPI")
    static APLATEAUCityModelLoader* GetCityModelLoaderServer(const UObject* Context, const FString& InServerURL, const FString& InToken, const FString& DatasetID, const TArray<FString> GridCodes, const int ZoneID, const FVector& ReferencePoint, const TMap<EPLATEAUCityModelPackage, FPackageInfoSettings>& PackageInfoSettingsData);

};
