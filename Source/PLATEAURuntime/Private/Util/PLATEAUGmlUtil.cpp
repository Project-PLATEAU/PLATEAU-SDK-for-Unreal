// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "Util/PLATEAUGmlUtil.h"
#include "Util/PLATEAUComponentUtil.h"
#include <CityGML/citymodel.h>
#include <plateau/dataset/i_dataset_accessor.h>
#include "Misc/EngineVersionComparison.h"

namespace {

    void GetParentNodeNameRecursive(const plateau::polygonMesh::Node& InNode, TArray<FString>& NodePath) {
        NodePath.Add(FString(InNode.getName().c_str()));
        if (InNode.hasParentNode()) {
            GetParentNodeNameRecursive(InNode.getParentNode(), NodePath);
        }
    }

    void GetChildrenGmlIdsRecursive(const FPLATEAUCityObject CityObj, TSet<FString>& IdList) {
        for (auto child : CityObj.Children) {
            IdList.Add(child.GmlID);
            GetChildrenGmlIdsRecursive(child, IdList);
        }
    }

    void GetCityObjectRecursive(const citygml::CityObject* InCityObject, TMap<FString, FPLATEAUCityObject>& OutMap) {
        FPLATEAUCityObject ConvertedCityObject;
        FPLATEAUGmlUtil::ConvertCityObject(InCityObject, ConvertedCityObject);
        OutMap.Add(ConvertedCityObject.GmlID, ConvertedCityObject);
        for (unsigned int i = 0; i < InCityObject->getChildCityObjectsCount(); i++) {
            const auto& childCityObject = InCityObject->getChildCityObject(i);
            GetCityObjectRecursive(&childCityObject, OutMap);
        }
    }
}

/**
* @brief 3D都市モデル内のCityGMLファイルに相当するコンポーネントを入力として、CityGMLファイル名を返します。
* @return CityGMLファイル名
*/
FString FPLATEAUGmlUtil::GetGmlFileName(const USceneComponent* const InGmlComponent) {
    return InGmlComponent->GetName().Append(".gml");
}

/**
 * @brief Gmlコンポーネントのパッケージ情報を取得します。
 */
plateau::dataset::PredefinedCityModelPackage FPLATEAUGmlUtil::GetCityModelPackage(const USceneComponent* const InGmlComponent) {
    const auto GmlFileName = GetGmlFileName(InGmlComponent);
    // udxのサブフォルダ名は地物種類名に相当するため、UdxSubFolderの関数を使用してgmlのパッケージ種を取得
    return plateau::dataset::UdxSubFolder::getPackage(plateau::dataset::GmlFile(TCHAR_TO_UTF8(*GmlFileName)).getFeatureType());
}

FString FPLATEAUGmlUtil::GetNodePathString(const USceneComponent* Component) {
    FString Path;
    TArray<USceneComponent*> Parents;
    Component->GetParentComponents(Parents);
    Parents.Remove(Component->GetAttachmentRoot());
#if UE_VERSION_NEWER_THAN(5, 5, 0)
    const auto& Op = Parents.Pop(EAllowShrinking::Yes);
#else
    const auto& Op = Parents.Pop(true);
#endif
    Algo::Reverse(Parents);

    Path = Op->GetName() + "/"; //Opはsuffixを処理しない
    for (auto P : Parents) {
        Path += FPLATEAUComponentUtil::GetOriginalComponentName(P) + "/";
    }
    Path += FPLATEAUComponentUtil::GetOriginalComponentName(Component);
    return Path;
}

FString FPLATEAUGmlUtil::GetNodePathString(TArray<FString> NodePath) {
    FString NodePathStr;
    for (auto N : NodePath)
        NodePathStr += N + "/";
    return NodePathStr.LeftChop(1);
}

FString FPLATEAUGmlUtil::GetNodePathString(const plateau::polygonMesh::Node& InNode) {
    TArray<FString> NodePath;
    GetParentNodeNameRecursive(InNode, NodePath);
    Algo::Reverse(NodePath);
    return GetNodePathString(NodePath);
}

TSet<FString> FPLATEAUGmlUtil::GetChildrenGmlIds(const FPLATEAUCityObject CityObj) {
    TSet<FString> IdList;
    for (auto child : CityObj.Children) {
        IdList.Add(child.GmlID);
        GetChildrenGmlIdsRecursive(child, IdList);
    }
    return IdList;
}

TMap<FString, FPLATEAUCityObject> FPLATEAUGmlUtil::CreateMapFromCityModel(const std::shared_ptr<const citygml::CityModel> InCityModel) {
    TMap<FString, FPLATEAUCityObject> OutMap;
    const auto& rootCityObjects = InCityModel->getRootCityObjects();
    for (const auto& cityObject : rootCityObjects) {
        GetCityObjectRecursive(cityObject, OutMap);
    }
    return OutMap;
}

void FPLATEAUGmlUtil::ConvertCityObject(const citygml::CityObject* InCityObject, FPLATEAUCityObject& OutCityObject) {
    OutCityObject.SetGmlID(UTF8_TO_TCHAR(InCityObject->getId().c_str()));
    OutCityObject.SetCityObjectsType(plateau::CityObject::CityObjectsTypeToString(InCityObject->getType()));

    FPLATEAUAttributeMap AttributeMap;
    for (const auto& [key, value] : InCityObject->getAttributes()) {
        FPLATEAUAttributeValue PLATEAUAttributeValue;
        PLATEAUAttributeValue.SetAttributeValue(value);
        AttributeMap.AttributeMap.Add(UTF8_TO_TCHAR(key.c_str()), PLATEAUAttributeValue);
    }
    OutCityObject.SetAttribute(AttributeMap.AttributeMap);
}