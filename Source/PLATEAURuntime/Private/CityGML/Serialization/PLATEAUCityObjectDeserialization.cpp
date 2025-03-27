// Copyright 2023 Ministry of Land, Infrastructure and Transport

#include "CityGML/Serialization/PLATEAUCityObjectDeserialization.h"
#include <Component/PLATEAUCityObjectGroup.h>
#include "Util/PLATEAUGmlUtil.h"


void FPLATEAUCityObjectDeserialization::DeserializeCityObjects(const FString InSerializedCityObjects, const TArray<TObjectPtr<USceneComponent>> InAttachChildren, 
    TArray<FPLATEAUCityObject>& OutRootCityObjects, FString& OutOutsideParent) {

    TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(InSerializedCityObjects);
    TSharedPtr<FJsonObject> JsonRootObject;
    FJsonSerializer::Deserialize(JsonReader, JsonRootObject);

    const auto& CityObjectsJsonArray = JsonRootObject->GetArrayField(plateau::CityObjectGroup::CityObjectsFieldName);
    for (const auto& CityJsonValue : CityObjectsJsonArray) {
        OutRootCityObjects.Emplace(GetCityObject(CityJsonValue));
    }

    OutOutsideParent = JsonRootObject->GetStringField(plateau::CityObjectGroup::OutsideParentFieldName);

    // 最小地物単位
    const auto& OutsideChildrenJsonArray = JsonRootObject->GetArrayField(plateau::CityObjectGroup::OutsideChildrenFieldName);
    if (0 < OutsideChildrenJsonArray.Num() && 0 < OutRootCityObjects.Num()) {
        for (const auto& ChildComponent : InAttachChildren) {
            const auto& PLATEAUCityObjectGroup = Cast<UPLATEAUCityObjectGroup>(ChildComponent);
            OutRootCityObjects[0].Children.Append(PLATEAUCityObjectGroup->GetAllRootCityObjects());
        }
    }
}

/**
* @brief シティオブジェクトからシリアライズに必要な情報を抽出してJsonValue配列として返却
* @param InCityObject CityModelから得られるシティオブジェクト情報
* @param CityObjectIndex CityObjectListが持つインデックス情報
* @return シティオブジェクト情報
*/

FPLATEAUCityObject FPLATEAUCityObjectDeserialization::GetCityObject(TSharedPtr<FJsonValue> CityJsonValue) {
    FPLATEAUCityObject CityObject;
    const auto& CityJsonObject = CityJsonValue->AsObject();
    CityObject.SetGmlID(CityJsonObject->GetStringField(plateau::CityObjectGroup::GmlIdFieldName));

    const auto& CityObjectIndexJsonValueArray = CityJsonObject->GetArrayField(plateau::CityObjectGroup::CityObjectIndexFieldName);
    const auto PrimaryIndexJsonValue = CityObjectIndexJsonValueArray[0];
    const auto AtomicIndexJsonValue = CityObjectIndexJsonValueArray[1];
    plateau::polygonMesh::CityObjectIndex CityObjectIndex{
        static_cast<int32>(PrimaryIndexJsonValue->AsNumber()),
        static_cast<int32>(AtomicIndexJsonValue->AsNumber())
    };
    CityObject.SetCityObjectIndex(CityObjectIndex);

    CityObject.SetCityObjectsType(CityJsonObject->GetStringField(plateau::CityObjectGroup::CityObjectTypeFieldName));

    TMap<FString, FPLATEAUAttributeValue> AttributeMap;
    const auto& AttributesJsonValueArray = CityJsonObject->GetArrayField(plateau::CityObjectGroup::AttributesFieldName);
    for (const auto& AttributeJsonValue : AttributesJsonValueArray) {
        const auto& AttributeJsonObject = AttributeJsonValue->AsObject();
        const auto& AttributeKey = AttributeJsonObject->GetStringField(plateau::CityObjectGroup::KeyFieldName);
        const auto& AttributeType = AttributeJsonObject->GetStringField(plateau::CityObjectGroup::TypeFieldName);

        FPLATEAUAttributeValue PLATEAUAttributeValue;
        PLATEAUAttributeValue.SetType(AttributeType);
        PLATEAUAttributeValue.SetValue(PLATEAUAttributeValue.Type, AttributeJsonObject);
        AttributeMap.Add(AttributeKey, PLATEAUAttributeValue);
    }
    CityObject.SetAttribute(AttributeMap);

    const TArray<TSharedPtr<FJsonValue>>* CityObjectsChildren;
    if (CityJsonObject->TryGetArrayField(plateau::CityObjectGroup::ChildrenFieldName, CityObjectsChildren)) {
        for (const auto& CityObjectChild : *CityObjectsChildren) {
            CityObject.Children.Emplace(GetCityObject(CityObjectChild));
        }
    }

    return CityObject;
}
