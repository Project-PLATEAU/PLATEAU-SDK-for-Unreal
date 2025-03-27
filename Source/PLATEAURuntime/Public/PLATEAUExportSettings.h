// Copyright 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "HAL/FileManagerGeneric.h"
#include "PLATEAUExportSettings.generated.h"


UENUM(BlueprintType)
enum class EMeshTransformType : uint8 {
    //! ローカル座標
    Local,
    //! 平面直角座標系
    PlaneRect
};

UENUM(BlueprintType, meta = (ScriptName = "PLATEAUCoordinateSystem"))
enum class ECoordinateSystem : uint8 {
    //! PLATEAUでの座標系
    ENU = 0,
    WUN = 1,
    //! Unreal Engineでの座標系
    ESU = 2,
    //! Unityでの座標系
    EUN = 3
};

UENUM(BlueprintType)
enum class EMeshFileFormat : uint8 {
    OBJ = 0,
    FBX,
    GLTF,
    EMeshFileFormat_MAX,
};

USTRUCT(BlueprintType)
struct FPLATEAUMeshExportOptions {
    GENERATED_BODY()

    FPLATEAUMeshExportOptions() :
        TransformType(EMeshTransformType::Local)
        , bExportHiddenObjects(false)
        , bExportTexture(true)
        , CoordinateSystem(ECoordinateSystem::ENU)
        , FileFormat(EMeshFileFormat::FBX)
        , bExportAsBinary(false) {
    }

    UPROPERTY(BlueprintReadWrite, Category = "PLATEAU|ExportSettings")
    EMeshTransformType TransformType;

    UPROPERTY(BlueprintReadWrite, Category = "PLATEAU|ExportSettings")
    bool bExportHiddenObjects;

    UPROPERTY(BlueprintReadWrite, Category = "PLATEAU|ExportSettings")
    bool bExportTexture;

    UPROPERTY(BlueprintReadWrite, Category = "PLATEAU|ExportSettings")
    ECoordinateSystem CoordinateSystem;

    UPROPERTY(BlueprintReadWrite, Category = "PLATEAU|ExportSettings")
    EMeshFileFormat FileFormat;

    UPROPERTY(BlueprintReadWrite, Category = "PLATEAU|ExportSettings")
    bool bExportAsBinary;
};

namespace plateau::Export {
    static TArray<FString> GetFoundFiles(const EMeshFileFormat InEMeshFileFormat, const FString& InExportPath) {
        TArray<FString> FoundFileArray;
        FoundFileArray.Empty();
        switch (InEMeshFileFormat) {
        case EMeshFileFormat::OBJ:
            FFileManagerGeneric::Get().FindFiles(FoundFileArray, *(InExportPath + "/*.obj"), true, false);
            break;
        case EMeshFileFormat::FBX:
            FFileManagerGeneric::Get().FindFiles(FoundFileArray, *(InExportPath + "/*.fbx"), true, false);
            break;
        case EMeshFileFormat::GLTF:
            FFileManagerGeneric::Get().FindFilesRecursive(FoundFileArray, *InExportPath, UTF8_TO_TCHAR("*.gltf"), true, false);
            break;
        default:
            break;
        }

        return FoundFileArray;
    }
    
    static FString MeshFileFormatToStr(const EMeshFileFormat InEMeshFileFormat) {
        switch (InEMeshFileFormat) {
        case EMeshFileFormat::OBJ:
            return "OBJ";
        case EMeshFileFormat::FBX:
            return "FBX";
        case EMeshFileFormat::GLTF:
            return "GLTF";
        default:
            return "";
        }
    }
}