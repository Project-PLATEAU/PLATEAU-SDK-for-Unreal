// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "PLATEAUBasemap.h"
#include "SEditorViewport.h"

class FPLATEAUEditorStyle;

class SPLATEAUExtentEditorViewport : public SEditorViewport {
public:
    SLATE_BEGIN_ARGS(SPLATEAUExtentEditorViewport) {}
    SLATE_ARGUMENT(TWeakPtr<class FPLATEAUExtentEditor>, ExtentEditor)
        SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    SPLATEAUExtentEditorViewport();
    virtual ~SPLATEAUExtentEditorViewport() override;

    void SetOwnerTab(TSharedRef<class SDockTab> Tab);
    TSharedPtr<class SDockTab> GetOwnerTab() const;

protected:
    /** SEditorViewport interface */
    virtual bool IsVisible() const override;
    virtual TSharedRef<class FEditorViewportClient> MakeEditorViewportClient() override;
    virtual void PopulateViewportOverlays(TSharedRef<class SOverlay> Overlay) override;
    virtual void BindCommands() override;

private:
    int64 GetPackageMask(const bool bImportFromServer) const;
    
    // このインスタンスを保持しているExtentEditorへのポインタ
    TWeakPtr<class FPLATEAUExtentEditor> ExtentEditorPtr;
    // ViewportClientに渡すScene
    TSharedPtr<class FAdvancedPreviewScene> PreviewScene;
    TSharedPtr<class FPLATEAUExtentEditorViewportClient> ViewportClient;
    // このビューポートを含むDockTab
    TWeakPtr<class SDockTab> OwnerTab;
    // エディタースタイル
    TSharedPtr<FPLATEAUEditorStyle> Style;
    //メッシュコード入力パネル
    TWeakPtr<SWindow> GridCodeInputWindow;
    TWeakPtr<SEditableTextBox> GridCodeTextBox;
    TWeakPtr<STextBlock> GridCodeErrorText;
};
