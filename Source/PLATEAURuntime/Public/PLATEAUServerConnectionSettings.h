// Copyright 2023 Ministry of Land、Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"

#include "PLATEAUServerConnectionSettings.generated.h"

UCLASS()
class PLATEAURUNTIME_API UPLATEAUServerConnectionSettings : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Connection Settings Override")
        FString Url;

    UPROPERTY(EditAnywhere, Category = "Connection Settings Override")
        FString Token;
};