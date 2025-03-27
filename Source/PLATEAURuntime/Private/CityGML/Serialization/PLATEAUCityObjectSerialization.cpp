// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "CityGML/Serialization/PLATEAUCityObjectSerialization.h"
#include <Component/PLATEAUCityObjectGroup.h>
#include "Util/PLATEAUGmlUtil.h"

FString FPLATEAUCityObjectSerialization::SerializeCityObject(const FString& InNodeName, const plateau::polygonMesh::Mesh& InMesh,
    const plateau::polygonMesh::MeshGranularity& Granularity, TMap<FString, FPLATEAUCityObject> CityObjMap) {

    const auto& CityObjectList = InMesh.getCityObjectList();
    const std::vector<plateau::polygonMesh::CityObjectIndex> CityObjectIndices = *CityObjectList.getAllKeys();
    const TSharedPtr<FJsonObject> JsonRootObject = MakeShareable(new FJsonObject);
    JsonRootObject->SetStringField(plateau::CityObjectGroup::OutsideParentFieldName, "");
    JsonRootObject->SetArrayField(plateau::CityObjectGroup::OutsideChildrenFieldName, {});

    // 最小地物単位の親を求める（主要地物のIDを設定）
    if (plateau::polygonMesh::MeshGranularity::PerAtomicFeatureObject == Granularity) {
        for (const auto& CityObjectIndex : CityObjectIndices) {
            const FString& AtomicGmlId = FString(CityObjectList.getAtomicGmlID(CityObjectIndex).c_str());
            if (AtomicGmlId != InNodeName) {
                JsonRootObject->SetStringField(plateau::CityObjectGroup::OutsideParentFieldName, AtomicGmlId);
            }
        }
    }

    if (plateau::polygonMesh::MeshGranularity::PerCityModelArea == Granularity) {
        // 地域単位
        TArray<TSharedPtr<FJsonValue>> CityObjectJsonArray;
        TSharedPtr<FJsonObject> CityJsonObjectParent = MakeShareable(new FJsonObject);
        TArray<TSharedPtr<FJsonValue>> CityJsonObjectChildren;
        int CurrentPrimaryIndex = -1;
        for (int i = 0; i < CityObjectIndices.size(); i++) {
            const auto& CityObjectIndex = CityObjectIndices[i];
            const auto& AtomicGmlId = FString(CityObjectList.getAtomicGmlID(CityObjectIndex).c_str());

            const auto& CityObjRef = CityObjMap.Find(AtomicGmlId);
            if (CityObjRef == nullptr)
                continue;

            if (CityObjectIndex.primary_index != CurrentPrimaryIndex) {
                // 主要地物
                CurrentPrimaryIndex = CityObjectIndex.primary_index;
                CityJsonObjectParent = GetCityJsonObject(*CityObjRef, CityObjectIndex);
                CityObjectJsonArray.Emplace(MakeShared<FJsonValueObject>(CityJsonObjectParent));
                CityJsonObjectChildren.Empty();
            }
            else {
                // 最小地物
                CityJsonObjectChildren.Emplace(MakeShared<FJsonValueObject>(GetCityJsonObject(*CityObjRef, CityObjectIndex)));
            }

            // データの区切りか？
            if (i + 1 == CityObjectIndices.size() || CityObjectIndices[i + 1].primary_index != CurrentPrimaryIndex) {
                CityJsonObjectParent->SetArrayField(plateau::CityObjectGroup::ChildrenFieldName, CityJsonObjectChildren);
            }
        }
        JsonRootObject->SetArrayField(plateau::CityObjectGroup::CityObjectsFieldName, CityObjectJsonArray);
    }
    else {
        // 最小地物単位・主要地物単位共通
        const auto& CityObjParentRef = CityObjMap.Find(InNodeName);
        if (CityObjParentRef != nullptr) {
            const auto& CityObjectParent = *CityObjParentRef;
            const auto& CityObjectParentIndex = CityObjectList.getCityObjectIndex(TCHAR_TO_UTF8(*InNodeName));

            const auto& CityJsonObjectParent = GetCityJsonObject(CityObjectParent, CityObjectParentIndex);
            TArray<TSharedPtr<FJsonValue>> CityObjectJsonArray;
            CityObjectJsonArray.Emplace(MakeShared<FJsonValueObject>(CityJsonObjectParent));

            if (plateau::polygonMesh::MeshGranularity::PerPrimaryFeatureObject == Granularity) {
                TArray<TSharedPtr<FJsonValue>> CityObjectsChildrenJsonArray;
                for (const auto& CityObjectIndex : CityObjectIndices) {
                    const auto& AtomicGmlId = FString(CityObjectList.getAtomicGmlID(CityObjectIndex).c_str());
                    if (AtomicGmlId == InNodeName)
                        // 親は前の処理で既に情報抽出済み
                        continue;

                    const auto& CityObjRef = CityObjMap.Find(AtomicGmlId);
                    if (CityObjRef == nullptr)
                        continue;

                    const auto& CityObject = *CityObjRef;
                    CityObjectsChildrenJsonArray.Emplace(MakeShared<FJsonValueObject>(GetCityJsonObject(CityObject, CityObjectIndex)));
                }
                CityJsonObjectParent->SetArrayField(plateau::CityObjectGroup::ChildrenFieldName, CityObjectsChildrenJsonArray);
            }
            JsonRootObject->SetArrayField(plateau::CityObjectGroup::CityObjectsFieldName, CityObjectJsonArray);
        }
    }

    FString SerializedCityObjects;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&SerializedCityObjects);
    FJsonSerializer::Serialize(JsonRootObject.ToSharedRef(), Writer);
    return SerializedCityObjects;
}

FString FPLATEAUCityObjectSerialization::SerializeCityObject(const plateau::polygonMesh::Node& InNode, const FPLATEAUCityObject& InCityObject) {
    const TSharedPtr<FJsonObject> JsonRootObject = MakeShareable(new FJsonObject);
    // 親はなし
    JsonRootObject->SetStringField(plateau::CityObjectGroup::OutsideParentFieldName, "");

    // Outside子コンポーネント名取得
    TArray<TSharedPtr<FJsonValue>> OutsideChildrenJsonArray;
    for (int i = 0; i < InNode.getChildCount(); i++) {
        OutsideChildrenJsonArray.Emplace(MakeShared<FJsonValueString>(UTF8_TO_TCHAR(InNode.getChildAt(i).getName().c_str())));
    }
    JsonRootObject->SetArrayField(plateau::CityObjectGroup::OutsideChildrenFieldName, OutsideChildrenJsonArray);

    // CityObjects取得
    TArray<TSharedPtr<FJsonValue>> CityObjectsJsonArray;
    CityObjectsJsonArray.Emplace(MakeShared<FJsonValueObject>(GetCityJsonObject(InCityObject)));
    JsonRootObject->SetArrayField(plateau::CityObjectGroup::CityObjectsFieldName, CityObjectsJsonArray);

    // Json書き出し
    FString SerializedCityObjects;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&SerializedCityObjects);
    FJsonSerializer::Serialize(JsonRootObject.ToSharedRef(), Writer);
    return SerializedCityObjects;
}

FString FPLATEAUCityObjectSerialization::SerializeCityObject(const FPLATEAUCityObject& InCityObject, const FString InOutsideParent, const TArray<FString> InOutsideChildren) {
    const TSharedPtr<FJsonObject> JsonRootObject = MakeShareable(new FJsonObject);

    JsonRootObject->SetStringField(plateau::CityObjectGroup::OutsideParentFieldName, InOutsideParent);

    // 子コンポーネント名取得
    TArray<TSharedPtr<FJsonValue>> OutsideChildrenJsonArray;
    for (const auto& OutsideChild : InOutsideChildren) {
        OutsideChildrenJsonArray.Emplace(MakeShared<FJsonValueString>(OutsideChild));
    }
    JsonRootObject->SetArrayField(plateau::CityObjectGroup::OutsideChildrenFieldName, OutsideChildrenJsonArray);

    // CityObjects取得
    TArray<TSharedPtr<FJsonValue>> CityObjectsJsonArray;
    CityObjectsJsonArray.Emplace(MakeShared<FJsonValueObject>(GetCityJsonObjectWithChildren(InCityObject)));
    JsonRootObject->SetArrayField(plateau::CityObjectGroup::CityObjectsFieldName, CityObjectsJsonArray);

    // Json書き出し
    FString SerializedCityObjects;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&SerializedCityObjects);
    FJsonSerializer::Serialize(JsonRootObject.ToSharedRef(), Writer);
    return SerializedCityObjects;
}


/**
 * @brief 再帰的に属性マップから属性情報を取得
 * @param InAttributesMap 属性マップ
 * @param InAttributesJsonObjectArray 属性情報を格納する配列
 */
void FPLATEAUCityObjectSerialization::GetAttributesJsonObjectRecursive(const FPLATEAUAttributeMap& InAttributesMap, TArray<TSharedPtr<FJsonValue>>& InAttributesJsonObjectArray) {
    for (const auto& [key, value] : InAttributesMap.AttributeMap) {
        TSharedRef<FJsonObject> AttributesJsonObject = MakeShared<FJsonObject>();
        if (EPLATEAUAttributeType::AttributeSets == value.Type) {
            TArray<TSharedPtr<FJsonValue>> AttributeSetJsonObjectArray;
            GetAttributesJsonObjectRecursive(*value.Attributes.Get(), AttributeSetJsonObjectArray);
            AttributesJsonObject->SetStringField(plateau::CityObjectGroup::KeyFieldName, key);
            AttributesJsonObject->SetStringField(plateau::CityObjectGroup::TypeFieldName, "AttributeSets");
            AttributesJsonObject->SetArrayField(plateau::CityObjectGroup::ValueFieldName, AttributeSetJsonObjectArray);
            InAttributesJsonObjectArray.Emplace(MakeShared<FJsonValueObject>(AttributesJsonObject));
        }
        else {
            AttributesJsonObject->SetStringField(plateau::CityObjectGroup::KeyFieldName, key);

            switch (value.Type) {
            case EPLATEAUAttributeType::String:
                AttributesJsonObject->SetStringField(plateau::CityObjectGroup::TypeFieldName, "String");
                break;
            case EPLATEAUAttributeType::Double:
                AttributesJsonObject->SetStringField(plateau::CityObjectGroup::TypeFieldName, "Double");
                break;
            case EPLATEAUAttributeType::Integer:
                AttributesJsonObject->SetStringField(plateau::CityObjectGroup::TypeFieldName, "Integer");
                break;
            case EPLATEAUAttributeType::Date:
                AttributesJsonObject->SetStringField(plateau::CityObjectGroup::TypeFieldName, "Date");
                break;
            case EPLATEAUAttributeType::Uri:
                AttributesJsonObject->SetStringField(plateau::CityObjectGroup::TypeFieldName, "Uri");
                break;
            case EPLATEAUAttributeType::Measure:
                AttributesJsonObject->SetStringField(plateau::CityObjectGroup::TypeFieldName, "Measure");
                break;
            case EPLATEAUAttributeType::Boolean:
                AttributesJsonObject->SetStringField(plateau::CityObjectGroup::TypeFieldName, "Boolean");
                break;
            default: UE_LOG(LogTemp, Log, TEXT("Error citygml::AttributeType"));
            }

            AttributesJsonObject->SetStringField(plateau::CityObjectGroup::ValueFieldName, value.StringValue);
            InAttributesJsonObjectArray.Emplace(MakeShared<FJsonValueObject>(AttributesJsonObject));
        }
    }
}

/**
 * @brief シティオブジェクトからシリアライズに必要な情報を抽出してJsonValue配列として返却
 * @param InCityObject CityModelから得られるシティオブジェクト情報
 * @return シティオブジェクト情報
 */
TSharedRef<FJsonObject> FPLATEAUCityObjectSerialization::GetCityJsonObject(const FPLATEAUCityObject& InCityObject) {
    TSharedRef<FJsonObject> CityJsonObject = MakeShared<FJsonObject>();

    CityJsonObject->SetStringField(plateau::CityObjectGroup::GmlIdFieldName, InCityObject.GmlID);

    TArray<TSharedPtr<FJsonValue>> CityObjectIndexArray;
    CityObjectIndexArray.Emplace(MakeShared<FJsonValueNumber>(InCityObject.CityObjectIndex.PrimaryIndex));
    CityObjectIndexArray.Emplace(MakeShared<FJsonValueNumber>(InCityObject.CityObjectIndex.AtomicIndex));
    CityJsonObject->SetArrayField(plateau::CityObjectGroup::CityObjectIndexFieldName, CityObjectIndexArray);

    CityJsonObject->SetStringField(plateau::CityObjectGroup::CityObjectTypeFieldName, plateau::CityObject::CityObjectsTypeToString(InCityObject.Type));

    TArray<TSharedPtr<FJsonValue>> AttributesJsonObjectArray;
    GetAttributesJsonObjectRecursive(InCityObject.Attributes, AttributesJsonObjectArray);
    CityJsonObject->SetArrayField(plateau::CityObjectGroup::AttributesFieldName, AttributesJsonObjectArray);

    return CityJsonObject;
}

/**
 * @brief シティオブジェクトからシリアライズに必要な情報を抽出してJsonValue配列として返却
 * @param InCityObject CityModelから得られるシティオブジェクト情報
 * @param CityObjectIndex CityObjectListが持つインデックス情報
 * @return シティオブジェクト情報
 */
TSharedRef<FJsonObject> FPLATEAUCityObjectSerialization::GetCityJsonObject(const FPLATEAUCityObject& InCityObject, const plateau::polygonMesh::CityObjectIndex& CityObjectIndex) {
    TSharedRef<FJsonObject> CityJsonObject = MakeShared<FJsonObject>();

    CityJsonObject->SetStringField(plateau::CityObjectGroup::GmlIdFieldName, InCityObject.GmlID);

    TArray<TSharedPtr<FJsonValue>> CityObjectIndexArray;
    CityObjectIndexArray.Emplace(MakeShared<FJsonValueNumber>(CityObjectIndex.primary_index));
    CityObjectIndexArray.Emplace(MakeShared<FJsonValueNumber>(CityObjectIndex.atomic_index));
    CityJsonObject->SetArrayField(plateau::CityObjectGroup::CityObjectIndexFieldName, CityObjectIndexArray);

    CityJsonObject->SetStringField(plateau::CityObjectGroup::CityObjectTypeFieldName, plateau::CityObject::CityObjectsTypeToString(InCityObject.Type));

    TArray<TSharedPtr<FJsonValue>> AttributesJsonObjectArray;
    GetAttributesJsonObjectRecursive(InCityObject.Attributes, AttributesJsonObjectArray);
    CityJsonObject->SetArrayField(plateau::CityObjectGroup::AttributesFieldName, AttributesJsonObjectArray);

    return CityJsonObject;
}


/**
 * @brief Childを含めてシリアライズ
 * @param InCityObject FPLATEAUCityObject
 * @return シティオブジェクト情報
 */
TSharedRef<FJsonObject> FPLATEAUCityObjectSerialization::GetCityJsonObjectWithChildren(const FPLATEAUCityObject& InCityObject) {
    TSharedRef<FJsonObject> CityJsonObject = GetCityJsonObject(InCityObject);
    TArray<TSharedPtr<FJsonValue>> CityJsonObjectChildren;

    for (const auto& Child : InCityObject.Children)
        CityJsonObjectChildren.Emplace(MakeShared<FJsonValueObject>(GetCityJsonObject(Child)));

    CityJsonObject->SetArrayField(plateau::CityObjectGroup::ChildrenFieldName, CityJsonObjectChildren);
    return CityJsonObject;
}

