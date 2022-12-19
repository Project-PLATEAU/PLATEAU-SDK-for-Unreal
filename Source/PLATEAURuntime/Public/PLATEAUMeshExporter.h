#pragma once

#include "CoreMinimal.h"

#include "plateau/mesh_writer/gltf_writer.h"
#include "plateau/mesh_writer/fbx_writer.h"

enum class EMeshTransformType : uint8_t;
enum class EMeshFileFormat : uint8_t;
enum class ECoordinateSystem : uint8;

class APLATEAUInstancedCityModel;

struct MeshExportOptions {
    EMeshTransformType TransformType;
    bool bExportHiddenObjects;
    bool bExportTexture;
    ECoordinateSystem CoordinateSystem;
    EMeshFileFormat FileFormat;
    plateau::meshWriter::GltfWriteOptions GltfWriteOptions;
    plateau::meshWriter::FbxWriteOptions FbxWriteOptions;
};

namespace plateau {
    namespace polygonMesh {
        class Model;
    }
}

class PLATEAURUNTIME_API FPLATEAUMeshExporter {
public:
    void Export(const FString ExportPath, APLATEAUInstancedCityModel* ModelActor, const MeshExportOptions Option);

private:
    void ExportAsOBJ(const FString ExportPath, APLATEAUInstancedCityModel* ModelActor, const MeshExportOptions Option);
    void ExportAsFBX(const FString ExportPath, APLATEAUInstancedCityModel* ModelActor, const MeshExportOptions Option);
    void ExportAsGLTF(const FString ExportPath, APLATEAUInstancedCityModel* ModelActor, const MeshExportOptions Option);
    TArray<std::shared_ptr<plateau::polygonMesh::Model>> CreateModelFromActor(APLATEAUInstancedCityModel* ModelActor, const MeshExportOptions Option);
    std::shared_ptr<plateau::polygonMesh::Model> CreateModel(USceneComponent* ModelRootComponent, const MeshExportOptions Option);
    void CreateNode(plateau::polygonMesh::Node& OutNode, USceneComponent* NodeRootComponent, const MeshExportOptions Option);
    void CreateMesh(plateau::polygonMesh::Mesh& OutMesh, USceneComponent* MeshComponent, const MeshExportOptions Option);
    FString RemoveSuffix(const FString ComponentName);

    TArray<FString> ModelNames;
    FVector ReferencePoint;
    APLATEAUInstancedCityModel* TargetActor = nullptr;
};
