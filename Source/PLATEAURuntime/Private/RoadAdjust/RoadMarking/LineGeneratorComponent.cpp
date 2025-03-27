// Copyright © 2023 Ministry of Land, Infrastructure and Transport

#include "RoadAdjust/RoadMarking/LineGeneratorComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "RoadAdjust/PLATEAUReproducedRoad.h"
#include "Engine/StaticMesh.h"

ULineGeneratorComponent::ULineGeneratorComponent() :
    SplineMeshType(ESplineMeshType::LengthBased), 
    SplinePointType(ESplinePointType::Linear), 
    CoordinateSpace(ESplineCoordinateSpace::Local),  
    FillEnd(false),
    EnableShadow(false) {
    this->SetDrawDebug(false);
    this->bInputSplinePointsToConstructionScript = false;
    this->SetMobility(EComponentMobility::Static);
}

float ULineGeneratorComponent::GetMeshLength(bool includeGap) {
	if (StaticMesh == nullptr)
		return 0.f;

    float Length = MeshLength != 0.f ? MeshLength : StaticMesh->GetBounds().SphereRadius;
    //UE_LOG(LogTemp, Log, TEXT("Mesh BoxExtent : ( %f , %f , %f )"), StaticMesh->GetBounds().BoxExtent.X, StaticMesh->GetBounds().BoxExtent.Y, StaticMesh->GetBounds().BoxExtent.Z);

	if (includeGap)
	{
		return Length + MeshGap;
	}

	return Length;
}

void ULineGeneratorComponent::CreateSplineFromVectorArray(TArray<FVector> Points) {
	this->ClearSplinePoints();
    
	for (const auto& Point : Points) {
		this->AddSplinePoint(Point, CoordinateSpace, true);
	}

    RefreshSplinePoints();
}

void ULineGeneratorComponent::RefreshSplinePoints() {
    for (int32 i = 0; i < this->GetNumberOfSplinePoints(); i++) {
        this->SetSplinePointType(i, SplinePointType, true);
    }
}

void ULineGeneratorComponent::CreateSplineMesh(AActor* Actor) {

	if (StaticMesh == nullptr)
		return;

	if (SplineMeshRoot == nullptr) {
		SplineMeshRoot = NewObject<USceneComponent>(this, FName(TEXT("SplineMeshRoot")));
        SplineMeshRoot->SetMobility(EComponentMobility::Static);
		SplineMeshRoot->RegisterComponent();
        SplineMeshRoot->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);
        Actor->AddInstanceComponent(SplineMeshRoot);
	}

	//Delete old components
	TArray<USceneComponent*> SplineMeshChildren;
	SplineMeshRoot->GetChildrenComponents(true, SplineMeshChildren);
	for (auto Comp : SplineMeshChildren)
	{
		Comp->DestroyComponent();
	}

    if (SplineMeshType == ESplineMeshType::LengthBased)
        CreateSplineMeshLengthBased(Actor);
    else if (SplineMeshType == ESplineMeshType::SegmentBased)
        CreateSplineMeshSegmentBased(Actor);
}

void ULineGeneratorComponent::CreateSplineMeshLengthBased(AActor* Actor) {

    //Add Spline Mesh
    for(int index = 0; index < GetNumberOfSplinePoints() - 1; index++)
    {
        FVector StartPos, EndPos, StartTangent, EndTangent;
        GetLocalLocationAndTangentAtSplinePoint(index, StartPos, StartTangent);
        GetLocalLocationAndTangentAtSplinePoint(index + 1, EndPos, EndTangent);

        // タンジェントが変になることがあるので修正
        StartTangent = (EndPos - StartPos).GetSafeNormal();
        EndTangent = (EndPos - StartPos).GetSafeNormal();

        CreateSplineMeshComponent(FName(TEXT("SplineMesh_") + FString::FromInt(index)), Actor, StartPos, StartTangent, EndPos, EndTangent);
    }
}

void ULineGeneratorComponent::CreateSplineMeshSegmentBased(AActor* Actor)
{
    float SplineLength = GetSplineLength();
    float Length = GetMeshLength(true);
    int numLoop = SplineLength / Length;
    //Add Spline Mesh
    for (int64 index = 0; index < numLoop; index++)
    {
        float startDistance = Length * index;
        const auto& startLocation = this->GetLocationAtDistanceAlongSpline(startDistance, CoordinateSpace);
        const auto& startTangent = UKismetMathLibrary::Normal(
            this->GetTangentAtDistanceAlongSpline(startDistance, CoordinateSpace));

        float endDistance = (Length * (index + 1)) - MeshGap;

        if (index == numLoop - 1 && FillEnd)
        {
            endDistance = this->GetSplineLength();

            if (SplinePointType == ESplinePointType::Linear)
                endDistance -= 0.01f;
        }

        const auto& endLocation = this->GetLocationAtDistanceAlongSpline(endDistance, CoordinateSpace);
        const auto& endTangent = UKismetMathLibrary::Normal(
            this->GetTangentAtDistanceAlongSpline(endDistance, CoordinateSpace));
        CreateSplineMeshComponent(FName(TEXT("SplineMesh_") + FString::FromInt(index)), Actor, startLocation,
                                  startTangent, endLocation, endTangent);
    }
}

USplineMeshComponent* ULineGeneratorComponent::CreateSplineMeshComponent(FName Name, AActor* Actor, FVector StartLocation, FVector StartTangent, FVector EndLocation, FVector EndTangent) {
    auto SplineMeshComponent = NewObject<USplineMeshComponent>(this, Name);
    SplineMeshComponent->SetMobility(EComponentMobility::Static);
    SplineMeshComponent->RegisterComponent();
    SplineMeshComponent->AttachToComponent(SplineMeshRoot, FAttachmentTransformRules::KeepWorldTransform);
    Actor->AddInstanceComponent(SplineMeshComponent);

    SplineMeshComponent->SetStaticMesh(StaticMesh);
    if (MaterialInterface != nullptr) {
        SplineMeshComponent->SetMaterial(0, MaterialInterface);
    }

    SplineMeshComponent->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent, true);
    SplineMeshComponent->SetStartScale(FVector2D(MeshXScale, 1.0f));
    SplineMeshComponent->SetEndScale(FVector2D(MeshXScale, 1.0f));
    SplineMeshComponent->SetStartOffset(Offset);
    SplineMeshComponent->SetEndOffset(Offset);
    SplineMeshComponent->CastShadow = EnableShadow;
    return SplineMeshComponent;
}

void ULineGeneratorComponent::CreateSplineMeshFromAssets(AActor* Actor, UStaticMesh* Mesh, UMaterialInterface* Material, float Gap, float XScale, float Length) {
	StaticMesh = Mesh;
	MaterialInterface = Material;
	MeshGap = Gap;
    MeshXScale = XScale;
    MeshLength = Length;
	CreateSplineMesh(Actor);
}

void ULineGeneratorComponent::Init(const TArray<FVector>& InPoints, const FPLATEAURoadLineParam& Param, FVector2D InOffset) {
    // スプラインポイントの種類を設定
    SplinePointType = Param.SplinePointType;
    
    // スプラインポイントを作成
    CreateSplineFromVectorArray(InPoints);
    
    // スプラインメッシュの種類を設定
    if( Param.LineGap > 0.001f )
    {
        SplineMeshType = ESplineMeshType::SegmentBased;
    }else
    {
        SplineMeshType = ESplineMeshType::LengthBased; 
    }
    
    // 終端を埋めるかどうかを設定
    FillEnd = Param.FillEnd;
    
    // オフセット値を設定
    Offset = InOffset;
}

#if WITH_EDITOR
void ULineGeneratorComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    CreateSplineMesh(GetOwner());
}
#endif
