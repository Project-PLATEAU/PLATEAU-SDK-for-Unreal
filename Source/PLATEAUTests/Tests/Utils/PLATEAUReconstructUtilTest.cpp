// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "FileHelpers.h"
#include "PLATEAUTests/Tests/PLATEAUAutomationTestBase.h"
#include "Util/PLATEAUReconstructUtil.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include <PLATEAURuntime.h>

using LibMeshGranularity = plateau::polygonMesh::MeshGranularity;


IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPLATEAUTest_Util_Reconstruct_Util, FPLATEAUAutomationTestBase, "PLATEAUTest.FPLATEAUTest.Util.ReconstructUtil", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPLATEAUTest_Util_Reconstruct_Util::RunTest(const FString& Parameters) {
    InitializeTest("ReconstructUtil");

    //plateau::granularityConvert::ConvertGranularity -> plateau::polygonMesh::MeshGranularity
    LibMeshGranularity MeshGr = FPLATEAUReconstructUtil::ConvertGranularityToMeshGranularity(ConvertGranularity::PerAtomicFeatureObject);
    TestEqual("ConvGr to Mesh Gr", MeshGr, LibMeshGranularity::PerAtomicFeatureObject);

    MeshGr = FPLATEAUReconstructUtil::ConvertGranularityToMeshGranularity(ConvertGranularity::PerCityModelArea);
    TestEqual("ConvGr to Mesh Gr", MeshGr, LibMeshGranularity::PerCityModelArea);

    MeshGr = FPLATEAUReconstructUtil::ConvertGranularityToMeshGranularity(ConvertGranularity::PerPrimaryFeatureObject);
    TestEqual("ConvGr to Mesh Gr", MeshGr, LibMeshGranularity::PerPrimaryFeatureObject);

    MeshGr = FPLATEAUReconstructUtil::ConvertGranularityToMeshGranularity(ConvertGranularity::MaterialInPrimary);
    TestEqual("ConvGr to Mesh Gr", MeshGr, LibMeshGranularity::PerAtomicFeatureObject);

    //plateau::polygonMesh::EPLATEAUMeshGranularity -> plateau::granularityConvert::ConvertGranularity
    ConvertGranularity ConvGr = FPLATEAUReconstructUtil::GetConvertGranularityFromReconstructType(EPLATEAUMeshGranularity::PerAtomicFeatureObject);
    TestEqual("EMeshGr to ConvGr", ConvGr, ConvertGranularity::PerAtomicFeatureObject);

    ConvGr = FPLATEAUReconstructUtil::GetConvertGranularityFromReconstructType(EPLATEAUMeshGranularity::PerCityModelArea);
    TestEqual("EMeshGr to ConvGr", ConvGr, ConvertGranularity::PerCityModelArea);

    ConvGr = FPLATEAUReconstructUtil::GetConvertGranularityFromReconstructType(EPLATEAUMeshGranularity::PerPrimaryFeatureObject);
    TestEqual("EMeshGr to ConvGr", ConvGr, ConvertGranularity::PerPrimaryFeatureObject);

    ConvGr = FPLATEAUReconstructUtil::GetConvertGranularityFromReconstructType(EPLATEAUMeshGranularity::PerMaterialInPrimary);
    TestEqual("EMeshGr to ConvGr", ConvGr, ConvertGranularity::MaterialInPrimary);

    ConvGr = FPLATEAUReconstructUtil::GetConvertGranularityFromReconstructType(EPLATEAUMeshGranularity::DoNotChange);
    TestEqual("EMeshGr to ConvGr", ConvGr, ConvertGranularity::PerPrimaryFeatureObject);

    return true;
}
