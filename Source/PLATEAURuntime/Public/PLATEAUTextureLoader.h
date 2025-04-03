// Copyright 2023 Ministry of Land、Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"

class PLATEAURUNTIME_API FPLATEAUTextureLoader {
public:
    static UTexture2D* Load(const FString& TexturePath, bool OverwriteTextre);
    static UTexture2D* LoadTransient(const FString& TexturePath);
    static bool SaveTexture(UTexture2D* Texture, const FString& TexturePath);
};
