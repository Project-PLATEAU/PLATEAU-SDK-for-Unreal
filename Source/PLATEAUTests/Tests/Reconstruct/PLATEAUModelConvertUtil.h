// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#pragma once
#include "CoreMinimal.h"
#include <PLATEAURuntime.h>
#include "Util/PLATEAUComponentUtil.h"
#include "Util/PLATEAUReconstructUtil.h"
#include <PLATEAUExportSettings.h>
#include <PLATEAUMeshExporter.h>
#include <CityGML/PLATEAUCityGmlProxy.h>
#include "Component/PLATEAUSceneComponent.h"
#include "PLATEAUTests/Tests/PLATEAUAutomationTestBase.h"

class ModelConvert {  
public:
    /// <summary>
    /// FPLATEAUModelReconstruct::ConvertModelWithGranularityの内部処理の再現Test
    /// </summary>
    static void TestConvertModel(FPLATEAUAutomationTestBase* Test, APLATEAUInstancedCityModel* Actor, UPLATEAUCityObjectGroup* Comp, ConvertGranularity ConvGranularity) {

        plateau::granularityConvert::GranularityConvertOption ConvOption(ConvGranularity, 0);

        FPLATEAUMeshExportOptions ExtOptions;
        ExtOptions.bExportHiddenObjects = false;
        ExtOptions.bExportTexture = true;
        ExtOptions.TransformType = EMeshTransformType::Local;
        ExtOptions.CoordinateSystem = ECoordinateSystem::ESU;

        FPLATEAUMeshExporter MeshExporter;
        plateau::granularityConvert::GranularityConverter Converter;

        std::shared_ptr<plateau::polygonMesh::Model> BaseModel = MeshExporter.CreateModelFromComponents(Actor, { Comp }, ExtOptions);
        Test->AddInfo("MeshExporter: " + FString(BaseModel->debugString().c_str()));

        std::shared_ptr<plateau::polygonMesh::Model> Converted = std::make_shared<plateau::polygonMesh::Model>(Converter.convert(*BaseModel, ConvOption));
        Test->AddInfo("GranularityConverter: " + FString(Converted->debugString().c_str()));

        Test->TestEqual("Model Root Node Name is the same", BaseModel->getRootNodeAt(0).getName(), Converted->getRootNodeAt(0).getName());
        Test->TestEqual("Model Lod Node Name is the same", BaseModel->getRootNodeAt(0).getChildAt(0).getName(), Converted->getRootNodeAt(0).getChildAt(0).getName());
    }
};

