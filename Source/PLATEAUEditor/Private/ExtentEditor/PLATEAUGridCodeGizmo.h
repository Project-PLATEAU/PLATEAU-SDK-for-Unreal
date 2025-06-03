// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include <plateau/dataset/grid_code.h>

namespace plateau {
    namespace geometry {
        class GeoReference;
        constexpr auto ShowFeatureDetailIconCameraDistance = 4000;
        constexpr auto ShowFeatureIconCameraDistance = 9000;
        constexpr auto ShowGridCodeIdCameraDistance = 9000;
    }
}

UENUM(BlueprintType)
enum class EGridCodeGizmoType : uint8 {
    MeshCode = 0,
    StandardMapCode = 1,
};

/**
 * @brief 各グリッドコードのギズモを表します。
 */
class PLATEAUEDITOR_API FPLATEAUGridCodeGizmo {
public:
    FPLATEAUGridCodeGizmo();

    void ResetSelectedArea();
    void DrawExtent(const FSceneView* View, FPrimitiveDrawInterface* PDI) const;
    void DrawRegionGridCodeID(const FViewport& InViewport, const FSceneView& View, FCanvas& Canvas, const FString& GridCodeID, double CameraDistance, int IconCount) const;

    /**
     * @brief 内部状態から範囲の最小値を取得します。
     */
    FVector2D GetMin() const;

    /**
     * @brief 内部状態から範囲の最大値を取得します。
     */
    FVector2D GetMax() const;

    /**
     * @brief 内部状態から範囲のサイズを取得します。 
     */
    FVector2D GetSize() const;

    /**
     * @brief グリッドコード取得
     */
    std::shared_ptr<plateau::dataset::GridCode> GetGridCode() const;
    
    /**
     * @brief グリッドコードを文字列で取得
     */
    FString GetRegionGridCodeID() const;

    /**
     * @brief 選択状態取得 
     */
    TArray<bool> GetbSelectedArray() const;

    /**
     * @brief 範囲が選択されているか否か 
     */
    bool bSelectedArea() const;

    /**
    * @brief 選択状態設定 
    */
    void SetbSelectedArray(const TArray<bool>& InbSelectedArray);

    /**
     * @brief 選択範囲をBoxとして取得
     * @param OutBoxes 選択範囲となるBoxリスト
     */
    void GetSelectedBoxes(TArray<FBox>& OutBoxes) const;

    /**
     * @brief インスタンスを初期化します。
     */
    void Init(const std::shared_ptr<plateau::dataset::GridCode>& InGridCode, const plateau::geometry::GeoReference& InGeoReference);

    /**
     * @brief マウス座標がエリア内であれば選択状態をトグル
     * @param X マウス座標X
     * @param Y マウス座標Y
     */
    void ToggleSelectArea(const double X, const double Y);

    /**
     * @brief ドラッグ矩形がギズモ範囲と交差していた時に選択されたとみなす
     * @param InMin ドラッグ矩形の最小座標
     * @param InMax ドラッグ矩形の最大座標
     * @param bSelect エリアを指定の選択状態に設定
     */
    void SetSelectArea(const FVector2d InMin, const FVector2d InMax, const bool bSelect);

    /**
     * @brief マウス座標がエリア内であれば指定の選択状態に設定
     * @param X マウス座標X
     * @param Y マウス座標Y
     * @param bSelect エリアを指定の選択状態に設定
     */
    void SetSelectArea(const double X, const double Y, const bool bSelect);

    /**
     * @brief Boxと重なった範囲を選択状態に設定・重なりがなければ選択解除（国土基本図郭選択用）
     * @param InBoxes 重なり判定用のBox
     */
    void SetOverlapSelection(const TArray<FBox>& InBoxes);

    /**
     * @brief 選択されているグリッドコードの文字列の配列を取得
     */
    TArray<FString> GetSelectedGridCodeIDs();

    /**
     * @brief 選択されているグリッドコードのタイプを取得(MeshCode/StandardMapCode)
     */
    EGridCodeGizmoType GetGridCodeType() const;
    
    /**
     * @brief エリア内の描画有効化状態を設定
     * @param bValue 有効化するか？
     */
    static void SetShowLevel5Mesh(const bool bValue);

private:
    inline static bool bShowLevel5Mesh = false;

    std::shared_ptr<plateau::dataset::GridCode> GridCode;
    FString GridCodeString;
    double Width;
    double Height;
    double MinX;
    double MinY;
    double MaxX;
    double MaxY;
    float LineThickness;
    // メッシュコード・国土基本図郭
    EGridCodeGizmoType GridCodeType;

    TArray<bool> bSelectedArray;
    TObjectPtr<UMaterialInstanceDynamic> AreaSelectedMaterial;
    TObjectPtr<UMaterialInstanceDynamic> AreaUnSelectedMaterial;
    bool IsSelectable() const;
    bool IsStandardMapGrid() const;

    /**
     * @brief メッシュコード・国土基本図郭判定
     */
    EGridCodeGizmoType GetGridCodeTypeByGridCodeString(const FString& GridCodeStr) const;

    /**
     * @brief 各セルのMatrixを取得
     * @param OutBoxes 選択範囲となるMatrixリスト
     * @param bSelectedOnly 選択状態のセルのみ取得するか
     */
    void GetCellMatrices(TArray<FMatrix>& OutMatrices, const bool bSelectedOnly) const;

    /**
     * @brief 各セルのBoxを取得
     * @param OutBoxes 選択範囲となるBoxリスト
     * @param bSelectedOnly 選択状態のセルのみ取得するか
     */
    void GetCellBoxes(TArray<FBox>& OutBoxes, const bool bSelectedOnly) const;
};
