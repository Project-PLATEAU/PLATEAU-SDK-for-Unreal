#include "PLATEAUMeshExporter.h"
#include "plateau/mesh_writer/gltf_writer.h"
#include "plateau/mesh_writer/obj_writer.h"
#include "plateau/mesh_writer/fbx_writer.h"
#include "PLATEAUExportSettings.h"
#include "PLATEAUInstancedCityModel.h"
#include "PLATEAUEditor/Private/SPLATEAUExportPanel.h"
#include "plateau/polygon_mesh/model.h"
#include "plateau/polygon_mesh/node.h"
#include "plateau/polygon_mesh/mesh.h"
#include "plateau/polygon_mesh/sub_mesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UObject/UObjectBaseUtility.h"
#include "filesystem"

void FPLATEAUMeshExporter::Export(const FString ExportPath, APLATEAUInstancedCityModel* ModelActor, const MeshExportOptions Option) {
    ModelNames.Empty();
    TargetActor = ModelActor;
    switch (Option.FileFormat) {
    case EMeshFileFormat::OBJ:
        ExportAsOBJ(ExportPath, ModelActor, Option);
        break;
    case EMeshFileFormat::FBX:
        ExportAsFBX(ExportPath, ModelActor, Option);
        break;
    case EMeshFileFormat::GLTF:
        ExportAsGLTF(ExportPath, ModelActor, Option);
        break;
    default:
        break;
    }
}

void FPLATEAUMeshExporter::ExportAsOBJ(const FString ExportPath, APLATEAUInstancedCityModel* ModelActor, const MeshExportOptions Option) {
    UE_LOG(LogTemp, Log, TEXT("Export as OBJ"));
    plateau::meshWriter::ObjWriter Writer;
    if (Option.TransformType == EMeshTransformType::PlaneRect) {
        ReferencePoint = ModelActor->GeoReference.ReferencePoint;
    }
    else {
        ReferencePoint = FVector::ZeroVector;
    }
    const auto ModelDataArray = CreateModelFromActor(ModelActor, Option);
    for (int i = 0; i < ModelDataArray.Num(); i++) {
        if (ModelDataArray[i]->getRootNodeCount() != 0) {
            const FString ExportPathWithName = ExportPath + "/" + ModelNames[i] + ".obj";
            Writer.write(TCHAR_TO_UTF8(*ExportPathWithName.Replace(TEXT("/"), TEXT("\\"))), *ModelDataArray[i]);
        }
    }
}

void FPLATEAUMeshExporter::ExportAsFBX(const FString ExportPath, APLATEAUInstancedCityModel* ModelActor, const MeshExportOptions Option) {
    UE_LOG(LogTemp, Log, TEXT("Export as FBX"));
    plateau::meshWriter::FbxWriter Writer;
    if (Option.TransformType == EMeshTransformType::PlaneRect) {
        ReferencePoint = ModelActor->GeoReference.ReferencePoint;
    }
    else {
        ReferencePoint = FVector::ZeroVector;
    }
    const auto ModelDataArray = CreateModelFromActor(ModelActor, Option);
    for (int i = 0; i < ModelDataArray.Num(); i++) {
        if (ModelDataArray[i]->getRootNodeCount() != 0) {
            const FString ExportPathWithName = ExportPath + "/" + ModelNames[i] + ".fbx";
            Writer.write(TCHAR_TO_UTF8(*ExportPathWithName.Replace(TEXT("/"), TEXT("\\"))), *ModelDataArray[i], Option.FbxWriteOptions);
        }
    }
}

void FPLATEAUMeshExporter::ExportAsGLTF(const FString ExportPath, APLATEAUInstancedCityModel* ModelActor, const MeshExportOptions Option) {
    UE_LOG(LogTemp, Log, TEXT("Export as GLTF"));
    plateau::meshWriter::GltfWriter Writer;
    const auto ModelDataArray = CreateModelFromActor(ModelActor, Option);
    for (int i = 0; i < ModelDataArray.Num(); i++) {
        if (ModelDataArray[i]->getRootNodeCount() != 0) {
            const FString ExportPathWithName = ExportPath + "/" + ModelNames[i] + "/" + ModelNames[i] + ".gltf";
            const FString ExportPathWithFolder = ExportPath + "/" + ModelNames[i];

            std::filesystem::create_directory(TCHAR_TO_UTF8(*ExportPathWithFolder.Replace(TEXT("/"), TEXT("\\"))));
            Writer.write(TCHAR_TO_UTF8(*ExportPathWithName.Replace(TEXT("/"), TEXT("\\"))), *ModelDataArray[i], Option.GltfWriteOptions);
        }
    }
}

TArray<std::shared_ptr<plateau::polygonMesh::Model>> FPLATEAUMeshExporter::CreateModelFromActor(APLATEAUInstancedCityModel* ModelActor, const MeshExportOptions Option) {
    TArray<std::shared_ptr<plateau::polygonMesh::Model>> ModelArray;
    const auto RootComponent = ModelActor->GetRootComponent();
    const auto Components = RootComponent->GetAttachChildren();
    for (int i = 0; i < Components.Num(); i++) {
        //BillboardComponentなるコンポーネントがついていることがあるので無視
        if (!Components[i]->GetName().Contains("BillboardComponent")) {
            ModelArray.Add(CreateModel(Components[i], Option));
            ModelNames.Add(RemoveSuffix(Components[i]->GetName()));
        }
    }
    return ModelArray;
}

std::shared_ptr<plateau::polygonMesh::Model> FPLATEAUMeshExporter::CreateModel(USceneComponent* ModelRootComponent, const MeshExportOptions Option) {
    auto OutModel = plateau::polygonMesh::Model::createModel();
    const auto Components = ModelRootComponent->GetAttachChildren();
    for (int i = 0; i < Components.Num(); i++) {
        auto& Node = OutModel->addEmptyNode(TCHAR_TO_UTF8(*RemoveSuffix(Components[i]->GetName())));
        CreateNode(Node, Components[i], Option);
    }
    return OutModel;
}

void FPLATEAUMeshExporter::CreateNode(plateau::polygonMesh::Node& OutNode, USceneComponent* NodeRootComponent, const MeshExportOptions Option) {
    const auto Components = NodeRootComponent->GetAttachChildren();
    for (int i = 0; i < Components.Num(); i++) {
        if (!Option.bExportHiddenObjects) {
            if (!Components[i]->IsVisible()) {
                continue;
            }
        }
        auto& Node = OutNode.addEmptyChildNode(TCHAR_TO_UTF8(*Components[i]->GetName()));
        auto& Mesh = Node.getMesh();
        Mesh.emplace();
        CreateMesh(Mesh.value(), Components[i], Option);
    }
}

void FPLATEAUMeshExporter::CreateMesh(plateau::polygonMesh::Mesh& OutMesh, USceneComponent* MeshComponent, const MeshExportOptions Option) {
    //StaticMeshComponentにキャスト
    UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent);
    FStaticMeshAttributes Attributes(*StaticMeshComponent->GetStaticMesh()->GetMeshDescription(0));

    //渡すためのデータ各種
    std::vector<TVec3d> Vertices;
    std::vector<unsigned int> OutIndices;
    plateau::polygonMesh::UV UV1;
    plateau::polygonMesh::UV UV2;
    plateau::polygonMesh::UV UV3;

    const auto& RenderMesh = StaticMeshComponent->GetStaticMesh()->GetLODForExport(0);

    for (uint32 i = 0; i < RenderMesh.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices(); ++i) {
        const FVector2f& UV = RenderMesh.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(i, 0);
        UV1.push_back(TVec2f(UV.X, 1.0f - UV.Y));
    }

    auto GeoRef = TargetActor->GeoReference.GetData();
    for (uint32 i = 0; i < RenderMesh.VertexBuffers.PositionVertexBuffer.GetNumVertices(); i++) {
        const auto VertexPosition = RenderMesh.VertexBuffers.PositionVertexBuffer.VertexPosition(i);
        TVec3d Vertex;
        if (Option.TransformType == EMeshTransformType::PlaneRect)
            Vertex = TVec3d(VertexPosition.X + ReferencePoint.X, VertexPosition.Y + ReferencePoint.Y, VertexPosition.Z + ReferencePoint.Z);
        else
            Vertex = TVec3d(VertexPosition.X, VertexPosition.Y, VertexPosition.Z);
        Vertex = GeoRef.convertAxisToENU(plateau::geometry::CoordinateSystem::ESU, Vertex);
        Vertex = GeoRef.convertAxisFromENUTo(StaticCast<plateau::geometry::CoordinateSystem>(Option.CoordinateSystem), Vertex);
        Vertices.push_back(Vertex);
    }

    for (int32 TriangleIndex = 0; TriangleIndex < RenderMesh.IndexBuffer.GetNumIndices() / 3; ++TriangleIndex) {
        OutIndices.push_back(RenderMesh.IndexBuffer.GetIndex(TriangleIndex * 3 + 2));
        OutIndices.push_back(RenderMesh.IndexBuffer.GetIndex(TriangleIndex * 3 + 1));
        OutIndices.push_back(RenderMesh.IndexBuffer.GetIndex(TriangleIndex * 3));
    }

    for (int k = 0; k < RenderMesh.Sections.Num(); k++) {
        const auto& Section = RenderMesh.Sections[k];
        //サブメッシュの開始・終了インデックス計算
        const int FirstIndex = Section.FirstIndex;
        const int EndIndex = Section.FirstIndex + Section.NumTriangles * 3;

        //マテリアルがテクスチャを持っているようなら取得、設定によってはスキップ
        FString PathName;
        if (Option.bExportTexture) {
            const auto  MaterialInstance = (UMaterialInstance*)StaticMeshComponent->GetMaterial(k);
            if (MaterialInstance->TextureParameterValues.Num() > 0) {
                FMaterialParameterMetadata MetaData;
                MaterialInstance->TextureParameterValues[0].GetValue(MetaData);
                if (const auto Texture = MetaData.Value.Texture; Texture != nullptr) {
                    PathName = (FPaths::ProjectContentDir() + "PLATEAU/" + Texture->GetName()).Replace(TEXT("/"), TEXT("\\"));
                }
            }
        }

        OutMesh.addSubMesh(TCHAR_TO_UTF8(*PathName), FirstIndex, EndIndex);
    }

    // TODO: MeshDescription使用する方法だと何故かUV取得できない
    //const auto VertexPositions = Attributes.GetVertexPositions();
    //const auto VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
    //const auto Indices = Attributes.GetVertexInstanceVertexIndices();
    //const auto SubMeshCount = StaticMeshComponent->GetStaticMesh()->GetMeshDescription(0)->PolygonGroups().Num();

    //for (const FVertexInstanceID InstanceID : StaticMeshComponent->GetStaticMesh()->GetMeshDescription(0)->VertexInstances().GetElementIDs()) {
    //    auto UV = VertexInstanceUVs.Get(InstanceID, 0);
    //    UV1.push_back(TVec2f(UV.X, UV.Y));
    //}


    //for (int i = 0; i < VertexPositions.GetNumElements(); i++) {
    //    const auto VertexPosition = VertexPositions.Get(i, 0);
    //    const TVec3d Vertex(VertexPosition.X + ReferencePoint.X, VertexPosition.Y + ReferencePoint.Y, VertexPosition.Z + ReferencePoint.Z);
    //    Vertices.push_back(Vertex);
    //}
    //for (int i = 0; i < Indices.GetNumElements(); i++) {
    //    OutIndices.push_back(Indices.Get(i));
    //}

    //int SubMeshIndex = 0;
    //for (int k = 0; k < SubMeshCount; k++) {
    //    //サブメッシュの開始・終了インデックス計算
    //    const auto SubMeshPolygonNum = StaticMeshComponent->GetStaticMesh()->GetMeshDescription(0)->GetPolygonGroupPolygons(FPolygonGroupID(k)).Num();
    //    const int SubMeshEndIndex = FMath::Min(SubMeshIndex - 1 + ((int)(SubMeshPolygonNum * 3)), (int)Vertices.size() - 1);

    //    //マテリアルがテクスチャを持っているようなら取得、設定によってはスキップ
    //    FString PathName;
    //    if (Option.bExportTexture) {
    //        const auto  MaterialInstance = (UMaterialInstance*)StaticMeshComponent->GetMaterial(k);
    //        if (MaterialInstance->TextureParameterValues.Num() > 0) {
    //            FMaterialParameterMetadata MetaData;
    //            MaterialInstance->TextureParameterValues[0].GetValue(MetaData);
    //            if (const auto Texture = MetaData.Value.Texture; Texture != nullptr) {
    //                PathName = (FPaths::ProjectContentDir() + "PLATEAU/" + Texture->GetName()).Replace(TEXT("/"), TEXT("\\"));
    //            }
    //        }
    //    }

    //    OutMesh.addSubMesh(TCHAR_TO_UTF8(*PathName), SubMeshIndex, SubMeshEndIndex);
    //    SubMeshIndex = SubMeshEndIndex + 1;
    //}

    OutMesh.addVerticesList(Vertices);

    OutMesh.addIndicesList(OutIndices, 0, false);
    OutMesh.addUV1(UV1, Vertices.size());
    OutMesh.addUV2WithSameVal(TVec2f(0.0f, 0.0f), Vertices.size());
    OutMesh.addUV3WithSameVal(TVec2f(0.0f, 0.0f), Vertices.size());
}

FString FPLATEAUMeshExporter::RemoveSuffix(const FString ComponentName) {
    int Index = 0;
    if (ComponentName.FindLastChar('_', Index)) {
        if (ComponentName.RightChop(Index + 1).IsNumeric()) {
            return ComponentName.LeftChop(ComponentName.Len() - Index);
        }
        else {
            return ComponentName;
        }
    }
    else
        return ComponentName;
}