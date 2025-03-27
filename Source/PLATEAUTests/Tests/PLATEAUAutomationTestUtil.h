// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#pragma once
#include "CoreMinimal.h"
#include <PLATEAURuntime.h>
#include "Util/PLATEAUComponentUtil.h"
#include "Util/PLATEAUReconstructUtil.h"
#include <PLATEAUExportSettings.h>
#include <PLATEAUMeshExporter.h>
#include <ImageUtils.h>
#include <CityGML/PLATEAUCityGmlProxy.h>
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

//ダイナミック生成等のテスト用共通処理
namespace PLATEAUAutomationTestUtil {

    namespace Fixtures {

        const FString TEST_ACTOR_NAME = "TestActor";
        const FString TEST_OP_NAME = "00000000_bldg_0000_op";
        const FString TEST_LOD_NAME = "LOD1";
        const FString TEST_OBJ_NAME = "bldg_00000000-aaaa-0000-0000-000000000000";
        const FString TEST_CITYOBJ_TYPE = "Building";
        const FName TEST_OBJ_TAG = "test";
        const FString TEST_CITYOBJ_WALL_NAME = "surface-00000000-0000-0000-0000-000000000000";
        const FString TEST_CITYOBJ_ROOF_NAME = "surface-00000000-1111-0000-0000-000000111111";
        const FString TEST_CITYOBJ_WALL_TYPE = "WallSurface";
        const FString TEST_CITYOBJ_ROOF_TYPE = "RoofSurface";

        /// <summary>
        /// Actor と Compoenent生成
        /// </summary>
        inline APLATEAUInstancedCityModel* CreateActor(UWorld& World) {
            APLATEAUInstancedCityModel* Actor = World.SpawnActor<APLATEAUInstancedCityModel>();
            const auto& SceneRoot = NewObject<UPLATEAUSceneComponent>(Actor,
                USceneComponent::GetDefaultSceneRootVariableName());
            const auto& CompRoot = NewObject<UPLATEAUSceneComponent>(Actor,
                FName(TEST_OP_NAME));
            const auto& CompLod = NewObject<UPLATEAUSceneComponent>(Actor,
                FName(TEST_LOD_NAME + "__1"));
            const auto& CompObj = NewObject<UPLATEAUCityObjectGroup>(Actor,
                FName(TEST_OBJ_NAME + "__1"));

            Actor->SetActorLabel(TEST_ACTOR_NAME);
            Actor->AddInstanceComponent(SceneRoot);
            Actor->SetRootComponent(SceneRoot);
            SceneRoot->RegisterComponent();

            Actor->AddInstanceComponent(CompRoot);
            CompRoot->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
            CompRoot->RegisterComponent();
            CompRoot->SetMobility(EComponentMobility::Static);

            CompLod->AttachToComponent(CompRoot, FAttachmentTransformRules::KeepWorldTransform);
            Actor->AddInstanceComponent(CompLod);
            CompLod->RegisterComponent();
            CompLod->SetMobility(EComponentMobility::Static);

            CompObj->AttachToComponent(CompLod, FAttachmentTransformRules::KeepWorldTransform);
            Actor->AddInstanceComponent(CompObj);
            CompObj->RegisterComponent();
            CompObj->SetMobility(EComponentMobility::Static);
            CompObj->ComponentTags.Add(TEST_OBJ_TAG);

            GEngine->BroadcastLevelActorListChanged();
            return Actor;
        }

        /// <summary>
        /// Mesh用のCityObjectIndexのList生成
        /// </summary>
        inline void CreateCityObjectList(plateau::polygonMesh::CityObjectList& CityObj) {

            CityObj.add(plateau::polygonMesh::CityObjectIndex(0, -1), TCHAR_TO_UTF8(*TEST_OBJ_NAME));
            CityObj.add(plateau::polygonMesh::CityObjectIndex(0, 1), TCHAR_TO_UTF8(*TEST_CITYOBJ_WALL_NAME));
            CityObj.add(plateau::polygonMesh::CityObjectIndex(0, 2), TCHAR_TO_UTF8(*TEST_CITYOBJ_ROOF_NAME));
        }

        /// <summary>
        /// Mesh生成
        /// </summary>
        inline void CreateMesh(plateau::polygonMesh::Mesh& Mesh, const plateau::polygonMesh::CityObjectList CityObj) {
            std::vector<unsigned int> indices{ 0, 1, 2, 3, 2, 0 };
            std::vector<TVec3d> vertices{ TVec3d(0,0,0),TVec3d(0, 100, 10),TVec3d(100, 100, 30),TVec3d(100, 0, 10) };
            plateau::polygonMesh::UV uv1;

            for (auto v : vertices) {
                uv1.push_back(TVec2f(0, 0));
            }
            Mesh.addIndicesList(indices, 0, false);
            Mesh.addVerticesList(vertices);
            Mesh.addSubMesh("", nullptr, 0, indices.size() - 1, 0);
            Mesh.addUV1(uv1, vertices.size());
            Mesh.addUV4WithSameVal(TVec2f(0, 1), vertices.size());
            Mesh.setCityObjectList(CityObj);
        }

        /// <summary>
        /// Model / 各Node 生成
        /// </summary>
        inline std::shared_ptr<plateau::polygonMesh::Model> CreateModel(plateau::polygonMesh::Mesh& Mesh) {
            std::shared_ptr<plateau::polygonMesh::Model> Model = plateau::polygonMesh::Model::createModel();
            auto& NodeOP = Model->addEmptyNode(TCHAR_TO_UTF8(*TEST_OP_NAME));
            auto& NodeLod = NodeOP.addEmptyChildNode(TCHAR_TO_UTF8(*TEST_LOD_NAME));
            auto& NodeObj = NodeLod.addEmptyChildNode(TCHAR_TO_UTF8(*TEST_OBJ_NAME));
            auto MeshPtr = std::make_unique<plateau::polygonMesh::Mesh>(Mesh);
            NodeObj.setMesh(std::move(MeshPtr));
            Model->assignNodeHierarchy();
            Model->optimizeMeshes();
            return Model;
        }

        /// <summary>
        /// CreateModelで生成したModelから属性情報を持つNodeの取得用  (root/lod/bldg)各単体の場合
        /// </summary>
        inline const plateau::polygonMesh::Node& GetObjNode(std::shared_ptr<plateau::polygonMesh::Model> Model) {
            const auto& root = Model->getRootNodeAt(0);
            const auto& lod = root.getChildAt(0);
            return lod.getChildAt(0); //bldg_00000000-aaaa-0000-0000-000000000000
        }

        /// <summary>
        /// FPLATEAUCityObject & GMLID : FPLATEAUCityObject のMap 生成
        /// </summary>
        inline TMap<FString, FPLATEAUCityObject> CreateCityObjectMap() {
            TMap<FString, FPLATEAUCityObject> Map;
            FPLATEAUCityObject CityObj;
            CityObj.SetGmlID(TEST_OBJ_NAME);
            CityObj.SetCityObjectsType(TEST_CITYOBJ_TYPE);
            CityObj.SetCityObjectIndex(plateau::polygonMesh::CityObjectIndex(0, -1));
            Map.Add(TEST_OBJ_NAME, CityObj);
            return Map;
        }

        //CityObject (Building)
        inline void CreateCityObjectBuilding(FPLATEAUCityObject& InCityObj) {
            InCityObj.SetGmlID(TEST_OBJ_NAME);
            InCityObj.SetCityObjectsType(TEST_CITYOBJ_TYPE);
            InCityObj.SetCityObjectIndex(plateau::polygonMesh::CityObjectIndex(0, -1));
        }

        //CityObject Children (Wall/Roof)
        inline void CreateCityObjectBuildingChildren(FPLATEAUCityObject& ParentCityObj) {
            FPLATEAUCityObject Wall;
            Wall.SetGmlID(TEST_CITYOBJ_WALL_NAME);
            Wall.SetCityObjectsType(TEST_CITYOBJ_WALL_TYPE);
            Wall.SetCityObjectIndex(plateau::polygonMesh::CityObjectIndex(0, 0));
            ParentCityObj.Children.Add(Wall);
            FPLATEAUCityObject Roof;
            Roof.SetGmlID(TEST_CITYOBJ_ROOF_NAME);
            Roof.SetCityObjectsType(TEST_CITYOBJ_ROOF_TYPE);
            Roof.SetCityObjectIndex(plateau::polygonMesh::CityObjectIndex(0, 0));
            ParentCityObj.Children.Add(Roof);
        }

        /// <summary>
        /// Load用データ生成
        /// </summary>
        inline FLoadInputData CreateLoadInputData(plateau::polygonMesh::MeshGranularity MeshGranularity) {
            plateau::polygonMesh::MeshExtractOptions MeshExtractOptions{};
            MeshExtractOptions.mesh_granularity = MeshGranularity;
            return FLoadInputData
            {
                MeshExtractOptions,
                std::vector<plateau::geometry::Extent>{},
                FString(),
                false,
                nullptr
            };
        }

        /// <summary>
        /// StaticMesh生成
        /// </summary>
        inline UStaticMesh* CreateStaticMesh(AActor* Actor, FName Name, FVector3f Offset = FVector3f::Zero()) {
            FMeshDescription mesh_desc;

            FStaticMeshAttributes attributes(mesh_desc);
            attributes.Register();

            TVertexAttributesRef<FVector3f> positions = mesh_desc.GetVertexPositions();

            mesh_desc.ReserveNewVertices(4);
            FVertexID v0 = mesh_desc.CreateVertex();
            FVertexID v1 = mesh_desc.CreateVertex();
            FVertexID v2 = mesh_desc.CreateVertex();
            FVertexID v3 = mesh_desc.CreateVertex();

            mesh_desc.ReserveNewVertexInstances(4);
            FVertexInstanceID vi0 = mesh_desc.CreateVertexInstance(v0);
            FVertexInstanceID vi1 = mesh_desc.CreateVertexInstance(v1);
            FVertexInstanceID vi2 = mesh_desc.CreateVertexInstance(v2);
            FVertexInstanceID vi3 = mesh_desc.CreateVertexInstance(v3);

            mesh_desc.ReserveNewUVs(4, 0);
            FUVID uv0 = mesh_desc.CreateUV();
            FUVID uv1 = mesh_desc.CreateUV();
            FUVID uv2 = mesh_desc.CreateUV();
            FUVID uv3 = mesh_desc.CreateUV();

            //UV4
            mesh_desc.SetNumUVChannels(4);
            mesh_desc.ReserveNewUVs(4, 3);
            FUVID uv4_0 = mesh_desc.CreateUV();
            FUVID uv4_1 = mesh_desc.CreateUV();
            FUVID uv4_2 = mesh_desc.CreateUV();
            FUVID uv4_3 = mesh_desc.CreateUV();

            FPolygonGroupID polygon_group = mesh_desc.CreatePolygonGroup();

            mesh_desc.ReserveNewPolygons(1);
            mesh_desc.CreatePolygon(polygon_group, { vi0, vi1, vi2, vi3 });

            positions = attributes.GetVertexPositions();

            positions[0] = FVector3f(-100, -100, 0) + Offset;
            positions[1] = FVector3f(-100, 100, 0) + Offset;
            positions[2] = FVector3f(100, 100, 0) + Offset;
            positions[3] = FVector3f(100, -100, 0) + Offset;

            TVertexInstanceAttributesRef<FVector3f> normals = attributes.GetVertexInstanceNormals();

            normals[0] = FVector3f(0, 0, 1);
            normals[1] = FVector3f(0, 0, 1);
            normals[2] = FVector3f(0, 0, 1);
            normals[3] = FVector3f(0, 0, 1);

            TVertexInstanceAttributesRef<FVector2f> uvs = attributes.GetVertexInstanceUVs();

            uvs.Get(0, 0) = FVector2f(0, 0);
            uvs.Get(1, 0) = FVector2f(0, 1);
            uvs.Get(2, 0) = FVector2f(1, 1);
            uvs.Get(3, 0) = FVector2f(1, 0);

            //UV4
            uvs.SetNumChannels(4);
            uvs.Get(0, 3) = FVector2f(0, 1);
            uvs.Get(1, 3) = FVector2f(0, 1);
            uvs.Get(2, 3) = FVector2f(0, 2);
            uvs.Get(3, 3) = FVector2f(0, 2);

            // At least one material must be added
            UStaticMesh* mesh = NewObject<UStaticMesh>(Actor, Name);
            //mesh->GetStaticMaterials().Add(FStaticMaterial());

            UStaticMesh::FBuildMeshDescriptionsParams mdParams;
            mdParams.bBuildSimpleCollision = true;

            mesh->NaniteSettings.bEnabled = false;

            // Build static mesh
            mesh->BuildFromMeshDescriptions({ &mesh_desc }, mdParams);

            mesh->InitResources();
            // make sure it has a new lighting guid
            mesh->SetLightingGuid();
            // Set it to use textured lightmaps. Note that Build Lighting will do the error-checking (texcoordindex exists for all LODs, etc).
            mesh->SetLightMapResolution(64);
            mesh->SetLightMapCoordinateIndex(1);

            return mesh;
        }
        /// <summary>
        /// マテリアルを生成
        /// </summary>
        inline UMaterialInterface* CreateMaterial() {
            const auto SourceMaterialPath = TEXT("/PLATEAU-SDK-for-Unreal/Materials/PLATEAUX3DMaterial");
            UMaterial* Mat = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), nullptr, SourceMaterialPath));
            if (!Mat) {
                UE_LOG(LogTemp, Warning, TEXT("Failed to load material at path: %s"), SourceMaterialPath);
            }
            return Mat;
        }

        /// <summary>
        /// StaticMeshに色付きマテリアルを設定
        /// </summary>
        inline void SetMaterial(UStaticMesh* mesh, FVector3f Color = FVector3f::Zero()) {
            UMaterial* Base = Cast<UMaterial>(CreateMaterial());
            if (!Base) {
                UE_LOG(LogTemp, Error, TEXT("Failed to create material for mesh"));
                return; 
            }
            UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, mesh);
            if (Color != FVector3f::Zero()) Mat->SetVectorParameterValue("BaseColor", Color);
            mesh->AddMaterial(Mat);
        }

        /// <summary>
        /// Componentに色付きマテリアルを設定
        /// </summary>
        inline void SetMaterial(UPLATEAUCityObjectGroup* Comp, FVector3f Color = FVector3f::Zero()) {
            UMaterial* Base = Cast<UMaterial>(CreateMaterial());
            if (!Base) {
                UE_LOG(LogTemp, Error, TEXT("Failed to create material for mesh"));
                return;
            }
            UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, Comp);
            if (Color != FVector3f::Zero()) Mat->SetVectorParameterValue("BaseColor", Color);
            Comp->SetMaterial(0, Mat);
        }

        /// <summary>
        /// StaticMeshにTexture付きマテリアルを設定
        /// </summary>
        inline void SetMaterialWithTexture(UStaticMesh* mesh, FString TextureName) {
            UMaterial* Base = Cast<UMaterial>(CreateMaterial());
            UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, mesh);
            if (!Base) {
                UE_LOG(LogTemp, Error, TEXT("Failed to create material for mesh with texture"));
                return;
            }
            FString Path = FPLATEAURuntimeModule::GetContentDir().Append("/TestData/texture/").Append(TextureName);
            UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(Path);
            if (Texture) Mat->SetTextureParameterValue("Texture", Texture);
            mesh->AddMaterial(Mat);
        }

        //Atomic
        /// <summary>
        /// Actor と Compoenent生成
        /// </summary>
        inline APLATEAUInstancedCityModel* CreateActorAtomic(UWorld& World) {
            APLATEAUInstancedCityModel* Actor = World.SpawnActor<APLATEAUInstancedCityModel>();
            const auto& SceneRoot = NewObject<UPLATEAUSceneComponent>(Actor,
                USceneComponent::GetDefaultSceneRootVariableName());
            const auto& CompRoot = NewObject<UPLATEAUSceneComponent>(Actor,
                FName(TEST_OP_NAME));
            const auto& CompLod = NewObject<UPLATEAUSceneComponent>(Actor,
                FName(TEST_LOD_NAME + "__1"));
            const auto& CompObj = NewObject<UPLATEAUCityObjectGroup>(Actor,
                FName(TEST_OBJ_NAME + "__1"));
            const auto& CompObjWall = NewObject<UPLATEAUCityObjectGroup>(Actor,
                FName(TEST_CITYOBJ_WALL_NAME + "__1"));
            const auto& CompObjRoof = NewObject<UPLATEAUCityObjectGroup>(Actor,
                FName(TEST_CITYOBJ_ROOF_NAME + "__1"));

            Actor->SetActorLabel(TEST_ACTOR_NAME);
            Actor->AddInstanceComponent(SceneRoot);
            Actor->SetRootComponent(SceneRoot);
            SceneRoot->RegisterComponent();

            Actor->AddInstanceComponent(CompRoot);
            CompRoot->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
            CompRoot->RegisterComponent();
            CompRoot->SetMobility(EComponentMobility::Static);

            CompLod->AttachToComponent(CompRoot, FAttachmentTransformRules::KeepWorldTransform);
            Actor->AddInstanceComponent(CompLod);
            CompLod->RegisterComponent();
            CompLod->SetMobility(EComponentMobility::Static);

            CompObj->AttachToComponent(CompLod, FAttachmentTransformRules::KeepWorldTransform);
            Actor->AddInstanceComponent(CompObj);
            CompObj->RegisterComponent();
            CompObj->SetMobility(EComponentMobility::Static);
            CompObj->ComponentTags.Add(TEST_OBJ_TAG);

            CompObjWall->AttachToComponent(CompObj, FAttachmentTransformRules::KeepWorldTransform);
            Actor->AddInstanceComponent(CompObjWall);
            CompObjWall->RegisterComponent();
            CompObjWall->SetMobility(EComponentMobility::Static);

            CompObjRoof->AttachToComponent(CompObj, FAttachmentTransformRules::KeepWorldTransform);
            Actor->AddInstanceComponent(CompObjRoof);
            CompObjRoof->RegisterComponent();
            CompObjRoof->SetMobility(EComponentMobility::Static);

            GEngine->BroadcastLevelActorListChanged();
            return Actor;
        }

        /// <summary>
        /// Model / 各Node 生成
        /// </summary>
        inline std::shared_ptr<plateau::polygonMesh::Model> CreateModelAtomic(plateau::polygonMesh::Mesh& Mesh) {
            std::shared_ptr<plateau::polygonMesh::Model> Model = plateau::polygonMesh::Model::createModel();
            auto& NodeOP = Model->addEmptyNode(TCHAR_TO_UTF8(*TEST_OP_NAME));
            auto& NodeLod = NodeOP.addEmptyChildNode(TCHAR_TO_UTF8(*TEST_LOD_NAME));
            auto& NodeObj = NodeLod.addEmptyChildNode(TCHAR_TO_UTF8(*TEST_OBJ_NAME));
            auto& NodeWall = NodeObj.addEmptyChildNode(TCHAR_TO_UTF8(*TEST_CITYOBJ_WALL_NAME));
            auto& NodeRoof = NodeObj.addEmptyChildNode(TCHAR_TO_UTF8(*TEST_CITYOBJ_ROOF_NAME));

            auto MeshPtr = std::make_unique<plateau::polygonMesh::Mesh>(Mesh);
            NodeWall.setMesh(std::move(MeshPtr));
            auto MeshPtr2 = std::make_unique<plateau::polygonMesh::Mesh>(Mesh);
            NodeWall.setMesh(std::move(MeshPtr2));

            Model->assignNodeHierarchy();
            Model->optimizeMeshes();
            return Model;
        }

        /// <summary>
        /// FPLATEAUCityObject & GMLID : FPLATEAUCityObject のMap 生成
        /// </summary>
        inline TMap<FString, FPLATEAUCityObject> CreateCityObjectMapAtomic() {
            TMap<FString, FPLATEAUCityObject> Map;
            FPLATEAUCityObject CityObj;
            CityObj.SetGmlID(TEST_OBJ_NAME);
            CityObj.SetCityObjectsType(TEST_CITYOBJ_TYPE);
            CityObj.SetCityObjectIndex(plateau::polygonMesh::CityObjectIndex(0, -1));
            FPLATEAUCityObject CityObjWall;
            CityObjWall.SetGmlID(TEST_CITYOBJ_WALL_NAME);
            CityObjWall.SetCityObjectsType(TEST_CITYOBJ_WALL_TYPE);
            CityObjWall.SetCityObjectIndex(plateau::polygonMesh::CityObjectIndex(0, 0));
            FPLATEAUCityObject CityObjRoof;
            CityObjWall.SetGmlID(TEST_CITYOBJ_ROOF_NAME);
            CityObjWall.SetCityObjectsType(TEST_CITYOBJ_ROOF_TYPE);
            CityObjWall.SetCityObjectIndex(plateau::polygonMesh::CityObjectIndex(0, 0));
            Map.Add(TEST_OBJ_NAME, CityObj);
            return Map;
        }

        //Wall CityObject
        inline void CreateCityObjectWall(FPLATEAUCityObject& CityObj) {
            CityObj.SetGmlID(TEST_CITYOBJ_WALL_NAME);
            CityObj.SetCityObjectsType(TEST_CITYOBJ_WALL_TYPE);
            CityObj.SetCityObjectIndex(plateau::polygonMesh::CityObjectIndex(0, 0));
        }
        //Roof CityObject
        inline void CreateCityObjectRoof(FPLATEAUCityObject& CityObj) {
            CityObj.SetGmlID(TEST_CITYOBJ_ROOF_NAME);
            CityObj.SetCityObjectsType(TEST_CITYOBJ_ROOF_TYPE);
            CityObj.SetCityObjectIndex(plateau::polygonMesh::CityObjectIndex(0, 0));
        }

        //Building OutsideChildren
        inline TArray<FString> CreateCityObjectBuildingOutsideChildren() {
            TArray<FString> Children;
            Children.Add(TEST_CITYOBJ_WALL_NAME);
            Children.Add(TEST_CITYOBJ_ROOF_NAME);
            return Children;
        }

        /// <summary>
        /// 属性情報を持つNodeの取得用
        /// </summary>
        inline const plateau::polygonMesh::Node& GetObjNodeAtomic(std::shared_ptr<plateau::polygonMesh::Model> Model, int32 index) {
            const auto& root = Model->getRootNodeAt(0);
            const auto& lod = root.getChildAt(0);
            const auto& obj = lod.getChildAt(0); //bldg_00000000-aaaa-0000-0000-000000000000
            return obj.getChildAt(index);
        }
    }

    //Landscape/Heightmap用　ダイナミック生成等のテスト用共通処理
    namespace LandscapeFixtures {

        const FString TEST_DEM_OP_NAME = "000000_dem_0000_op";
        const FString TEST_DEM_OBJ_NAME = "dem_00000000-0000-0000-0000-000000000000";
        const FString TEST_DEM_CITYOBJ_TYPE = "TINRelief";

        /// <summary>
        /// Landscape用Param生成
        /// </summary>
        inline FPLATEAULandscapeParam CreateLandscapeParam() {
            FPLATEAULandscapeParam Param;
            Param.TextureWidth = 505;
            Param.TextureHeight = 505;
            Param.NumSubsections = 2;
            Param.SubsectionSizeQuads = 63;
            Param.ComponentCountX = 126;
            Param.ComponentCountY = 126;
            Param.ConvertTerrain = true;
            Param.ConvertToLandscape = true;
            Param.FillEdges = true;
            Param.ApplyBlurFilter = true;
            Param.AlignLand = true;
            Param.InvertRoadLod3 = true;
            return Param;
        }

        /// <summary>
        /// DemのCityObjectIndexのList生成
        /// </summary>
        inline void CreateCityObjectList(plateau::polygonMesh::CityObjectList& CityObj) {
            CityObj.add(plateau::polygonMesh::CityObjectIndex(0, -1), TCHAR_TO_UTF8(*TEST_DEM_OBJ_NAME));
        }

        /// <summary>
        /// Dem Model / 各Node 生成
        /// </summary>
        inline std::shared_ptr<plateau::polygonMesh::Model> CreateModel(plateau::polygonMesh::Mesh& Mesh) {
            std::shared_ptr<plateau::polygonMesh::Model> Model = plateau::polygonMesh::Model::createModel();
            auto& NodeOP = Model->addEmptyNode(TCHAR_TO_UTF8(*TEST_DEM_OP_NAME));
            auto& NodeLod = NodeOP.addEmptyChildNode(TCHAR_TO_UTF8(*PLATEAUAutomationTestUtil::Fixtures::TEST_LOD_NAME));
            auto& NodeObj = NodeLod.addEmptyChildNode(TCHAR_TO_UTF8(*TEST_DEM_OBJ_NAME));
            auto MeshPtr = std::make_unique<plateau::polygonMesh::Mesh>(Mesh);
            NodeObj.setMesh(std::move(MeshPtr));
            Model->assignNodeHierarchy();
            Model->optimizeMeshes();
            return Model;
        }

        /// <summary>
        /// Actor と DEM Compoenent生成
        /// </summary>
        inline APLATEAUInstancedCityModel* CreateActor(UWorld& World) {
            APLATEAUInstancedCityModel* Actor = World.SpawnActor<APLATEAUInstancedCityModel>();
            const auto& SceneRoot = NewObject<UPLATEAUSceneComponent>(Actor,
                USceneComponent::GetDefaultSceneRootVariableName());
            const auto& CompRoot = NewObject<UPLATEAUSceneComponent>(Actor,
                FName(TEST_DEM_OP_NAME));
            const auto& CompLod = NewObject<UPLATEAUSceneComponent>(Actor,
                FName(PLATEAUAutomationTestUtil::Fixtures::TEST_LOD_NAME + "__1"));
            const auto& CompObj = NewObject<UPLATEAUCityObjectGroup>(Actor,
                FName(TEST_DEM_OBJ_NAME + "__1"));

            Actor->SetActorLabel(PLATEAUAutomationTestUtil::Fixtures::TEST_ACTOR_NAME);
            Actor->AddInstanceComponent(SceneRoot);
            Actor->SetRootComponent(SceneRoot);
            SceneRoot->RegisterComponent();

            Actor->AddInstanceComponent(CompRoot);
            CompRoot->AttachToComponent(SceneRoot, FAttachmentTransformRules::KeepWorldTransform);
            CompRoot->RegisterComponent();
            CompRoot->SetMobility(EComponentMobility::Static);

            CompLod->AttachToComponent(CompRoot, FAttachmentTransformRules::KeepWorldTransform);
            Actor->AddInstanceComponent(CompLod);
            CompLod->RegisterComponent();
            CompLod->SetMobility(EComponentMobility::Static);

            CompObj->AttachToComponent(CompLod, FAttachmentTransformRules::KeepWorldTransform);
            Actor->AddInstanceComponent(CompObj);
            CompObj->RegisterComponent();
            CompObj->SetMobility(EComponentMobility::Static);
            CompObj->ComponentTags.Add(PLATEAUAutomationTestUtil::Fixtures::TEST_OBJ_TAG);

            GEngine->BroadcastLevelActorListChanged();
            return Actor;
        }

        /// <summary>
        /// Dem CityObject
        /// </summary>
        inline void CreateCityObjectDem(FPLATEAUCityObject& InCityObj) {
            InCityObj.SetGmlID(TEST_DEM_OBJ_NAME);
            InCityObj.SetCityObjectsType(TEST_DEM_CITYOBJ_TYPE);
            InCityObj.SetCityObjectIndex(plateau::polygonMesh::CityObjectIndex(0, -1));
        }
    }

    // Texture処理
    namespace Texture {

        //画像ロード
        inline UTexture2D* LoadImage(FString TextureName) {
            FString Path = FPLATEAURuntimeModule::GetContentDir().Append("/TestData/texture/").Append(TextureName);
            UTexture2D* texture = FImageUtils::ImportFileAsTexture2D(Path);
            return texture;
        }

        //Texture2d => Uint16 Array 変換
        inline TArray<uint16> ConvertTexture2dToUint16Array(UTexture2D* Texture) {

            if (Texture->GetPlatformData()->PixelFormat != PF_G16R16 &&
                Texture->GetPlatformData()->PixelFormat != PF_R16_UINT &&
                Texture->GetPlatformData()->PixelFormat != PF_R16F &&
                Texture->GetPlatformData()->PixelFormat != PF_G16) {
                UE_LOG(LogTemp, Warning, TEXT("Unsupported Pixel Format! "));
            }

            TArray<uint16> PixelDataArray;
            FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
            int32 TextureWidth = Mip.SizeX;
            int32 TextureHeight = Mip.SizeY;
            PixelDataArray.SetNum(TextureWidth * TextureHeight);
            void* Data = Mip.BulkData.Lock(LOCK_READ_ONLY);
            FMemory::Memcpy(PixelDataArray.GetData(), Data, TextureWidth * TextureHeight * sizeof(uint16));
            Mip.BulkData.Unlock();
            return PixelDataArray;
        }

        // Pixel FormatをStringで取得 (Log用）
        inline FString GetPixelFormatString(UTexture2D* Texture) {
            //UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EPixelFormat"), true);  //AddInfo("Name " + GetNameSafe(EnumPtr->GetOuter())); 
            UEnum* EnumPtr = FindObject<UEnum>(nullptr, TEXT("/Script/CoreUObject.EPixelFormat"), true);
            FString EnumName = EnumPtr->GetDisplayNameTextByValue(Texture->GetPlatformData()->PixelFormat).ToString();
            return EnumName;
        }
    }

    namespace CityModel {

        inline void CopyDirectory(const FString& SourceDirectory, const FString& DestinationDirectory) {
            // IPlatformFileのインスタンスを取得
            IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

            // ディレクトリが存在するか確認
            if (PlatformFile.DirectoryExists(*SourceDirectory)) {
                // コピー処理を実行
                bool bCopySuccess = PlatformFile.CopyDirectoryTree(*DestinationDirectory, *SourceDirectory, true);

                if (bCopySuccess) {
                    UE_LOG(LogTemp, Log, TEXT("Directory successfully copied from %s to %s"), *SourceDirectory, *DestinationDirectory);
                }
                else {
                    UE_LOG(LogTemp, Error, TEXT("Failed to copy directory from %s to %s"), *SourceDirectory, *DestinationDirectory);
                }
            }
            else {
                UE_LOG(LogTemp, Warning, TEXT("Source directory does not exist: %s"), *SourceDirectory);
            }
        }

        inline std::shared_ptr<const citygml::CityModel> LoadCityModel() {
            FPLATEAUCityObjectInfo GmlInfo;
            GmlInfo.DatasetName = "data";
            GmlInfo.GmlName = "53392642_bldg_6697_op2.gml";
            return UPLATEAUCityGmlProxy::Load(GmlInfo);
        }
    }

};
