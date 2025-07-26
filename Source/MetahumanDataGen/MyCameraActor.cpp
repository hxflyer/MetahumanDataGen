// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Runtime/Core/Public/HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Serialization/BufferArchive.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/PostProcessVolume.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Camera/CameraComponent.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

#include "Components/SkinnedMeshComponent.h"
#include "SkeletalRenderPublic.h"            // FFinalSkinVertex
#include "Rendering/SkeletalMeshRenderData.h"// LODRenderData / IndexBuffer

#include "Engine/Console.h"
#include "Runtime/Engine/Classes/Engine/GameViewportClient.h"
#include "Runtime/Engine/Public/BufferVisualizationData.h"

#include "Math/RotationMatrix.h"

#include "ImageUtil.h"

//USceneCaptureComponent2D* SceneCaptureBaseColor = nullptr;
AMyCameraActor::AMyCameraActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    // Create ALL components first
    SceneCaptureColor = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
    SceneCaptureDepth = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureDepth"));
    SceneCaptureNormal = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureNormal"));
    SceneCaptureBaseColor = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureBaseColor"));

    // THEN attach them
    SceneCaptureColor->SetupAttachment(RootComponent);
    SceneCaptureDepth->SetupAttachment(RootComponent);
    SceneCaptureNormal->SetupAttachment(RootComponent);
    SceneCaptureBaseColor->SetupAttachment(RootComponent);

    BaseDir = FPaths::ProjectSavedDir() + TEXT("CapturedFrames/");
    //mh = NewObject<UMetaHumanController>(this, UMetaHumanController::StaticClass());
    

    if (GEngine)
    {
        GEngine->Exec(GetWorld(), TEXT("r.HairStrands.Simulation 0"));
        GEngine->Exec(GetWorld(), TEXT("r.HairStrands.SkyLighting.IntegrationType 1"));
        GEngine->Exec(GetWorld(), TEXT("r.HairStrands.SkyLighting.SampleCount 8")); 
        
        //GEngine->Exec(GetWorld(), TEXT("r.SceneCapture.DeferredCaptures 0"));
        //GEngine->Exec(GetWorld(), TEXT("r.SceneCapture.DeferredCaptures 2"));

        FString Command = FString::Printf(TEXT("r.SetRes %dx%d%s"), CaptureSize.X, CaptureSize.Y, TEXT("w"));
        GEngine->Exec(nullptr, *Command);
    }



}
AMyCameraActor::~AMyCameraActor()
{
}

void AMyCameraActor::BeginPlay()
{
    Super::BeginPlay();

    CamComp = GetCameraComponent();
    if (!CamComp)
    {
        UE_LOG(LogTemp, Error, TEXT("No camera component found!"));
        return;
    }
    if (GEngine)
    {
        //GEngine->Exec(GetWorld(), TEXT("r.DefaultBackBufferPixelFormat 3"));
        //GEngine->Exec(GetWorld(), TEXT("r.HairStrands.Enable 1"));
        //GEngine->Exec(GetWorld(), TEXT("r.HairStrands.Visibility 1"));
    }
    NativeConsole = GEngine->GameViewport->ViewportConsole;
    SceneCaptureColor->FOVAngle = CamComp->FieldOfView;
    SceneCaptureDepth->FOVAngle = CamComp->FieldOfView;
    SceneCaptureNormal->FOVAngle = CamComp->FieldOfView;
    SceneCaptureBaseColor->FOVAngle = CamComp->FieldOfView;
    // Set up color capture render target
    ColorRenderTarget = NewObject<UTextureRenderTarget2D>(this);
    ColorRenderTarget->InitAutoFormat(CaptureSize.X, CaptureSize.Y);
    //CaptureRenderTarget->RenderTargetFormat = RTF_RGBA8;
    ColorRenderTarget->ClearColor = FLinearColor::Black;
    ColorRenderTarget->bUseLegacyGamma = true;
    ColorRenderTarget->UpdateResourceImmediate(true);

    // Set up depth capture render target
    DepthRenderTarget = NewObject<UTextureRenderTarget2D>(this);
    DepthRenderTarget->InitAutoFormat(CaptureSize.X, CaptureSize.Y);
    DepthRenderTarget->RenderTargetFormat = RTF_R32f;
    DepthRenderTarget->ClearColor = FLinearColor::Black;
    DepthRenderTarget->UpdateResourceImmediate(true);

    // Set up normal capture render target
    NormalRenderTarget = NewObject<UTextureRenderTarget2D>(this);
    NormalRenderTarget->InitAutoFormat(CaptureSize.X, CaptureSize.Y);
    NormalRenderTarget->RenderTargetFormat = RTF_RGBA16f;
    //NormalRenderTarget->RenderTargetFormat = RTF_RGBA16f;
    //NormalRenderTarget->ClearColor = FLinearColor::Black;
    NormalRenderTarget->ClearColor = FLinearColor(0.5f, 0.5f, 1.0f, 1.0f);
    NormalRenderTarget->UpdateResourceImmediate(true);

    // Set up base color capture render target
    BaseColorRenderTarget = NewObject<UTextureRenderTarget2D>(this);
    BaseColorRenderTarget->InitAutoFormat(CaptureSize.X, CaptureSize.Y);
    //BaseColorRenderTarget->RenderTargetFormat = RTF_RGBA8;
    BaseColorRenderTarget->ClearColor = FLinearColor::Black;
    BaseColorRenderTarget->UpdateResourceImmediate(true);

    // Configure color capture
    SceneCaptureColor->TextureTarget = ColorRenderTarget;
    SceneCaptureColor->bRenderInMainRenderer = true;
    SceneCaptureColor->CaptureSource = ESceneCaptureSource::SCS_FinalToneCurveHDR;
    SceneCaptureColor->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
    SceneCaptureColor->bCaptureEveryFrame = true;
    SceneCaptureColor->bCaptureOnMovement = false;
    SceneCaptureColor->ShowFlags.TemporalAA = true;
    SceneCaptureColor->ShowFlags.AntiAliasing = true;
    SceneCaptureColor->ShowFlags.ScreenSpaceAO = true;
    SceneCaptureColor->ShowFlags.DistanceFieldAO = true;
    SceneCaptureColor->ShowFlags.ContactShadows = true;
    SceneCaptureColor->ShowFlags.Bloom = true;
    SceneCaptureColor->ShowFlags.MotionBlur = false;
    SceneCaptureColor->bAlwaysPersistRenderingState = true;

    // Configure depth capture
    SceneCaptureDepth->TextureTarget = DepthRenderTarget;
    SceneCaptureDepth->bRenderInMainRenderer = true;
    SceneCaptureDepth->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
    SceneCaptureDepth->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
    SceneCaptureDepth->bCaptureEveryFrame = true;
    SceneCaptureDepth->bCaptureOnMovement = false;
    SceneCaptureDepth->ShowFlags.TemporalAA = false;
    SceneCaptureDepth->ShowFlags.MotionBlur = false;
    SceneCaptureDepth->ShowFlags.Hair = true;
    SceneCaptureDepth->ShowFlags.SetDistanceFieldAO(false);
    SceneCaptureDepth->ShowFlags.SetScreenSpaceAO(false);
    SceneCaptureDepth->bAlwaysPersistRenderingState = true;
    SceneCaptureDepth->HiddenActors.AddUnique(HDRI);


    // Configure normal capture
    SceneCaptureNormal->TextureTarget = NormalRenderTarget;
    SceneCaptureNormal->bRenderInMainRenderer = true;
    SceneCaptureNormal->CaptureSource = ESceneCaptureSource::SCS_Normal;
    SceneCaptureNormal->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
    SceneCaptureNormal->bCaptureEveryFrame = true;
    SceneCaptureNormal->bCaptureOnMovement = false;
    SceneCaptureNormal->ShowFlags.TemporalAA = false;
    SceneCaptureNormal->ShowFlags.MotionBlur = false;
    SceneCaptureNormal->bAlwaysPersistRenderingState = true;
    SceneCaptureNormal->HiddenActors.AddUnique(HDRI);

    // Configure base color capture
    SceneCaptureBaseColor->TextureTarget = BaseColorRenderTarget;
    SceneCaptureBaseColor->bRenderInMainRenderer = true;
    SceneCaptureBaseColor->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
    SceneCaptureBaseColor->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
    SceneCaptureBaseColor->bCaptureEveryFrame = true;
    SceneCaptureBaseColor->bCaptureOnMovement = false;
    SceneCaptureBaseColor->ShowFlags.TemporalAA = false;
    SceneCaptureBaseColor->ShowFlags.MotionBlur = false;
    SceneCaptureBaseColor->bAlwaysPersistRenderingState = true;
    SceneCaptureBaseColor->HiddenActors.AddUnique(HDRI);
    // Optional: Set view target
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        PC->SetViewTarget(this);
    }

}

void AMyCameraActor::Snapshot(FString id) {

    SnapshotID = id;
    CaptureStep = 0;
    
}


void AMyCameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // Orbit logic
    if (!ColorRenderTarget || !DepthRenderTarget || !NormalRenderTarget || !BaseColorRenderTarget) return;
    if (!IsValid(ColorRenderTarget) || !IsValid(DepthRenderTarget) || !IsValid(NormalRenderTarget) || !IsValid(BaseColorRenderTarget)) return;



    

    if (CaptureStep == 0) {
        UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.00000001f);
        GEngine->Exec(GetWorld(), TEXT("r.HairStrands.SkyLighting.SampleCount 1024"));
        PrepareRTCpature();
        //PrepareCaptureViewportColor();

    }else if (CaptureStep == 1) {
        CaptureRTColor();
        GEngine->Exec(GetWorld(), TEXT("r.HairStrands.SkyLighting.SampleCount 8"));
        CaptureRTDepth();
        CaptureRTNormal();
        //CaptureRTUV();
        CaptureViewportColor(BaseDir + FString::Printf(TEXT("Color_%s.png"), *SnapshotID));
        SaveLandmarks(BaseDir + FString::Printf(TEXT("Landmarks_%s.txt"), *SnapshotID));
        SaveMats(BaseDir + FString::Printf(TEXT("Mats_%s.txt"), *SnapshotID));
    }
    else if (CaptureStep == 3) {
        //PrepareCaptureViewportDepth();
        mh->HideFacialHairs();
    }
    else if (CaptureStep == 5) {
        //CaptureViewportDepth(BaseDir + FString::Printf(TEXT("Depth_%s.png"), *SnapshotID));
    }
    else if (CaptureStep == 6) {
        //PrepareCaptureViewportNormal();
    }
    else if (CaptureStep == 8) {
        //CaptureViewportNormal(BaseDir + FString::Printf(TEXT("WorldNormal_%s.png"), *SnapshotID),BaseDir + FString::Printf(TEXT("ScreenNormal_%s.png"), *SnapshotID));
    }
    else if (CaptureStep == 9) {
        mh->SetFaceUVMat();
        mh->HideHairs();
        mh->HideCloths();
    }
    else if (CaptureStep == 11) {
        CaptureRTUV();
    }
    else if (CaptureStep == 12) {
        mh->ResetFaceMat();
        mh->ShowHairs();
        mh->ShowCloths();
        mh->ShowFacialHairs();
        
        UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
    }
    else if (CaptureStep == 13) {
        CaptureStep = -1;
    }
    if (CaptureStep > -1) {
        CaptureStep++;
    }
}

void AMyCameraActor::SaveMats(const FString& FilePath) {
    FTransform headMat = mh->GetHeadMat();
    FTransform camMat = GetActorTransform();

    FString headString = FString::Printf(TEXT("Head:\nLocation: %s\nRotation: %s\nScale: %s\n"),
        *headMat.GetLocation().ToString().TrimStartAndEnd(),
        *headMat.GetRotation().Rotator().ToString().TrimStartAndEnd(),
        *headMat.GetScale3D().ToString().TrimStartAndEnd());

    FString camString = FString::Printf(TEXT("Camera:\nLocation: %s\nRotation: %s\nScale: %s\n"),
        *camMat.GetLocation().ToString().TrimStartAndEnd(),
        *camMat.GetRotation().Rotator().ToString().TrimStartAndEnd(),
        *camMat.GetScale3D().ToString().TrimStartAndEnd());

    float FOV = 0.0f;
    FOV = CamComp->FieldOfView;


    // Get screen resolution

    FString cameraInfoString = FString::Printf(TEXT("Camera Info:\nFOV: %.2f\nResolution: %dx%d\n"),
        FOV, CaptureSize.X, CaptureSize.Y);

    // Combine all strings
    FString CombinedString = headString + TEXT("\n") + camString + TEXT("\n") + cameraInfoString;
    // Save to file

    FFileHelper::SaveStringToFile(CombinedString, *FilePath);
}

void AMyCameraActor::SaveLandmarks(const FString& FilePath) {
    TArray<FFinalSkinVertex> CPUVerts = mh->GetLandmarks();

    const FTransform& C2W = mh->Face->GetComponentTransform();
    float FOV = CamComp->FieldOfView;

    FTransform CameraTransform = CamComp->GetComponentTransform();
    FMatrix ViewMatrix = CameraTransform.ToInverseMatrixWithScale();
    ViewMatrix = ViewMatrix * FMatrix(FPlane(0, 0, 1, 0), FPlane(1, 0, 0, 0), FPlane(0, 1, 0, 0), FPlane(0, 0, 0, 1));
    FMatrix ProjectionMatrix = FReversedZPerspectiveMatrix(FMath::DegreesToRadians(FOV) * 0.5f, (float)1, (float)1, GNearClippingPlane);
    FMatrix viewProjectionMatrix = ViewMatrix * ProjectionMatrix;
    TArray<FString> Lines;

    for (int32 i = 0; i < CPUVerts.Num(); ++i)
    {
        const FVector WorldPos = C2W.TransformPosition(FVector(CPUVerts[i].Position));
        FVector4 ViewProjPos = viewProjectionMatrix.TransformFVector4(FVector4(WorldPos, 1.0f));
        const float RHW = 1.0f / ViewProjPos.W;
        FPlane posInScreenSpace = FPlane(ViewProjPos.X * RHW, ViewProjPos.Y * RHW, ViewProjPos.Z * RHW, ViewProjPos.W);
        const float normalizedX = (posInScreenSpace.X / 2.f) + 0.5f;
        const float normalizedY = 1.f - (posInScreenSpace.Y / 2.f) - 0.5f;

        FVector2D ScreenPos(
            (normalizedX * (float)CaptureSize.X),
            (normalizedY * (float)CaptureSize.Y)
        );
        Lines.Add(FString::Printf(TEXT("%.6f %.6f %.6f %.3f %.3f"),
            WorldPos.X, WorldPos.Y, WorldPos.Z,
            ScreenPos.X, ScreenPos.Y));
    }

    FString OutStr = FString::Join(Lines, TEXT("\n"));
    FFileHelper::SaveStringToFile(OutStr, *FilePath);
}


void AMyCameraActor::PrepareRTCpature()
{
    if (CamComp)
    {
        FTransform CamTransform = CamComp->GetComponentTransform();
        SceneCaptureColor->SetWorldTransform(CamTransform);
        SceneCaptureDepth->SetWorldTransform(CamTransform);
        SceneCaptureNormal->SetWorldTransform(CamTransform);
        SceneCaptureBaseColor->SetWorldTransform(CamTransform);

    }
    SceneCaptureColor->UpdateComponentToWorld();
    SceneCaptureDepth->UpdateComponentToWorld();
    SceneCaptureNormal->UpdateComponentToWorld();
    SceneCaptureBaseColor->UpdateComponentToWorld();
}

void  AMyCameraActor::PrepareCaptureViewportColor()
{
    
    NativeConsole->ConsoleCommand(TEXT("ViewMode VisualizeBuffer | r.BufferVisualizationTarget FinalImage"));
}

void  AMyCameraActor::PrepareCaptureViewportNormal()
{
    NativeConsole->ConsoleCommand(TEXT("ViewMode VisualizeBuffer | r.BufferVisualizationTarget WorldNormal"));
}

void  AMyCameraActor::PrepareCaptureViewportDepth()
{
    NativeConsole->ConsoleCommand(TEXT("ViewMode VisualizeBuffer | r.BufferVisualizationTarget SceneDepth"));
}


void  AMyCameraActor::CaptureViewportColor(const FString& FilePath)
{
    UWorld* World = GetWorld();
    UGameViewportClient* ViewportClient = World->GetGameViewport();

    FViewport* Viewport = ViewportClient->Viewport;

    FIntPoint ViewportSize = Viewport->GetSizeXY();
    UE_LOG(LogTemp, Error, TEXT("Captured screen color: %d %d"), ViewportSize.X, ViewportSize.Y);

    int32 CropX = (ViewportSize.X - CaptureSize.X) / 2;
    FIntRect CaptureRect(CropX, 0, CropX + CaptureSize.X, CaptureSize.Y);
    TArray<FColor> ScreenPixels;
    bool bSuccess = Viewport->ReadPixels(ScreenPixels, FReadSurfaceDataFlags(), CaptureRect);

    SaveToPng(ScreenPixels, CaptureSize, FilePath);
}

void  AMyCameraActor::CaptureViewportNormal(const FString& WorldNormalFilePath, const FString& ScreenNormalFilePath)
{
    UWorld* World = GetWorld();
    UGameViewportClient* ViewportClient = World->GetGameViewport();

    FViewport* Viewport = ViewportClient->Viewport;

    FIntPoint ViewportSize = Viewport->GetSizeXY();
    UE_LOG(LogTemp, Error, TEXT("Captured screen normal: %d %d"),  ViewportSize.X, ViewportSize.Y);

    int32 CropX = (ViewportSize.X - CaptureSize.X) / 2;
    FIntRect CaptureRect(CropX, 0, CropX + CaptureSize.X, CaptureSize.Y);
    //TArray<FLinearColor> ScreenPixels;
    //bool bSuccess = Viewport->ReadLinearColorPixels(ScreenPixels, FReadSurfaceDataFlags(), CaptureRect);
    //ExportNormal(ScreenPixels,true, WorldNormalFilePath,  ScreenNormalFilePath);

    TArray<FColor> ScreenPixels;
    bool bSuccess = Viewport->ReadPixels(ScreenPixels, FReadSurfaceDataFlags(), CaptureRect);
    ExportNormal(ScreenPixels, WorldNormalFilePath, ScreenNormalFilePath);
}

void  AMyCameraActor::CaptureViewportDepth(const FString& FilePath)
{
    UWorld* World = GetWorld();
    UGameViewportClient* ViewportClient = World->GetGameViewport();

    FViewport* Viewport = ViewportClient->Viewport;

    FIntPoint ViewportSize = Viewport->GetSizeXY();
    UE_LOG(LogTemp, Error, TEXT("Captured screen depth: %d %d"), ViewportSize.X, ViewportSize.Y);

    int32 CropX = (ViewportSize.X - CaptureSize.X) / 2;
    FIntRect CaptureRect(CropX, 0, CropX + CaptureSize.X, CaptureSize.Y);
    
    bool bSuccess = Viewport->ReadLinearColorPixels(ViewPortDepth, FReadSurfaceDataFlags(), CaptureRect);

    ExportDepth(ViewPortDepth,true, FilePath);
}

void  AMyCameraActor::CaptureRTColor()
{
    TArray<FColor> CapturedColor;
    FTextureRenderTargetResource* RTResource = ColorRenderTarget->GameThread_GetRenderTargetResource();
    RTResource->ReadPixels(CapturedColor);
    SaveToPng(CapturedColor, CaptureSize, BaseDir + FString::Printf(TEXT("Color_%s.png"), *SnapshotID));
}

void  AMyCameraActor::CaptureRTNormal() {
    SceneCaptureNormal->CaptureScene();
    TArray<FLinearColor> CapturedNormals;
    FTextureRenderTargetResource* NormalRTResource = NormalRenderTarget->GameThread_GetRenderTargetResource();
    
    NormalRTResource->ReadLinearColorPixels(CapturedNormals);
    
    ExportNormal(CapturedNormals, false, BaseDir + FString::Printf(TEXT("WorldNormal1_%s.png"), *SnapshotID),BaseDir + FString::Printf(TEXT("ScreenNormal1_%s.png"), *SnapshotID));
}

void AMyCameraActor::CaptureRTDepth() {
    //TArray<FLinearColor> CapturedDepths;
    FTextureRenderTargetResource* DepthRTResource = DepthRenderTarget->GameThread_GetRenderTargetResource();
    DepthRTResource->ReadLinearColorPixels(ViewPortDepth);
    ExportDepth(ViewPortDepth, false, BaseDir + FString::Printf(TEXT("Depth1_%s.png"), *SnapshotID));
}

void AMyCameraActor::CaptureRTUV()
{
    TArray<FColor> CapturedUV;
    FTextureRenderTargetResource* RTResource = BaseColorRenderTarget->GameThread_GetRenderTargetResource();
    RTResource->ReadPixels(CapturedUV);
    SaveToPng(CapturedUV, CaptureSize, BaseDir + FString::Printf(TEXT("UV_%s.png"), *SnapshotID));

}







void AMyCameraActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    // Detach render targets from capture components
    if (SceneCaptureColor)
        SceneCaptureColor->TextureTarget = nullptr;

    if (SceneCaptureDepth)
        SceneCaptureDepth->TextureTarget = nullptr;

    if (SceneCaptureNormal)
        SceneCaptureNormal->TextureTarget = nullptr;

    if (SceneCaptureBaseColor)
        SceneCaptureBaseColor->TextureTarget = nullptr;
}

void AMyCameraActor::ExportDepth(TArray<FLinearColor> Data,bool IsFromViewport, const FString& FilePath)
{
    

    ViewPortDepth8.Empty();
    ViewPortDepth8.Reserve(Data.Num());

    
    for (const FLinearColor& DepthPixel : Data)
    {
        FColor DepthColor= FColor::White;
        float depth;
        bool isBackground = false;
        if (IsFromViewport) {
            if (DepthPixel.R < 0.9) {
                depth = DepthPixel.R * 10000000;
            }
            else {
                isBackground = true;
            }
        }
        else {
            if (DepthPixel.R < 90) {
                depth = DepthPixel.R * 100000;
            }
            else {
                isBackground = true;
            }
        }
        if (!isBackground) {
            DepthColor.R = FMath::Clamp(FMath::FloorToInt(depth / 65025), 0, 255);
            DepthColor.G = FMath::Clamp(FMath::FloorToInt((depth - DepthColor.R * 65025) / 255), 0, 255);
            DepthColor.B = FMath::Clamp(FMath::FloorToInt(depth - DepthColor.R * 65025 - DepthColor.G * 255), 0, 255);
        }
        ViewPortDepth8.Add(DepthColor);
    }
    SaveToPng(ViewPortDepth8,CaptureSize, FilePath);
}



void AMyCameraActor::ExportNormal(TArray<FLinearColor> Data,bool IsFromViewport, const FString& WorldNormalFilePath,const FString& ScreenNormalFilePath)
{
    UE_LOG(LogTemp, Error, TEXT("IsFromViewport: %s"), IsFromViewport ? TEXT("true") : TEXT("false"));

    if (true) PrintMinMax(Data);

    FQuat CameraRotation = CamComp->GetComponentRotation().Quaternion();
    FQuat InverseCameraRotation = CameraRotation.Inverse();

    FMatrix CameraMatrixTransposed = FRotationMatrix::Make(CameraRotation).GetTransposed();
    

    // Transform normals from world space to view space
    TArray<FColor> ScreenSpaceNormals;
    ScreenSpaceNormals.Reserve(Data.Num());
    //TArray<FColor> WorldSpaceNormals;
    //WorldSpaceNormals.Reserve(Data.Num());

    FColor flatNormal = FColor(127, 127, 255, 255);
    int pixelID = 0;
    for (const FLinearColor& WorldNormalColor : Data)
    {
        
        FVector WorldNormal;
        if (IsFromViewport) {
            //WorldNormal.X = WorldNormalColor.R < 0.00313067 ? WorldNormalColor.R * 12.92 : pow(WorldNormalColor.R, (1.0 / 2.4)) * 1.055 - 0.055;
            //WorldNormal.Y = WorldNormalColor.G < 0.00313067 ? WorldNormalColor.G * 12.92 : pow(WorldNormalColor.G, (1.0 / 2.4)) * 1.055 - 0.055;
            //WorldNormal.Z = WorldNormalColor.B < 0.00313067 ? WorldNormalColor.B * 12.92 : pow(WorldNormalColor.B, (1.0 / 2.4)) * 1.055 - 0.055;
            WorldNormal.X = WorldNormalColor.R;
            WorldNormal.Y = WorldNormalColor.G;
            WorldNormal.Z = WorldNormalColor.B;
            WorldNormal = (WorldNormal - 0.5) * 2;
        }
        else
        {
            WorldNormal.X = WorldNormalColor.R;
            WorldNormal.Y = WorldNormalColor.G;
            WorldNormal.Z = WorldNormalColor.B;
            //WorldNormal = (WorldNormal + 1) * 0.5f;
        }
        //WorldNormal = (WorldNormal - 0.5f) * 2;
        //WorldNormal.Normalize();
        //WorldNormal = (WorldNormal + 1)*0.5f;
        /*FColor WorldSpaceNormalColor;

        WorldSpaceNormalColor.R = FMath::Clamp(FMath::RoundToInt(WorldNormal.X * 255.0f), 0, 255);
        WorldSpaceNormalColor.B = FMath::Clamp(FMath::RoundToInt(WorldNormal.Y  * 255.0f), 0, 255);
        WorldSpaceNormalColor.G = FMath::Clamp(FMath::RoundToInt(WorldNormal.Z  * 255.0f), 0, 255);
        WorldSpaceNormalColor.A = 255;
        WorldSpaceNormals.Add(WorldSpaceNormalColor);*/

        bool isBackground = false;
        
        
        if (ViewPortDepth8[pixelID].R == 255 and ViewPortDepth8[pixelID].G == 255 && ViewPortDepth8[pixelID].B == 255) {
            ScreenSpaceNormals.Add(flatNormal);
            isBackground = true;
        }
            
            //
        
        if (!isBackground) {
            
            FVector ViewNormal = CameraMatrixTransposed.TransformVector(WorldNormal);
            //FVector ViewNormal = InverseCameraRotation.RotateVector(WorldNormal);
            // Normalize after transformation
            ViewNormal.Normalize();

            // Convert back to color (0 to 1 range, then 0 to 255)
            FColor ScreenSpaceNormalColor;
            ScreenSpaceNormalColor.R = FMath::Clamp(FMath::RoundToInt((ViewNormal.Y + 1.0f) * 0.5f * 255.0f), 0, 255);
            ScreenSpaceNormalColor.G = FMath::Clamp(FMath::RoundToInt((ViewNormal.Z + 1.0f) * 0.5f * 255.0f), 0, 255);
            ScreenSpaceNormalColor.B = FMath::Clamp(FMath::RoundToInt((-ViewNormal.X + 1.0f) * 0.5f * 255.0f), 0, 255);
            ScreenSpaceNormalColor.A = 255;
            ScreenSpaceNormals.Add(ScreenSpaceNormalColor);
        }
        pixelID++;
    }

    //SaveToPng(WorldSpaceNormals, CaptureSize, WorldNormalFilePath);
    SaveToPng(ScreenSpaceNormals,CaptureSize, ScreenNormalFilePath);
}


void AMyCameraActor::ExportNormal(TArray<FColor> Data, const FString& WorldNormalFilePath, const FString& ScreenNormalFilePath)
{
    if (true) PrintMinMax(Data);

    FQuat CameraRotation = CamComp->GetComponentRotation().Quaternion();
    FQuat InverseCameraRotation = CameraRotation.Inverse();

    FMatrix CameraMatrixTransposed = FRotationMatrix::Make(CameraRotation).GetTransposed();


    // Transform normals from world space to view space
    TArray<FColor> ScreenSpaceNormals;
    ScreenSpaceNormals.Reserve(Data.Num());
    TArray<FColor> WorldSpaceNormals;
    WorldSpaceNormals.Reserve(Data.Num());

    FColor flatNormal = FColor(127, 127, 255, 255);
    int pixelID = 0;
    for (const FColor& WorldNormalColor : Data)
    {

        FVector WorldNormal;
        WorldNormal.X = 1.0f * WorldNormalColor.R / 255;
        WorldNormal.Y = 1.0f * WorldNormalColor.G / 255;
        WorldNormal.Z = 1.0f * WorldNormalColor.B / 255;
        WorldNormal = (WorldNormal - 0.5) * 2;

        //WorldNormal = (WorldNormal - 0.5f) * 2;
        //WorldNormal.Normalize();
        //WorldNormal = (WorldNormal + 1)*0.5f;
        //FColor WorldSpaceNormalColor;

        WorldSpaceNormals.Add(WorldNormalColor);

        bool isBackground = false;


        if (ViewPortDepth8[pixelID].R == 255 and ViewPortDepth8[pixelID].G == 255 && ViewPortDepth8[pixelID].B == 255) {
            ScreenSpaceNormals.Add(flatNormal);
            isBackground = true;
        }

        
        if (!isBackground) {

            FVector ViewNormal = CameraMatrixTransposed.TransformVector(WorldNormal);
            //FVector ViewNormal = InverseCameraRotation.RotateVector(WorldNormal);
            // Normalize after transformation
            ViewNormal.Normalize();

            // Convert back to color (0 to 1 range, then 0 to 255)
            FColor ScreenSpaceNormalColor;
            ScreenSpaceNormalColor.R = FMath::Clamp(FMath::RoundToInt((ViewNormal.Y + 1.0f) * 0.5f * 255.0f), 0, 255);
            ScreenSpaceNormalColor.G = FMath::Clamp(FMath::RoundToInt((ViewNormal.Z + 1.0f) * 0.5f * 255.0f), 0, 255);
            ScreenSpaceNormalColor.B = FMath::Clamp(FMath::RoundToInt((-ViewNormal.X + 1.0f) * 0.5f * 255.0f), 0, 255);
            ScreenSpaceNormalColor.A = 255;
            ScreenSpaceNormals.Add(ScreenSpaceNormalColor);
        }
        pixelID++;
    }

    SaveToPng(WorldSpaceNormals, CaptureSize, WorldNormalFilePath);
    SaveToPng(ScreenSpaceNormals, CaptureSize, ScreenNormalFilePath);
}

