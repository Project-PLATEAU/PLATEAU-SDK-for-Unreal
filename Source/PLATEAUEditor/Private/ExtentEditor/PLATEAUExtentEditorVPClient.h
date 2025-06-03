// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "PLATEAUGeometry.h"

namespace plateau::dataset {
    class IDatasetAccessor;

    UENUM()
    enum ETranslucentSortPriority : int {
        SortPriority_BaseMap = -1,
        SortPriority_IconComponent = 1,
    };
}

/** Viewport Client for the preview viewport */
class FPLATEAUExtentEditorViewportClient : public FEditorViewportClient, public TSharedFromThis<FPLATEAUExtentEditorViewportClient> {
public:
    FPLATEAUExtentEditorViewportClient(const TWeakPtr<class FPLATEAUExtentEditor>& InExtentEditor,
                                       const TSharedRef<class SPLATEAUExtentEditorViewport>& InPLATEAUExtentEditorViewport,
                                       const TSharedRef<class FAdvancedPreviewScene>& InPreviewScene);
    virtual ~FPLATEAUExtentEditorViewportClient() override;

    /**
     * @brief ViewportのConstructから呼び出される初期化処理です。
     */
    void Initialize(const std::shared_ptr<plateau::dataset::IDatasetAccessor>& InFileCollection);

    /**
     * @brief 選択状態を初期化
     */
    void ResetSelectedArea();

    // FEditorViewportClient interface
    virtual void Tick(float DeltaSeconds) override;
    virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
    virtual void DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) override;
    virtual void TrackingStarted(const struct FInputEventState& InInputState, bool bIsDragging, bool bNudge) override;
    virtual void CapturedMouseMove(FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;
    virtual void TrackingStopped() override;
    virtual bool ShouldScaleCameraSpeedByDistance() const override;

    void SwitchFeatureInfoDisplay(const int Lod, const bool bCheck) const;
    bool SetViewLocationByGridCode(FString StrGridCode);

private:
    // このインスタンスを保持しているExtentEditorへのポインタ
    TWeakPtr<FPLATEAUExtentEditor> ExtentEditorPtr;
    FAdvancedPreviewScene* AdvancedPreviewScene;

    TUniquePtr<class FPLATEAUBasemap> Basemap;
    TSharedPtr<class FPLATEAUFeatureInfoDisplay> FeatureInfoDisplay;
    std::shared_ptr<plateau::dataset::IDatasetAccessor> DatasetAccessor;

    // 内部状態
    FPLATEAUExtent Extent;
    bool IsLeftMouseButtonPressed = false;
    bool IsLeftMouseAndShiftButtonPressed = false;
    bool IsLeftMouseButtonMoved = false;
    bool IsLeftMouseAndShiftButtonMoved = false;
    bool IsCameraMoving = false;
    FVector CachedWorldMousePos; 
    FVector TrackingStartedPosition;
    FVector TrackingStartedCameraPosition;
    TArray<class FPLATEAUGridCodeGizmo> GridCodeGizmos;

    // GridCodeGizmosをMeshCodeとStandardMapに分けてindexを保持
    TArray<int32> MeshCodeGizmoIndices;
    TArray<int32> StandardMapCodeGizmoIndices;
    
    bool GizmoContains(const FPLATEAUGridCodeGizmo& Gizmo) const;
    FVector GetWorldPosition(uint32 X, uint32 Y);
    bool TryGetWorldPositionOfCursor(FVector& Position);
    void InitCamera();
    FPLATEAUGridCodeGizmo GetNearestGridCodeGizmo();
    void CreateExclusiveGridCodeGizmoIndices();
};
