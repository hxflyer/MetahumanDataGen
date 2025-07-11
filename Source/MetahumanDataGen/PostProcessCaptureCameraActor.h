// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PostProcessCaptureCameraActor.generated.h"

/**
 *
 */
UCLASS()
class METAHUMANDATAGEN_API APostProcessCaptureCameraActor : public ACameraActor
{
	GENERATED_BODY()

public:
	APostProcessCaptureCameraActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	

	// Post-process material for depth extraction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	class UMaterialInterface* DepthExtractionMaterial;

	UPROPERTY()
	class USceneCaptureComponent2D* NormalMapSceneCapture;

	UPROPERTY()
	class UTextureRenderTarget2D* DepthRenderTarget;
	// Dynamic material instance
	UPROPERTY()
	class UMaterialInstanceDynamic* DepthExtractionMID;

	// Capture settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	FIntPoint CaptureSize = FIntPoint(1920, 1080);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	bool bAutoCapture = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	float CaptureInterval = 1.0f; // Capture every 1 second

private:
	// Internal variables
	float TimeSinceLastCapture = 0.0f;
	int32 CaptureCounter = 0;
	FString BaseDir;

	class UCameraComponent* CameraComponent;

	// Setup functions
	void SetupDepthExtractionMaterial();
	void SetupRenderTarget();
	void SetupSceneCapture();

	// Capture and save functions
	UFUNCTION(BlueprintCallable, Category = "Capture")
	void CaptureNormal();

	void SaveNormalMapToDisk(const FString& FilePath);
	void SaveToBmp(const TArray<FColor>& ImageData, const FString& FilePath, int32 Width, int32 Height);


	void SaveScreenCapture(const TArray<FColor>& PixelData, int32 Width, int32 Height);
	void CaptureScreenBuffer();
};