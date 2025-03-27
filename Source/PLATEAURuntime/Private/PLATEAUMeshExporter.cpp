// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "PLATEAUMeshExporter.h"
#include "plateau/mesh_writer/gltf_writer.h"
#include "plateau/mesh_writer/obj_writer.h"
#include "plateau/mesh_writer/fbx_writer.h"
#include "PLATEAUExportSettings.h"
#include "PLATEAUInstancedCityModel.h"
#include "plateau/polygon_mesh/model.h"
#include "plateau/polygon_mesh/node.h"
#include "plateau/polygon_mesh/mesh.h"
#include "MaterialTypes.h"
#include "Engine/Texture.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstance.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "UObject/UObjectBaseUtility.h"
#include "Util/PLATEAUComponentUtil.h"
#include "Algo/Reverse.h"
#include "Misc/EngineVersionComparison.h"

#if WITH_EDITOR
#include "HAL/FileManager.h"
#include "filesystem"
#include "EditorFramework/AssetImportData.h"
#endif


namespace {
    /**
     * @brief NodeのChildに同名が存在する場合はindexを返します。ない場合は-1を返します。
     */
    int GetChildIndex(const FString& name, plateau::polygonMesh::Node* Node) {
        int num = Node->getChildCount();
        for (int i = 0; i < num; i++) {
            if (Node->getChildAt(i).getName() == TCHAR_TO_UTF8(*name))
                return i;
        }
        return -1;
    }
    /**
     * @brief ModelのRootChildに同名が存在する場合はindexを返します。ない場合は-1を返します。
     */
    int GetChildIndex(FString name, plateau::polygonMesh::Model* Model) {
        int num = Model->getRootNodeCount();
        for (int i = 0; i < num; i++) {
            if (Model->getRootNodeAt(i).getName() == TCHAR_TO_UTF8(*name))
                return i;
        }
        return -1;
    }

    /**
     * @brief FPLATEAUCityObjectからCityObjectIndexを取得してCityObjectListに追加します。
     */
    void SetCityObjectIndex(const FPLATEAUCityObject& cityObj, plateau::polygonMesh::CityObjectList& cityObjList) {
        
        // 注: 名前空間plateau::polygonMeshをusingで省略しないこと。Packageビルドで問題となる。
        plateau::polygonMesh::CityObjectIndex cityObjIdx;
        
        cityObjIdx.primary_index = cityObj.CityObjectIndex.PrimaryIndex;
        cityObjIdx.atomic_index = cityObj.CityObjectIndex.AtomicIndex;
        cityObjList.add(cityObjIdx, TCHAR_TO_UTF8(*cityObj.GmlID));
    }
}

bool FPLATEAUMeshExporter::Export(const FString& ExportPath, APLATEAUInstancedCityModel* ModelActor, const FPLATEAUMeshExportOptions& Option) {
    ModelNames.Empty();
    TargetActor = ModelActor;
    switch (Option.FileFormat) {
    case EMeshFileFormat::OBJ:
        return ExportAsOBJ(ExportPath, ModelActor, Option);
    case EMeshFileFormat::FBX:
        return ExportAsFBX(ExportPath, ModelActor, Option);
    case EMeshFileFormat::GLTF:
        return ExportAsGLTF(ExportPath, ModelActor, Option);
    default:
        return false;
    }
}


bool FPLATEAUMeshExporter::ExportAsOBJ(const FString& ExportPath, APLATEAUInstancedCityModel* ModelActor, const FPLATEAUMeshExportOptions& Option) {
    plateau::meshWriter::ObjWriter Writer;
    if (Option.TransformType == EMeshTransformType::PlaneRect) {
        ReferencePoint = ModelActor->GeoReference.ReferencePoint;
    } else {
        ReferencePoint = FVector::ZeroVector;
    }
    const auto ModelDataArray = CreateModelFromActor(ModelActor, Option);
    for (int i = 0; i < ModelDataArray.Num(); i++) {
        if (ModelDataArray[i]->getRootNodeCount() != 0) {
            const FString ExportPathWithName = ExportPath + "/" + ModelNames[i] + ".obj";
            try {
                if (!Writer.write(TCHAR_TO_UTF8(*ExportPathWithName), *ModelDataArray[i])) {
                    return false;
                }
            } catch (const std::exception& e) {
                UE_LOG(LogTemp, Error, TEXT("ExportAsOBJ Error : %s"), *FString(e.what()));
                return false;
            }
        }
    }
    return true;
}

bool FPLATEAUMeshExporter::ExportAsFBX(const FString& ExportPath, APLATEAUInstancedCityModel* ModelActor, const FPLATEAUMeshExportOptions& Option) {
    plateau::meshWriter::FbxWriter Writer;
    if (Option.TransformType == EMeshTransformType::PlaneRect) {
        ReferencePoint = ModelActor->GeoReference.ReferencePoint;
    } else {
        ReferencePoint = FVector::ZeroVector;
    }
    plateau::meshWriter::FbxWriteOptions FbxOptions;
    FbxOptions.file_format = Option.bExportAsBinary ? plateau::meshWriter::FbxFileFormat::Binary : plateau::meshWriter::FbxFileFormat::ASCII;
    FbxOptions.coordinate_system = static_cast<plateau::geometry::CoordinateSystem>(Option.CoordinateSystem);
    const auto ModelDataArray = CreateModelFromActor(ModelActor, Option);
    for (int i = 0; i < ModelDataArray.Num(); i++) {
        if (ModelDataArray[i]->getRootNodeCount() != 0) {
            const FString ExportPathWithName = ExportPath + "/" + ModelNames[i] + ".fbx";
            try {
                if (!Writer.write(TCHAR_TO_UTF8(*ExportPathWithName), *ModelDataArray[i], FbxOptions)) {
                    return false;
                }
            } catch (const std::exception& e) {
                UE_LOG(LogTemp, Error, TEXT("ExportAsFBX Error : %s"), *FString(e.what()));
                return false;
            }
        }
    }
    return true;
}

bool FPLATEAUMeshExporter::ExportAsGLTF(const FString& ExportPath, APLATEAUInstancedCityModel* ModelActor, const FPLATEAUMeshExportOptions& Option) {
    plateau::meshWriter::GltfWriter Writer;
    const auto ModelDataArray = CreateModelFromActor(ModelActor, Option);
    plateau::meshWriter::GltfWriteOptions GltfOptions;
    GltfOptions.mesh_file_format = Option.bExportAsBinary ? plateau::meshWriter::GltfFileFormat::GLTF : plateau::meshWriter::GltfFileFormat::GLB;
    GltfOptions.texture_directory_path = "./textures";
    for (int i = 0; i < ModelDataArray.Num(); i++) {
        if (ModelDataArray[i]->getRootNodeCount() != 0) {
#if WITH_EDITOR
            const FString ExportPathWithFolder = ExportPath + "/" + ModelNames[i];
            IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
            if (!PlatformFile.DirectoryExists(*ExportPathWithFolder)) {
                PlatformFile.CreateDirectory(*ExportPathWithFolder);
            }
#endif
            const FString ExportPathWithName = ExportPath + "/" + ModelNames[i] + "/" + ModelNames[i] + ".gltf";
            try {
                if (!Writer.write(TCHAR_TO_UTF8(*ExportPathWithName), *ModelDataArray[i], GltfOptions)) {
                    return false;
                }
            } catch (const std::exception& e) {
                UE_LOG(LogTemp, Error, TEXT("ExportAsGLTF Error : %s"), *FString(e.what()));
                return false;
            }
        }
    }
    return true;
}

TArray<std::shared_ptr<plateau::polygonMesh::Model>> FPLATEAUMeshExporter::CreateModelFromActor(APLATEAUInstancedCityModel* ModelActor, const FPLATEAUMeshExportOptions Option) {
    TArray<std::shared_ptr<plateau::polygonMesh::Model>> ModelArray;
    const auto RootComponent = ModelActor->GetRootComponent();
    const auto Components = RootComponent->GetAttachChildren();
    for (int i = 0; i < Components.Num(); i++) {
        //BillboardComponentなるコンポーネントがついていることがあるので無視
        if (Components[i]->GetName().Contains("BillboardComponent")) continue;

        ModelArray.Add(CreateModel(Components[i], Option));
        ModelNames.Add(FPLATEAUComponentUtil::GetOriginalComponentName(Components[i]));
    }
    return ModelArray;
}

std::shared_ptr<plateau::polygonMesh::Model> FPLATEAUMeshExporter::CreateModel(USceneComponent* ModelRootComponent, const FPLATEAUMeshExportOptions Option) {
    auto OutModel = plateau::polygonMesh::Model::createModel();
    const auto Components = ModelRootComponent->GetAttachChildren();
    for (int i = 0; i < Components.Num(); i++) {
        auto& Node = OutModel->addEmptyNode(TCHAR_TO_UTF8(*FPLATEAUComponentUtil::GetOriginalComponentName(Components[i])));
        CreateNode(Node, Components[i], Option);
    }
    return OutModel;
}

void FPLATEAUMeshExporter::CreateNode(plateau::polygonMesh::Node& OutNode, USceneComponent* NodeRootComponent, const FPLATEAUMeshExportOptions Option) {
    for (const auto& Component : NodeRootComponent->GetAttachChildren()) {
        if (!Option.bExportHiddenObjects && !Component->IsVisible())
            continue;

        auto& Node = OutNode.addEmptyChildNode(TCHAR_TO_UTF8(*Component->GetName()));
        auto Mesh = plateau::polygonMesh::Mesh();
        CreateMesh(Mesh, Component, Option);
        auto MeshPtr = std::make_unique<plateau::polygonMesh::Mesh>(Mesh);
        Node.setMesh(std::move(MeshPtr));
    }
}

void FPLATEAUMeshExporter::CreateMesh(plateau::polygonMesh::Mesh& OutMesh, USceneComponent* MeshComponent, const FPLATEAUMeshExportOptions Option) {
    const auto StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent);

    if (StaticMeshComponent == nullptr || StaticMeshComponent->GetStaticMesh() == nullptr)
        return;

    //渡すためのデータ各種
    std::vector<TVec3d> Vertices;
    std::vector<unsigned int> OutIndices;
    plateau::polygonMesh::UV UV1;
    plateau::polygonMesh::UV UV2;
    plateau::polygonMesh::UV UV3;
    plateau::polygonMesh::UV UV4;

    const auto& RenderMesh = StaticMeshComponent->GetStaticMesh()->GetLODForExport(0);

    auto& InVertices = RenderMesh.VertexBuffers.StaticMeshVertexBuffer;
    for (uint32 i = 0; i < InVertices.GetNumVertices(); ++i) {
        const FVector2f& UV = InVertices.GetVertexUV(i, 0);
        UV1.push_back(TVec2f(UV.X, 1.0f - UV.Y));
    }

    //UV4
    for (uint32 i = 0; i < InVertices.GetNumVertices(); ++i) {
        const FVector2f& UV = InVertices.GetVertexUV(i, 3);
        UV4.push_back(TVec2f(UV.X, UV.Y));
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

        // glTFの場合はm単位で出力
        if (Option.FileFormat == EMeshFileFormat::GLTF)
            Vertex = TVec3d(Vertex.x * 0.01f, Vertex.y * 0.01f, Vertex.z * 0.01f);

        Vertices.push_back(Vertex);
    }

    bool invertMesh = (Option.CoordinateSystem == ECoordinateSystem::EUN || Option.CoordinateSystem == ECoordinateSystem::ESU);
    for (int32 TriangleIndex = 0; TriangleIndex < RenderMesh.IndexBuffer.GetNumIndices() / 3; ++TriangleIndex) {
        if (!invertMesh) {
            OutIndices.push_back(RenderMesh.IndexBuffer.GetIndex(TriangleIndex * 3));
            OutIndices.push_back(RenderMesh.IndexBuffer.GetIndex(TriangleIndex * 3 + 1));
            OutIndices.push_back(RenderMesh.IndexBuffer.GetIndex(TriangleIndex * 3 + 2));
        }
        else {
            OutIndices.push_back(RenderMesh.IndexBuffer.GetIndex(TriangleIndex * 3 + 2));
            OutIndices.push_back(RenderMesh.IndexBuffer.GetIndex(TriangleIndex * 3 + 1));
            OutIndices.push_back(RenderMesh.IndexBuffer.GetIndex(TriangleIndex * 3));
        }
    }

    for (int k = 0; k < RenderMesh.Sections.Num(); k++) {
        const auto& Section = RenderMesh.Sections[k];
        if (Section.NumTriangles <= 0) continue;

        //サブメッシュの開始・終了インデックス計算
        const int FirstIndex = Section.FirstIndex;
        const int EndIndex = Section.FirstIndex + Section.NumTriangles * 3 - 1;
        ensureAlwaysMsgf((EndIndex - FirstIndex + 1) % 3 == 0, TEXT("SubMesh indices size should be multiple of 3."));

        //マテリアルがテクスチャを持っているようなら取得、設定によってはスキップ
        FString TextureFilePath = FString("");

        auto MaterialInterface =  StaticMeshComponent->GetMaterial(k);
        if (Option.bExportTexture) {
            
            if (MaterialInterface != nullptr) {
                
                const auto  MaterialInstance = Cast<UMaterialInstance>(MaterialInterface);
                if (MaterialInstance != nullptr && MaterialInstance->TextureParameterValues.Num() > 0) {

                    FMaterialParameterMetadata MetaData;
                    MaterialInstance->TextureParameterValues[0].GetValue(MetaData);
                    if (const auto Texture = MetaData.Value.Texture; Texture != nullptr) {
#if WITH_EDITOR
                        const auto TextureSourceFiles = Texture->AssetImportData->GetSourceData().SourceFiles;
                        if (TextureSourceFiles.Num() == 0) {
                            UE_LOG(LogTemp, Error, TEXT("SourceFilePath is missing in AssetImportData: %s"), *Texture->GetName());
                            // TODO マテリアル対応、下のnullptrをマテリアルに置き換える
                            OutMesh.addSubMesh("", nullptr, FirstIndex, EndIndex, CachedMaterials.Add(MaterialInterface));
                            continue;
                        }

                        const auto AssetBasePath = FPaths::GetPath(Texture->GetPackage()->GetLoadedPath().GetLocalFullPath());
                        const auto TextureFileRelativePath = TextureSourceFiles[0].RelativeFilename;
                        TextureFilePath = AssetBasePath / TextureFileRelativePath;
                        TextureFilePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*TextureFilePath);
#endif
                    }
                }
            }
        }
        std::string TextureFilePathStr = TCHAR_TO_UTF8(*TextureFilePath);

        // TODO マテリアル対応、下のnullptrをマテリアルに置き換える
        int gameMaterialID = MaterialInterface == nullptr ? -1 : CachedMaterials.Add(MaterialInterface);
        OutMesh.addSubMesh(TextureFilePathStr, nullptr, FirstIndex, EndIndex, gameMaterialID);
    }

    OutMesh.addVerticesList(Vertices);

    OutMesh.addIndicesList(OutIndices, 0, false);
    OutMesh.addUV1(UV1, Vertices.size());
    OutMesh.addUV4(UV4, Vertices.size());
    ensureAlwaysMsgf(OutMesh.getIndices().size() % 3 == 0, TEXT("Indice size should be multiple of 3."));
    ensureAlwaysMsgf(OutMesh.getVertices().size() == OutMesh.getUV1().size(), TEXT("Size of vertices and uv1 should be same."));
}

/**
 * @brief UPLATEAUCityObjectGroupのリストからplateauのModelを生成
 */
std::shared_ptr<plateau::polygonMesh::Model> FPLATEAUMeshExporter::CreateModelFromComponents(APLATEAUInstancedCityModel* ModelActor, const TArray<UPLATEAUCityObjectGroup*> ModelComponents, const FPLATEAUMeshExportOptions Option) {

    TargetActor = ModelActor;
    auto OutModel = plateau::polygonMesh::Model::createModel();

    for (const auto comp : ModelComponents) {

        plateau::polygonMesh::Node* Root;
        plateau::polygonMesh::Node* Lod; 
        FString RootName;
        FString LodName;

        TArray<USceneComponent*> Parents;
        comp->GetParentComponents(Parents);
        int LodCompIndex = Parents.IndexOfByPredicate([](const auto& Parent) {
            return Parent->GetName().StartsWith("LOD");
            });
        if (LodCompIndex == -1) {

            //LOD Nodeが存在しない場合 Nodeを１つ作ってModelに入れる
            auto& Node = OutModel->addEmptyNode(TCHAR_TO_UTF8(*comp->GetName()));
            auto Mesh = plateau::polygonMesh::Mesh();
            CreateMesh(Mesh, comp, Option);
            auto MeshPtr = std::make_unique<plateau::polygonMesh::Mesh>(Mesh);
            plateau::polygonMesh::CityObjectList cityObjList;
            for (auto cityObj : comp->GetAllRootCityObjects()) {
                SetCityObjectIndex(cityObj, cityObjList);
                for (auto child : cityObj.Children) {
                    SetCityObjectIndex(child, cityObjList);
                }
            }
            MeshPtr->setCityObjectList(cityObjList);
            Node.setMesh(std::move(MeshPtr));
        }
        else  {
            auto LodComp = Parents[LodCompIndex];
            LodName = FPLATEAUComponentUtil::GetOriginalComponentName(LodComp);
            RootName = LodComp->GetAttachParent()->GetName();
#if UE_VERSION_NEWER_THAN(5, 5, 0)
            Parents.RemoveAt(LodCompIndex, Parents.Num() - LodCompIndex, EAllowShrinking::Yes); //LOD削除
#else
            Parents.RemoveAt(LodCompIndex, Parents.Num() - LodCompIndex, true); //LOD削除
#endif

            int RootIndex = GetChildIndex(RootName, OutModel.get());
            Root = (RootIndex == -1) ?
                &OutModel->addEmptyNode(TCHAR_TO_UTF8(*RootName)) :
                &OutModel->getRootNodeAt(RootIndex);

            int LodIndex = GetChildIndex(LodName, Root);
            Lod = (LodIndex == -1) ?
                &Root->addEmptyChildNode(TCHAR_TO_UTF8(*LodName)) :
                &Root->getChildAt(LodIndex);

            plateau::polygonMesh::Node* Parent = Lod;
            if (Parents.Num() > 0) {    //最小地物の場合
                Algo::Reverse(Parents);
                for (auto p : Parents) {
                    int ParentIndex = GetChildIndex(FPLATEAUComponentUtil::GetOriginalComponentName(p), Parent);
                    Parent = (ParentIndex == -1) ?
                        &Parent->addEmptyChildNode(TCHAR_TO_UTF8(*FPLATEAUComponentUtil::GetOriginalComponentName(p))) :
                        &Parent->getChildAt(ParentIndex);
                }
            }
            auto& Node = Parent->addEmptyChildNode(TCHAR_TO_UTF8(*FPLATEAUComponentUtil::GetOriginalComponentName(comp)));
            auto Mesh = plateau::polygonMesh::Mesh();
            CreateMesh(Mesh, comp, Option);
            auto MeshPtr = std::make_unique<plateau::polygonMesh::Mesh>(Mesh);

            plateau::polygonMesh::CityObjectList cityObjList;
            for (auto cityObj : comp->GetAllRootCityObjects()) {
                SetCityObjectIndex(cityObj, cityObjList);
                for (auto child : cityObj.Children) {
                    SetCityObjectIndex(child, cityObjList);
                }
            }
            MeshPtr->setCityObjectList(cityObjList);
            Node.setMesh(std::move(MeshPtr));
        }
    }
    OutModel->assignNodeHierarchy();
    return OutModel;
}
