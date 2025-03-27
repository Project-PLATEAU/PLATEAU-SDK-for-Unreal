// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PLATEAUAttrInfoDrawGizmo.generated.h"


USTRUCT(BlueprintType)
struct FVertexData {
    GENERATED_BODY()

    FVertexData() : VertexID(), VertexPos(FVector::Zero()) {
    }

    FVertexData(const FVertexID InVertexID, const FVector InVertexPos) : VertexID(InVertexID), VertexPos(InVertexPos) {
    }

    UPROPERTY(BlueprintReadWrite, Category = "PLATEAU|BPLibraries")
    FVertexID VertexID;

    UPROPERTY(BlueprintReadWrite, Category = "PLATEAU|BPLibraries")
    FVector VertexPos;
};

struct FEdgeData {
    FEdgeData() {
    }

    FEdgeData(const FVector InVertexPos0, const FVector InVertexPos1) : VertexPos0(InVertexPos0), VertexPos1(InVertexPos1) {
    }

    FVector VertexPos0;
    FVector VertexPos1;
};


UCLASS()
class PLATEAUEDITORBPLIBRARIES_API UPLATEAUAttrInfoDrawGizmo : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    /**
     * @brief 親と子のコンポーネントを線で囲む
     * @param WorldContextObject 現在のワールド
     * @param HitResult レイキャスト結果
     * @param LodIndex LOD
     */
    UFUNCTION(BlueprintCallable, Category = "PLATEAU|BPLibraries")
    static void DrawAttrInfo(const UWorld* WorldContextObject, const FHitResult& HitResult, const int32 LodIndex = 0);

    /**
     * @brief 親と子のコンポーネントを線で囲む
     * @param WorldContextObject 現在のワールド
     * @param HitResult レイキャスト結果
     * @param ChildSceneComponents 線で囲む親の持つ全ての子のシーンコンポーネント
     * @param LodIndex LOD
     */
    UFUNCTION(BlueprintCallable, Category = "PLATEAU|BPLibraries")
    static void DrawAttrInfoWithChildSceneComponents(const UWorld* WorldContextObject, const FHitResult& HitResult, const TArray<USceneComponent*> ChildSceneComponents, const int32 LodIndex = 0);

    /**
     * @brief 対象コンポーネントの親スタティックメッシュを取得
     * @param SceneComponent 親を持つか確認するシーンコンポーネント
     * @return 親のスタティックメッシュコンポーネント
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PLATEAU|BPLibraries")
    static UStaticMeshComponent* GetParentStaticMeshComponent(USceneComponent* SceneComponent);
};
