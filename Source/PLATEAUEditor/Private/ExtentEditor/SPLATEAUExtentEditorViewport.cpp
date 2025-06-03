// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "ExtentEditor/SPLATEAUExtentEditorViewport.h"
#include "ExtentEditor/PLATEAUExtentEditorVPClient.h"
#include "ExtentEditor/PLATEAUExtentEditor.h"

#include "AdvancedPreviewScene.h"
#include "SSubobjectEditor.h"
#include "Slate/SceneViewport.h"
#include "BlueprintEditorSettings.h"
#include "SlateOptMacros.h"

#include "plateau/dataset/dataset_source.h"
#include "plateau/dataset/i_dataset_accessor.h"
#include "plateau/basemap/vector_tile_downloader.h"
#include "plateau/geometry/geo_reference.h"

#include <filesystem>
#include <fstream>
#include "Misc/FileHelper.h"
#include "IImageWrapperModule.h"
#include "PLATEAUEditor.h"
#include "PLATEAUEditorStyle.h"
#include "PLATEAUWindow.h"
#include "Async/Async.h"
#include "Widgets/PLATEAUSDKEditorUtilityWidget.h"


#define LOCTEXT_NAMESPACE "SPLATEAUExtentEditorViewport"


SPLATEAUExtentEditorViewport::SPLATEAUExtentEditorViewport() : PreviewScene(MakeShareable(new FAdvancedPreviewScene(FPreviewScene::ConstructionValues()))),
                                                               Style(MakeShareable(new FPLATEAUEditorStyle())) {
}

SPLATEAUExtentEditorViewport::~SPLATEAUExtentEditorViewport() {
    if (ViewportClient.IsValid()) {
        ViewportClient->Viewport = nullptr;
    }

    if (GridCodeInputWindow.IsValid()) {
        GridCodeInputWindow.Pin()->RequestDestroyWindow();
        GridCodeInputWindow.Reset();
    }
}

void SPLATEAUExtentEditorViewport::Construct(const FArguments& InArgs) {
    ExtentEditorPtr = InArgs._ExtentEditor;

    SEditorViewport::Construct(SEditorViewport::FArguments());

    if (ViewportClient.IsValid()) {
        UWorld* World = ViewportClient->GetPreviewScene()->GetWorld();
        if (World != nullptr) {
            World->ChangeFeatureLevel(GWorld->GetFeatureLevel());
        }
        const auto& SourcePath = ExtentEditorPtr.Pin()->GetSourcePath();
        const auto ClientRef = ExtentEditorPtr.Pin()->GetClientPtr();
        const auto ID = ExtentEditorPtr.Pin()->GetServerDatasetID();
        const auto bImportFromServer = ExtentEditorPtr.Pin()->IsImportFromServer();
        std::shared_ptr<plateau::dataset::IDatasetAccessor> DatasetAccessor;
        if (bImportFromServer) {
            try {
                const auto DatasetSource = plateau::dataset::DatasetSource::createServer(ID, *ClientRef);
                DatasetAccessor = DatasetSource.getAccessor();
            } catch (...) {
                UE_LOG(LogTemp, Error, TEXT("Failed to open source ID: %s"), *FString(ID.c_str()));
            }
        } else {
            try {
                const auto DatasetSource = plateau::dataset::DatasetSource::createLocal(TCHAR_TO_UTF8(*SourcePath));
                DatasetAccessor = DatasetSource.getAccessor();
            } catch (...) {
                UE_LOG(LogTemp, Error, TEXT("Failed to open udx source path: %s"), *SourcePath);
            }
        }

        if (DatasetAccessor == nullptr)
            return;

        if (DatasetAccessor->getGridCodes().size() == 0)
            return;

        auto GeoReference = ExtentEditorPtr.Pin()->GetGeoReference();
        const auto RawCenterPoint = DatasetAccessor->calculateCenterPoint(GeoReference.GetData());
        GeoReference.ReferencePoint.X = RawCenterPoint.x;
        GeoReference.ReferencePoint.Y = RawCenterPoint.y;
        GeoReference.ReferencePoint.Z = RawCenterPoint.z;
        ExtentEditorPtr.Pin()->SetGeoReference(GeoReference);

        ViewportClient->Initialize(DatasetAccessor);
    }
}

bool SPLATEAUExtentEditorViewport::IsVisible() const {
    // We consider the viewport to be visible if the reference is valid
    return ViewportWidget.IsValid() && SEditorViewport::IsVisible();
}

TSharedRef<FEditorViewportClient> SPLATEAUExtentEditorViewport::MakeEditorViewportClient() {
    // Construct a new viewport client instance.
    ViewportClient = MakeShareable(new FPLATEAUExtentEditorViewportClient(ExtentEditorPtr, SharedThis(this), PreviewScene.ToSharedRef()));
    ViewportClient->SetRealtime(true);
    ViewportClient->bSetListenerPosition = false;
    ViewportClient->VisibilityDelegate.BindSP(this, &SPLATEAUExtentEditorViewport::IsVisible);

    return ViewportClient.ToSharedRef();
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SPLATEAUExtentEditorViewport::PopulateViewportOverlays(TSharedRef<class SOverlay> Overlay) {
    SEditorViewport::PopulateViewportOverlays(Overlay);

    Overlay->AddSlot().VAlign(VAlign_Top).HAlign(HAlign_Left).Padding(5.f)
    [
        SNew(SBorder).BorderImage(Style.ToSharedRef()->GetBrush("PLATEAUEditor.FloatingBorder")).Padding(5.f).
        OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent& PointerEvent) {
            // マウスイベントをここで吸収
            return FReply::Handled();
        })
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).Padding(FMargin(5.f, 5.f, 5.f, 8.f))
            [
                SNew(STextBlock).Text(LOCTEXT("OverlayAreaSelectionText", "範囲選択")).
                TextStyle(Style.ToSharedRef(), "PLATEAUEditor.Bold.13").ColorAndOpacity(FLinearColor::White)                
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(5.f))
            [
                SNew(SButton).VAlign(VAlign_Center).ForegroundColor(FColor::White).ButtonStyle(Style.ToSharedRef(), "PLATEAUEditor.FlatButton.Gray").
                OnClicked_Lambda([this] {
                    ViewportClient->ResetSelectedArea();
                    return FReply::Handled();
                }).
                Content()
                [
                    SNew(STextBlock).Justification(ETextJustify::Center).Margin(FMargin(15.f, 2.f, 15.f, 2.f)).Text(LOCTEXT("Area Reset Button", "全選択解除"))
                ]
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(5.f))
            [
                SNew(SButton).VAlign(VAlign_Center).ForegroundColor(FColor::White).ButtonStyle(Style.ToSharedRef(), "PLATEAUEditor.FlatButton.Gray").
                OnClicked_Lambda([this] {

                if (GridCodeInputWindow.IsValid()) 
                    return FReply::Handled();

                //入力Window表示
                TSharedRef<SWindow> InputWindow = SNew(SWindow)
                    .Title(FText::FromString(TEXT("メッシュコード入力")))
                    .ClientSize(FVector2D(350, 140))
                    .SupportsMaximize(false)
                    .SupportsMinimize(false)
                    .AutoCenter(EAutoCenter::PrimaryWorkArea)
                    .FocusWhenFirstShown(true)
                    .IsTopmostWindow(true)
                    .HasCloseButton(false);
                GridCodeInputWindow = InputWindow;
                
                InputWindow->SetContent(
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight().Padding(FMargin(5.f, 8.f, 5.f, 8.f))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("メッシュコードを入力してください\n（6桁または８桁の数字）")))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight().Padding(FMargin(5.f, 8.f, 5.f, 8.f))
                    [
                        SAssignNew(GridCodeTextBox, SEditableTextBox)
                        .HintText(FText::FromString(TEXT("メッシュコード")))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight().Padding(FMargin(5.f, 2.f, 5.f, 2.f))
                    [
                        SAssignNew(GridCodeErrorText, STextBlock)
                        .ColorAndOpacity(FLinearColor::Red)
                        .Text(FText())
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight().Padding(FMargin(5.f, 8.f, 5.f, 8.f))
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth().Padding(FMargin(2.f, 0.f, 2.f, 0.f))
                        [
                            SNew(SButton)
                            .Text(FText::FromString(TEXT("キャンセル"))).
                            OnClicked_Lambda([this] {
                                //Windowを閉じる
                                if (GridCodeInputWindow.IsValid()) {
                                    GridCodeInputWindow.Pin()->RequestDestroyWindow();
                                    GridCodeInputWindow.Reset();
                                }
                                return FReply::Handled();
                            })
                        ]
                        + SHorizontalBox::Slot()
                            .AutoWidth().Padding(FMargin(2.f, 0.f, 2.f, 0.f))
                            [
                                SNew(SButton)
                                .Text(FText::FromString(TEXT("OK"))).
                                OnClicked_Lambda([this] {
                                    //メッシュコードの位置を表示
                                    FText Value = GridCodeTextBox.Pin()->GetText();
                                    FString meshcode = Value.ToString();
                                    if (!meshcode.IsNumeric()) {
                                        if(GridCodeErrorText.IsValid())
                                            GridCodeErrorText.Pin()->SetText(FText::FromString(TEXT("数字を入力してください")));
                                    }
                                    else if (meshcode.Len() != 6 && meshcode.Len() != 8) {
                                        if (GridCodeErrorText.IsValid())
                                            GridCodeErrorText.Pin()->SetText(FText::FromString(TEXT("6桁または８桁の数字を入力してください")));
                                    }
                                    else if (!ViewportClient->SetViewLocationByGridCode(meshcode)) {
                                        if (GridCodeErrorText.IsValid())
                                            GridCodeErrorText.Pin()->SetText(FText::FromString(TEXT("メッシュコードが範囲外です")));
                                    }
                                    else {
                                        if (GridCodeInputWindow.IsValid()) {
                                            GridCodeInputWindow.Pin()->RequestDestroyWindow();
                                            GridCodeInputWindow.Reset();
                                        }
                                    }     
                                    return FReply::Handled();
                                })
                            ]
                        ]
                    );
                    FSlateApplication::Get().AddWindow(InputWindow, true);
                    return FReply::Handled();
                }).
                Content()
                [
                    SNew(STextBlock).Justification(ETextJustify::Center).Margin(FMargin(15.f, 2.f, 15.f, 2.f)).Text(LOCTEXT("Search By Mesh Code Button", "メッシュコード検索"))
                ]
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(5.f))
            [
                SNew(SButton).VAlign(VAlign_Center).ForegroundColor(FColor::White).ButtonStyle(Style.ToSharedRef(), "PLATEAUEditor.FlatButton.Gray").
                OnClicked_Lambda([this] {
                    const auto ReferencePoint = ExtentEditorPtr.Pin()->GetSelectedCenterPoint(ExtentEditorPtr.Pin()->GetGeoReference().ZoneID, ExtentEditorPtr.Pin()->IsImportFromServer());
                    const auto PackageMask = GetPackageMask(ExtentEditorPtr.Pin()->IsImportFromServer());
                    const auto& EditorUtilityWidget = IPLATEAUEditorModule::Get().GetWindow()->GetEditorUtilityWidget();
                    if (EditorUtilityWidget != nullptr) {
                        const auto& PLATEAUSDKEditorUtilityWidget = dynamic_cast<UPLATEAUSDKEditorUtilityWidget*>(EditorUtilityWidget);
                        if (PLATEAUSDKEditorUtilityWidget != nullptr) {
                            PLATEAUSDKEditorUtilityWidget->AreaSelectSuccessInvoke(ReferencePoint, PackageMask);
                        }
                    } else {
                        UE_LOG(LogTemp, Warning, TEXT("PLATEAU SDK Widget Error"));
                        const FText Title = LOCTEXT("Warning", "警告");
                        const FText DialogText = LOCTEXT("WidgetError", "PLATEAU SDKに問題が発生しました。PLATEAU SDKを再起動して下さい。");
                        FMessageDialog::Open(EAppMsgType::Ok, DialogText, Title);
                    }                                               
                    if (GetOwnerTab())
                        GetOwnerTab()->RequestCloseTab();
                    return FReply::Handled();
                }).
                IsEnabled_Lambda([this] {
                    return ExtentEditorPtr.Pin().Get()->IsSelectedArea();
                }).
                Content()
                [
                    SNew(STextBlock).Justification(ETextJustify::Center).Margin(FMargin(15.f, 2.f, 15.f, 2.f)).Text(LOCTEXT("OK Button", "決定"))
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(5.f))
            [
                SNew(SButton).VAlign(VAlign_Center).ForegroundColor(FColor::White).ButtonStyle(Style.ToSharedRef(), "PLATEAUEditor.FlatButton.Gray").
                OnClicked_Lambda([this]() {
                    if (GetOwnerTab())
                        GetOwnerTab()->RequestCloseTab();
                    return FReply::Handled();
                }).
                Content()
                [
                    SNew(STextBlock).Justification(ETextJustify::Center).Margin(FMargin(15.f, 2.f, 15.f, 2.f)).Text(LOCTEXT("Cancel Button", "キャンセル"))
                ]
            ]
        ]
    ];

    Overlay->AddSlot().VAlign(VAlign_Bottom).HAlign(HAlign_Left).Padding(5.f)
    [
        SNew(SBorder).BorderImage(Style.ToSharedRef()->GetBrush("PLATEAUEditor.FloatingBorder")).Padding(10.f).
        OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent& PointerEvent) {
            // マウスイベントをここで吸収
            return FReply::Handled();
        })
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).AutoHeight().Padding(FMargin(5.f))
            [
                SNew(STextBlock).Text(LOCTEXT("OverlayLodText", "LOD")).
                TextStyle(Style.ToSharedRef(), "PLATEAUEditor.Bold.14").ColorAndOpacity(FLinearColor::White)
            ]
            + SVerticalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).AutoHeight().Padding(FMargin(5.f))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Right).AutoWidth().Padding(FMargin(5.f))
                [
                    SNew(SBox).WidthOverride(30.f).HeightOverride(30.f)
                    [
                        SNew(SImage).Image(Style.ToSharedRef()->GetBrush("PLATEAUEditor.Lod01"))
                    ]
                ]
                + SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Left).Padding(FMargin(5.f))
                [
                     SNew(SBox).WidthOverride(20.f).HeightOverride(20.f)
                     [
                         SNew(SCheckBox).Padding(4.f).Style(Style.ToSharedRef(), "PlateauCheckboxLookToggleButtonCheckbox").IsChecked(ECheckBoxState::Checked).
                         OnCheckStateChanged_Lambda([this](const ECheckBoxState State) {
                             ViewportClient->SwitchFeatureInfoDisplay(1, State == ECheckBoxState::Checked);
                         })
                     ]
                 ]
            ]
            + SVerticalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).Padding(FMargin(5.f))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Right).AutoWidth().Padding(FMargin(5.f))
                [
                    SNew(SBox).WidthOverride(30.f).HeightOverride(30.f)
                    [
                        SNew(SImage).Image(Style.ToSharedRef()->GetBrush("PLATEAUEditor.Lod02"))
                    ]
                ]
                + SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Left).Padding(FMargin(5.f))
                [
                     SNew(SBox).WidthOverride(20.f).HeightOverride(20.f)
                     [
                         SNew(SCheckBox).Padding(4.f).Style(Style.ToSharedRef(), "PlateauCheckboxLookToggleButtonCheckbox").IsChecked(ECheckBoxState::Checked).
                         OnCheckStateChanged_Lambda([this](const ECheckBoxState State) {
                             ViewportClient->SwitchFeatureInfoDisplay(2, State == ECheckBoxState::Checked);
                         })
                     ]
                 ]
            ]
            + SVerticalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).Padding(FMargin(5.f))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Right).AutoWidth().Padding(FMargin(5.f))
                [
                    SNew(SBox).WidthOverride(30.f).HeightOverride(30.f)
                    [
                        SNew(SImage).Image(Style.ToSharedRef()->GetBrush("PLATEAUEditor.Lod03"))
                    ]
                ]
                + SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Left).Padding(FMargin(5.f))
                [
                     SNew(SBox).WidthOverride(20.f).HeightOverride(20.f)
                     [
                         SNew(SCheckBox).Padding(4.f).Style(Style.ToSharedRef(), "PlateauCheckboxLookToggleButtonCheckbox").IsChecked(ECheckBoxState::Checked).
                         OnCheckStateChanged_Lambda([this](const ECheckBoxState State) {
                             ViewportClient->SwitchFeatureInfoDisplay(3, State == ECheckBoxState::Checked);
                         })
                     ]
                 ]
            ]
            + SVerticalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).Padding(FMargin(5.f))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Right).AutoWidth().Padding(FMargin(5.f))
                [
                    SNew(SBox).WidthOverride(30.f).HeightOverride(30.f)
                    [
                        SNew(SImage).Image(Style.ToSharedRef()->GetBrush("PLATEAUEditor.Lod04"))
                    ]
                ]
                + SHorizontalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Left).Padding(FMargin(5.f))
                [
                     SNew(SBox).WidthOverride(20.f).HeightOverride(20.f)
                     [
                         SNew(SCheckBox).Padding(4.f).Style(Style.ToSharedRef(), "PlateauCheckboxLookToggleButtonCheckbox").IsChecked(ECheckBoxState::Checked).
                         OnCheckStateChanged_Lambda([this](const ECheckBoxState State) {
                             ViewportClient->SwitchFeatureInfoDisplay(4, State == ECheckBoxState::Checked);
                         })
                     ]
                 ]
            ]
        ]
    ];

    Overlay->AddSlot().VAlign(VAlign_Bottom).HAlign(HAlign_Right).Padding(5.f)
    [
        SNew(SBorder).BorderImage(Style.ToSharedRef()->GetBrush("PLATEAUEditor.FloatingBorder")).Padding(10.f).
        OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent& PointerEvent) {
            // マウスイベントをここで吸収
            return FReply::Handled();
        })
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(5.f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).AutoHeight().Padding(FMargin(5.f))
                [
                    SNew(STextBlock).Text(LOCTEXT("OverlayLodText", "操作方法")).
                    TextStyle(Style.ToSharedRef(), "PLATEAUEditor.Bold.14").ColorAndOpacity(FLinearColor::White)
                ]
                + SVerticalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).Padding(FMargin(5.f))
                [
                    SNew(STextBlock).Text(LOCTEXT("OverlayLodText", "クリック：選択切替")).
                    TextStyle(Style.ToSharedRef(), "PLATEAUEditor.Bold.14").ColorAndOpacity(FLinearColor::White)
                ]
                + SVerticalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).Padding(FMargin(5.f))
                [
                    SNew(STextBlock).Text(LOCTEXT("OverlayLodText", "ドラッグ：矩形を選択範囲に追加")).
                    TextStyle(Style.ToSharedRef(), "PLATEAUEditor.Bold.14").ColorAndOpacity(FLinearColor::White)
                ]
                + SVerticalBox::Slot().VAlign(VAlign_Center).HAlign(HAlign_Center).Padding(FMargin(5.f))
                [
                    SNew(STextBlock).Text(LOCTEXT("OverlayLodText", "Shift＋ドラッグ：矩形を選択範囲から除外")).
                    TextStyle(Style.ToSharedRef(), "PLATEAUEditor.Bold.14").ColorAndOpacity(FLinearColor::White)
                ]
            ]
        ]
    ];    
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SPLATEAUExtentEditorViewport::BindCommands() {
}

void SPLATEAUExtentEditorViewport::SetOwnerTab(TSharedRef<SDockTab> Tab) {
    OwnerTab = Tab;
}

TSharedPtr<SDockTab> SPLATEAUExtentEditorViewport::GetOwnerTab() const {
    return OwnerTab.Pin();
}

int64 SPLATEAUExtentEditorViewport::GetPackageMask(const bool bImportFromServer) const {
    const auto& StrSelectedMeshCodes = ExtentEditorPtr.Pin()->GetSelectedCodes(bImportFromServer);
    std::vector<std::shared_ptr<plateau::dataset::GridCode>> NativeSelectedGridCodes;
    for (const auto& Code : StrSelectedMeshCodes) {
        NativeSelectedGridCodes.push_back(plateau::dataset::GridCode::create(TCHAR_TO_UTF8(*Code)));
    }

    if (ExtentEditorPtr.Pin()->IsImportFromServer()) {
        const auto ClientRef = ExtentEditorPtr.Pin()->GetClientPtr();
        const auto InDatasetSource = plateau::dataset::DatasetSource::createServer(ExtentEditorPtr.Pin()->GetServerDatasetID(), *ClientRef);
        const auto FilteredDatasetAccessor = InDatasetSource.getAccessor()->filterByGridCodes(NativeSelectedGridCodes);
        const auto PackageMask = FilteredDatasetAccessor->getPackages();
        ExtentEditorPtr.Pin()->SetServerPackageMask(PackageMask);
        return static_cast<int64>(PackageMask);
    }

    const auto InDatasetSource = plateau::dataset::DatasetSource::createLocal(TCHAR_TO_UTF8(*ExtentEditorPtr.Pin()->GetSourcePath()));
    const auto FilteredDatasetAccessor = InDatasetSource.getAccessor()->filterByGridCodes(NativeSelectedGridCodes);
    const auto PackageMask = FilteredDatasetAccessor->getPackages();
    ExtentEditorPtr.Pin()->SetLocalPackageMask(PackageMask);
    return static_cast<int64>(PackageMask);
}

#undef LOCTEXT_NAMESPACE
