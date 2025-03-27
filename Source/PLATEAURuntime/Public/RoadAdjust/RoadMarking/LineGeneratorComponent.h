// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "RoadAdjust/PLATEAUReproducedRoad.h"
#include "LineGeneratorComponent.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class ESplineMeshType : uint8 {
    LengthBased,
    SegmentBased,
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PLATEAURUNTIME_API ULineGeneratorComponent : public USplineComponent
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
	UStaticMesh* StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
	UMaterialInterface* MaterialInterface;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
	float MeshGap;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    float MeshXScale;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    float MeshLength;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    FVector2D Offset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    ESplineMeshType SplineMeshType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    TEnumAsByte<ESplinePointType::Type> SplinePointType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    TEnumAsByte<ESplineCoordinateSpace::Type> CoordinateSpace;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    bool FillEnd;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PLATEAU|RoadAdjust")
    bool EnableShadow;

	UFUNCTION(BlueprintCallable, Category = "PLATEAU|RoadAdjust")
	void CreateSplineFromVectorArray(TArray<FVector> Points);

    UFUNCTION(BlueprintCallable, Category = "PLATEAU|RoadAdjust")
    void RefreshSplinePoints();

	UFUNCTION(BlueprintCallable, Category = "PLATEAU|RoadAdjust")
	void CreateSplineMesh(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "PLATEAU|RoadAdjust")
	void CreateSplineMeshFromAssets(AActor* Actor, UStaticMesh* Mesh, UMaterialInterface* Material, float Gap, float XScale, float Length = 0.f);

    /**
     * @brief コンポーネントの初期化を行います
     */
    UFUNCTION(BlueprintCallable, Category = "PLATEAU|RoadAdjust")
    void Init(const TArray<FVector>& InPoints, const FPLATEAURoadLineParam& Param, FVector2D InOffset);

    ULineGeneratorComponent();

#if WITH_EDITOR
	/**
	 * スプラインを編集したとき、線メッシュに反映させるために必要です
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:

	float GetMeshLength(bool includeGap);
    void CreateSplineMeshLengthBased(AActor* Actor);
    void CreateSplineMeshSegmentBased(AActor* Actor);
    USplineMeshComponent* CreateSplineMeshComponent(FName Name, AActor* Actor, FVector StartLocation, FVector StartTangent, FVector EndLocation, FVector EndTangent);

	USceneComponent* SplineMeshRoot;
};
