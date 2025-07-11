// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Camera/CameraComponent.h"
#include "MHActor.h"
//#include "EnvActor.h"

#include "MyCameraActor.generated.h"
/**
 * 
 */
UCLASS()
class METAHUMANDATAGEN_API AMyCameraActor : public ACameraActor
{
	GENERATED_BODY()
	
public:
    AMyCameraActor();
    virtual ~AMyCameraActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    

    // How often to save (e.g., every 30 frames)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
    int32 SaveEveryNFrame = 30;

    // Render Target size
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
    FIntPoint CaptureSize = FIntPoint(1024, 1024);

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Capture")
    AMHActor* mh;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Capture")
    AActor* HDRI;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Capture")
    UMaterialInterface* NormalMat;
    //UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Capture")
    //AEnvActor* env;
    

    void Snapshot(FString id);
    int32 CaptureStep = -1;
    FString SnapshotID = "0";

private:
    
    
    
    FString BaseDir;

    
    // Components for capture
    UPROPERTY()
    UCameraComponent* CamComp = nullptr;

    UPROPERTY()
    USceneCaptureComponent2D* SceneCaptureColor = nullptr;

    UPROPERTY()
    USceneCaptureComponent2D* SceneCaptureDepth = nullptr;

    UPROPERTY()
    USceneCaptureComponent2D* SceneCaptureNormal = nullptr;

    UPROPERTY()
    USceneCaptureComponent2D* SceneCaptureBaseColor = nullptr;

    // Render targets
    UPROPERTY()
    UTextureRenderTarget2D* ColorRenderTarget = nullptr;

    UPROPERTY()
    UTextureRenderTarget2D* DepthRenderTarget = nullptr;

    UPROPERTY()
    UTextureRenderTarget2D* NormalRenderTarget = nullptr;

    UPROPERTY()
    UTextureRenderTarget2D* BaseColorRenderTarget = nullptr;

    UPROPERTY()
    UConsole* NativeConsole;



    void PrepareRTCpature();
    void CaptureRTColor();
    void CaptureRTNormal();
    void CaptureRTDepth();
    void CaptureRTUV();

    void PrepareCaptureViewportNormal();
    void CaptureViewportNormal(const FString& WorldNormalFilePath, const FString& ScreenNormalFilePath);
    void ExportNormal(TArray<FLinearColor> Data, bool IsFromViewport, const FString& WorldNormalFilePath, const FString& ScreenNormalFilePath);
    void ExportNormal(TArray<FColor> Data, const FString& WorldNormalFilePath, const FString& ScreenNormalFilePath);
    UPROPERTY()
    TArray<FLinearColor> ViewPortDepth;
    TArray<FColor> ViewPortDepth8;
    void PrepareCaptureViewportDepth();
    void CaptureViewportDepth(const FString& FilePath);
    void ExportDepth(TArray<FLinearColor> Data,bool IsFromViewport, const FString& FilePath);

    
    void CaptureViewportColor(const FString& FilePath);
    void PrepareCaptureViewportColor();

    //void ExportRenderTarget(UTextureRenderTarget2D* TextureRenderTarget, const FString& FilePath);
    //void SaveToBmp(TArray<FColor> Data, const FString& FilePath);
    void SaveMats(const FString& FilePath);
    void SaveLandmarks(const FString& FilePath);

    
};