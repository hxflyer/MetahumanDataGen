// Fill out your copyright notice in the Description page of Project Settings.

#include "PostProcessCaptureCameraActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Console.h"

#include "Runtime/Engine/Classes/Engine/GameViewportClient.h"
#include "Runtime/Engine/Public/BufferVisualizationData.h"

APostProcessCaptureCameraActor::APostProcessCaptureCameraActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    
    // Create scene capture component
    NormalMapSceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("DepthSceneCapture"));
    NormalMapSceneCapture->SetupAttachment(RootComponent);

    BaseDir = FPaths::ProjectSavedDir() + TEXT("CapturedFrames/");
}

void APostProcessCaptureCameraActor::BeginPlay()
{
    Super::BeginPlay();

    // Get camera component
    CameraComponent = GetCameraComponent();
    if (!CameraComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("No camera component found!"));
        return;
    }
    if (GEngine && GEngine->GameViewport)
    {
        UConsole* NativeConsole = GEngine->GameViewport->ViewportConsole;
        if (NativeConsole)
        {
            //NativeConsole->ConsoleCommand(TEXT("ShowFlag.VisualizeBuffer 1"));
            //NativeConsole->ConsoleCommand(TEXT("r.BufferVisualizationOverviewTargets WorldNormal
            NativeConsole->ConsoleCommand(TEXT("ViewMode VisualizeBuffer"));
            NativeConsole->ConsoleCommand(TEXT("r.BufferVisualizationOverviewTargets WorldNormal"));
            UE_LOG(LogTemp, Error, TEXT("r.BufferVisualizationOverviewTargets WorldNormal"));
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("r.BufferVisualizationOverviewTargets WorldNormal nooooooooooooooooooo"));
        }
    }

    static IConsoleVariable* ICVar = IConsoleManager::Get().FindConsoleVariable(FBufferVisualizationData::GetVisualizationTargetConsoleCommandName());
    if (ICVar)
    {

        ICVar->Set(TEXT("WorldNormal"), ECVF_SetByConsole); // TODO: Should wait here for a moment.
        // AsyncTask(ENamedThreads::GameThread, [ViewMode]() {}); // & means capture by reference
        // The ICVar can only be set in GameThread, the CommandDispatcher already enforce this requirement.
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("The BufferVisualization is not correctly configured."));
    }
    //GEngine->Exec(GetWorld(), TEXT("ShowFlag.VisualizeBuffer 1"));
    //GEngine->Exec(GetWorld(), TEXT("r.BufferVisualizationOverviewTargets WorldNormal"));
    //GEngine->Exec(GetWorld(), TEXT("ViewMode VisualizeBuffer"));
    //GEngine->Exec(GetWorld(), TEXT("r.BufferVisualizationOverviewTargets WorldNormal"));
    // 
    //FName ViewModeName = "WorldNormal";
    //FString Command = FString::Printf(TEXT("ShowFlag.BufferVisualization %s"), *ViewModeName.ToString());
    //GetWorld()->Exec(GetWorld(), *Command);
    SetupRenderTarget();
    SetupDepthExtractionMaterial();
    SetupSceneCapture();
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        PC->SetViewTarget(this);
    }

    UE_LOG(LogTemp, Log, TEXT("PostProcessCaptureCameraActor initialized successfully"));
}

void APostProcessCaptureCameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bAutoCapture)
    {
        TimeSinceLastCapture += DeltaTime;

        if (TimeSinceLastCapture >= CaptureInterval)
        {
            CaptureScreenBuffer();
            //CaptureNormal();
            TimeSinceLastCapture = 0.0f;
        }
    }
}

void APostProcessCaptureCameraActor::SetupRenderTarget()
{
    // Create depth render target
    DepthRenderTarget = NewObject<UTextureRenderTarget2D>(this);
    DepthRenderTarget->InitAutoFormat(CaptureSize.X, CaptureSize.Y);
    DepthRenderTarget->RenderTargetFormat = RTF_RGBA8; // Using RGBA8 for post-processed output
    DepthRenderTarget->ClearColor = FLinearColor::Black;
    DepthRenderTarget->bUseLegacyGamma = false;
    DepthRenderTarget->UpdateResourceImmediate(true);

    UE_LOG(LogTemp, Log, TEXT("Depth render target created: %dx%d"), CaptureSize.X, CaptureSize.Y);
}

void APostProcessCaptureCameraActor::SetupDepthExtractionMaterial()
{

    if (DepthExtractionMaterial)
    {
        // Create dynamic material instance
        DepthExtractionMID = UMaterialInstanceDynamic::Create(DepthExtractionMaterial, this);

        if (DepthExtractionMID)
        {
            UE_LOG(LogTemp, Log, TEXT("Depth extraction material loaded and dynamic instance created"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create dynamic material instance"));
        }
    }
}

void APostProcessCaptureCameraActor::SetupSceneCapture()
{
    if (!NormalMapSceneCapture || !DepthRenderTarget)
    {
        UE_LOG(LogTemp, Error, TEXT("Scene capture or render target not available"));
        return;
    }

    // Configure scene capture component
    NormalMapSceneCapture->TextureTarget = DepthRenderTarget;
    NormalMapSceneCapture->bRenderInMainRenderer = true;
    NormalMapSceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR; // Use final color for post-processing
    NormalMapSceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
    NormalMapSceneCapture->bCaptureEveryFrame = false; // Manual capture
    NormalMapSceneCapture->bCaptureOnMovement = false;

    // Match camera settings
    if (CameraComponent)
    {
        NormalMapSceneCapture->FOVAngle = CameraComponent->FieldOfView;
        NormalMapSceneCapture->SetWorldTransform(CameraComponent->GetComponentTransform());
    }

    // Configure show flags for better depth capture
    NormalMapSceneCapture->ShowFlags.TemporalAA = false;
    NormalMapSceneCapture->ShowFlags.MotionBlur = false;
    NormalMapSceneCapture->ShowFlags.Hair = true; // Enable groom rendering
    NormalMapSceneCapture->ShowFlags.SetLighting(true);
    NormalMapSceneCapture->ShowFlags.SetMaterials(true);
    NormalMapSceneCapture->bAlwaysPersistRenderingState = true;

    // Apply post-process material if available
    if (DepthExtractionMID)
    {
        NormalMapSceneCapture->PostProcessSettings.WeightedBlendables.Array.Empty();
        NormalMapSceneCapture->PostProcessSettings.WeightedBlendables.Array.Add(
            FWeightedBlendable(1.0f, DepthExtractionMID)
        );

        UE_LOG(LogTemp, Log, TEXT("Post-process material applied to scene capture"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No post-process material available - capturing without depth extraction"));
    }

    UE_LOG(LogTemp, Log, TEXT("Scene capture configured successfully"));
}

void APostProcessCaptureCameraActor::CaptureScreenBuffer()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UGameViewportClient* ViewportClient = World->GetGameViewport();
    if (!ViewportClient)
    {
        return;
    }

    // Get viewport size
    FViewport* Viewport = ViewportClient->Viewport;
    if (!Viewport)
    {
        return;
    }

    FIntPoint ViewportSize = Viewport->GetSizeXY();

    // Read pixels from the viewport
    TArray<FColor> ScreenPixels;
    bool bSuccess = Viewport->ReadPixels(ScreenPixels);

    if (bSuccess && ScreenPixels.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Captured screen: %dx%d, %d pixels"),
            ViewportSize.X, ViewportSize.Y, ScreenPixels.Num());

        // Process or save the screen pixels
        SaveScreenCapture(ScreenPixels, ViewportSize.X, ViewportSize.Y);
    }
}

void APostProcessCaptureCameraActor::SaveScreenCapture(const TArray<FColor>& PixelData, int32 Width, int32 Height)
{
    FString FileName = FString::Printf(TEXT("ScreenCapture_%04d.bmp"), CaptureCounter);
    FString FullPath = BaseDir + FileName;

    // Save using your existing BMP save function
    SaveToBmp(PixelData, FullPath, Width, Height);

    CaptureCounter++;
}

void APostProcessCaptureCameraActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    if (NormalMapSceneCapture)
        NormalMapSceneCapture->TextureTarget = nullptr;
}
void APostProcessCaptureCameraActor::CaptureNormal()
{
    if (!NormalMapSceneCapture || !DepthRenderTarget)
    {
        UE_LOG(LogTemp, Error, TEXT("Scene capture components not ready"));
        return;
    }

    // Update capture transform to match camera
    if (CameraComponent)
    {
        NormalMapSceneCapture->SetWorldTransform(CameraComponent->GetComponentTransform());
        NormalMapSceneCapture->UpdateComponentToWorld();
    }

    // Trigger capture
    NormalMapSceneCapture->CaptureScene();

    // Save to disk
    FString FileName = FString::Printf(TEXT("Depth_%04d.bmp"), CaptureCounter);
    FString FullPath = BaseDir + FileName;

    SaveNormalMapToDisk(FullPath);

    CaptureCounter++;

    UE_LOG(LogTemp, Log, TEXT("Depth captured and saved: %s"), *FullPath);
}

void APostProcessCaptureCameraActor::SaveNormalMapToDisk(const FString& FilePath)
{
    if (!DepthRenderTarget)
    {
        UE_LOG(LogTemp, Error, TEXT("Depth render target is null"));
        return;
    }

    // Read pixels from render target
    TArray<FColor> CapturedPixels;
    FTextureRenderTargetResource* TextureResource = DepthRenderTarget->GameThread_GetRenderTargetResource();

    if (!TextureResource)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get render target resource"));
        return;
    }

    TextureResource->ReadPixels(CapturedPixels);

    if (CapturedPixels.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No pixels captured from render target"));
        return;
    }

    // Save as BMP
    SaveToBmp(CapturedPixels, FilePath,CaptureSize.X, CaptureSize.Y);
}

void APostProcessCaptureCameraActor::SaveToBmp(const TArray<FColor>& ImageData, const FString& FilePath, int32 Width, int32 Height)
{
    // BMP file format structures
#pragma pack(push, 1)
    struct BMPFileHeader
    {
        uint16 bfType = 0x4D42; // "BM"
        uint32 bfSize;
        uint16 bfReserved1 = 0;
        uint16 bfReserved2 = 0;
        uint32 bfOffBits = 54; // Size of headers
    };

    struct BMPInfoHeader
    {
        uint32 biSize = 40;
        int32 biWidth;
        int32 biHeight;
        uint16 biPlanes = 1;
        uint16 biBitCount = 24; // 24-bit RGB
        uint32 biCompression = 0; // No compression
        uint32 biSizeImage = 0;
        int32 biXPelsPerMeter = 2835; // 72 DPI
        int32 biYPelsPerMeter = 2835; // 72 DPI
        uint32 biClrUsed = 0;
        uint32 biClrImportant = 0;
    };
#pragma pack(pop)

    // Prepare headers
    BMPFileHeader FileHeader;
    BMPInfoHeader InfoHeader;

    InfoHeader.biWidth = Width;
    InfoHeader.biHeight = Height;

    // Calculate row padding (BMP rows must be multiple of 4 bytes)
    int32 RowPadding = (4 - (InfoHeader.biWidth * 3) % 4) % 4;
    int32 RowSize = InfoHeader.biWidth * 3 + RowPadding;
    InfoHeader.biSizeImage = RowSize * InfoHeader.biHeight;
    FileHeader.bfSize = 54 + InfoHeader.biSizeImage;

    // Create output array
    TArray<uint8> BMPData;
    BMPData.Reserve(FileHeader.bfSize);

    // Write headers
    BMPData.Append((uint8*)&FileHeader, sizeof(FileHeader));
    BMPData.Append((uint8*)&InfoHeader, sizeof(InfoHeader));

    // Write pixel data (BMP stores bottom-to-top, BGR format)
    for (int32 y = InfoHeader.biHeight - 1; y >= 0; y--)
    {
        for (int32 x = 0; x < InfoHeader.biWidth; x++)
        {
            const FColor& Pixel = ImageData[y * InfoHeader.biWidth + x];
            BMPData.Add(Pixel.B);
            BMPData.Add(Pixel.G);
            BMPData.Add(Pixel.R);
        }

        // Add row padding
        for (int32 p = 0; p < RowPadding; p++)
        {
            BMPData.Add(0);
        }
    }

    // Ensure directory exists
    FString Directory = FPaths::GetPath(FilePath);
    if (!IFileManager::Get().DirectoryExists(*Directory))
    {
        IFileManager::Get().MakeDirectory(*Directory, true);
    }

    // Save to file
    if (FFileHelper::SaveArrayToFile(BMPData, *FilePath))
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully saved depth BMP to: %s"), *FilePath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save BMP to: %s"), *FilePath);
    }
}

