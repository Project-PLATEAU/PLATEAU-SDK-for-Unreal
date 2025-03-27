// Copyright © 2023 Ministry of Land, Infrastructure and Transport


#include "ModelClassification/PLATEAUModelClassificationAPI.h"
#include "CityGML/PLATEAUCityObject.h"
#include "Component/PLATEAUCityObjectGroup.h"
#include "PLATEAURuntime/Public/PLATEAUInstancedCityModel.h"
#include "Algo/Sort.h"
#include "Misc/MessageDialog.h"

using namespace Algo;

namespace {

    TSet<EPLATEAUCityObjectsType> GetAllTypesInComponent(USceneComponent* Component) {
        TSet<EPLATEAUCityObjectsType> UniqueTypes;
        if (Component->IsA(UPLATEAUCityObjectGroup::StaticClass()) && Component->IsVisible()) {
            auto CompCityObj = StaticCast<UPLATEAUCityObjectGroup*>(Component);
            if (CompCityObj->GetStaticMesh() != nullptr) {
                for (const auto CityObj : CompCityObj->GetAllRootCityObjects()) {
                    if(CityObj.Children.Num() == 0)
                        UniqueTypes.Add(CityObj.Type);
                    for (const auto child : CityObj.Children) {
                        UniqueTypes.Add(child.Type);
                    }
                }
            }
        }
        return UniqueTypes;
    }

    void AddAttributeKey(FString Key, FPLATEAUAttributeValue Value, TSet<FString> &Set, FString ParentKey = FString()) {

        if (Value.Type == EPLATEAUAttributeType::AttributeSets) {
            ParentKey = ParentKey.IsEmpty() ? Key + "/" : ParentKey + "/" + Key + "/";
            for (auto& attr : Value.Attributes->AttributeMap) {
                AddAttributeKey(attr.Key, attr.Value, Set, ParentKey);
            }
        }
        else {
            Set.Emplace(ParentKey + Key);
        }
    }

    TSet<FString> GetAllAttrKeysInComponent(USceneComponent* Component) {
        TSet<FString> UniqueKeys;
        if (Component->IsA(UPLATEAUCityObjectGroup::StaticClass()) && Component->IsVisible()) {
            auto CompCityObj = StaticCast<UPLATEAUCityObjectGroup*>(Component);
            for (const auto CityObj : CompCityObj->GetAllRootCityObjects()) {
                for (auto& attr : CityObj.Attributes.AttributeMap){
                    AddAttributeKey(attr.Key, attr.Value, UniqueKeys);
                }
                for (const auto child : CityObj.Children) {
                    for (auto& attr : child.Attributes.AttributeMap) {
                        AddAttributeKey(attr.Key, attr.Value, UniqueKeys);
                    }    
                }
            }
        }
        return UniqueKeys;
    }

    TSet<FString> GetAllAttrStringValuesByKeyInComponent(USceneComponent* Component, FString Key) {
        TSet<FString> Values;
        if (Component->IsA(UPLATEAUCityObjectGroup::StaticClass()) && Component->IsVisible()) {
            auto CompCityObj = StaticCast<UPLATEAUCityObjectGroup*>(Component);
            for (const auto CityObj : CompCityObj->GetAllRootCityObjects()) {
                const auto& Attrs = UPLATEAUAttributeValueBlueprintLibrary::GetAttributesByKey(Key, CityObj.Attributes);
                for (const auto Attr : Attrs) {
                    Values.Add(Attr.StringValue);
                }

                for (const auto Child : CityObj.Children) {
                    const auto& ChildAttrs = UPLATEAUAttributeValueBlueprintLibrary::GetAttributesByKey(Key, Child.Attributes);
                    for (const auto Attr : ChildAttrs) {
                        Values.Add(Attr.StringValue);
                    }
                }
            }
        }
        return Values;
    }

}

TSet<FString> UPLATEAUModelClassificationAPI::SearchAttributeKeys(const TArray<USceneComponent*> TargetComponents) {
    TSet<FString> UniqueKeys;
    for (const auto comp : TargetComponents) {
        if (comp->IsA(UActorComponent::StaticClass()) || comp->IsA(UStaticMeshComponent::StaticClass()) && comp->IsVisible()) {
            UniqueKeys.Append(GetAllAttrKeysInComponent(comp));
            TArray<USceneComponent*> children;
            comp->GetChildrenComponents(true, children);
            for (const auto child : children) {
                UniqueKeys.Append(GetAllAttrKeysInComponent(child));
            }
        }
    }
    return UniqueKeys;
}

TSet<FString> UPLATEAUModelClassificationAPI::SearchAttributeStringValuesFromKey(const TArray<USceneComponent*> TargetComponents, FString Key) {

    TArray<FString> Keys;
    Key.ParseIntoArray(Keys, TEXT("/"));

    TSet<FString> StringValues;
    for (const auto comp : TargetComponents) {
        if (comp->IsA(UActorComponent::StaticClass()) || comp->IsA(UStaticMeshComponent::StaticClass()) && comp->IsVisible()) {
            if (comp->IsA(UPLATEAUCityObjectGroup::StaticClass()) && comp->IsVisible()) {
                StringValues.Append(GetAllAttrStringValuesByKeyInComponent(comp, Key));
            }

            TArray<USceneComponent*> children;
            comp->GetChildrenComponents(true, children);
            for (const auto child : children) {
                StringValues.Append(GetAllAttrStringValuesByKeyInComponent(child, Key));
            }
        }
    }
    return StringValues;
}

TArray<FString> UPLATEAUModelClassificationAPI::SortAttributeStringValues(const TArray<FString> InStrings) {
    TArray<FString> OutStrings = TArray<FString>(InStrings);
    OutStrings.Sort([](const FString& str1, const FString& str2) {
        return FCString::Atoi(*str1) < FCString::Atoi(*str2);
        });
    return OutStrings;
}

TSet<EPLATEAUCityObjectsType> UPLATEAUModelClassificationAPI::SearchTypes(const TArray<USceneComponent*> TargetComponents) {

    TSet<EPLATEAUCityObjectsType> UniqueTypes;
    for (const auto comp : TargetComponents) {
        if (comp->IsA(UActorComponent::StaticClass()) || comp->IsA(UStaticMeshComponent::StaticClass()) && StaticCast<UStaticMeshComponent*>(comp)->GetStaticMesh() == nullptr && comp->IsVisible()) {
            UniqueTypes.Append(GetAllTypesInComponent(comp));
            TArray<USceneComponent*> children;
            comp->GetChildrenComponents(true, children);
            for (const auto child : children) {
                UniqueTypes.Append(GetAllTypesInComponent(child));
            }
        }     
    }
    return UniqueTypes;
}

void UPLATEAUModelClassificationAPI::ClassifyByType(APLATEAUInstancedCityModel* TargetCityModel, TArray<USceneComponent*> TargetComponents, TMap<EPLATEAUCityObjectsType, UMaterialInterface*> Materials, const EPLATEAUMeshGranularity ReconstructType, bool bDestroyOriginal, UMaterialInterface* DefaultMaterial) {
        
#if WITH_EDITOR
    TargetCityModel->ClassifyModel(TargetComponents, Materials, ReconstructType, bDestroyOriginal, DefaultMaterial);
#else
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("この機能は、エディタのみでご利用いただけます。")));
#endif  
}


void UPLATEAUModelClassificationAPI::ClassifyByAttribute(APLATEAUInstancedCityModel * TargetCityModel, TArray<USceneComponent*> TargetComponents, FString AttributeKey, TMap<FString, UMaterialInterface*> Materials, const EPLATEAUMeshGranularity ReconstructType, bool bDestroyOriginal, UMaterialInterface* DefaultMaterial) {
#if WITH_EDITOR
    TargetCityModel->ClassifyModel(TargetComponents, AttributeKey, Materials, ReconstructType, bDestroyOriginal, DefaultMaterial);
#else
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("この機能は、エディタのみでご利用いただけます。")));
#endif  
}