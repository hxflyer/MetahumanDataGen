// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


void PrintMinMax(TArray<FLinearColor> Data) {
    FLinearColor MinValues = Data[0];
    FLinearColor MaxValues = Data[0];

    for (const FLinearColor& Pixel : Data)
    {
        // Update minimums
        MinValues.R = FMath::Min(MinValues.R, Pixel.R);
        MinValues.G = FMath::Min(MinValues.G, Pixel.G);
        MinValues.B = FMath::Min(MinValues.B, Pixel.B);
        MinValues.A = FMath::Min(MinValues.A, Pixel.A);

        // Update maximums
        MaxValues.R = FMath::Max(MaxValues.R, Pixel.R);
        MaxValues.G = FMath::Max(MaxValues.G, Pixel.G);
        MaxValues.B = FMath::Max(MaxValues.B, Pixel.B);
        MaxValues.A = FMath::Max(MaxValues.A, Pixel.A);
    }

    // Print the results
    UE_LOG(LogTemp, Log, TEXT("Image Statistics:"));
    UE_LOG(LogTemp, Log, TEXT("R Channel - Min: %f, Max: %f"), MinValues.R, MaxValues.R);
    UE_LOG(LogTemp, Log, TEXT("G Channel - Min: %f, Max: %f"), MinValues.G, MaxValues.G);
    UE_LOG(LogTemp, Log, TEXT("B Channel - Min: %f, Max: %f"), MinValues.B, MaxValues.B);
    UE_LOG(LogTemp, Log, TEXT("A Channel - Min: %f, Max: %f"), MinValues.A, MaxValues.A);
}

void PrintMinMax(TArray<FColor> Data) {
    FColor MinValues = Data[0];
    FColor MaxValues = Data[0];

    for (const FColor& Pixel : Data)
    {
        // Update minimums
        MinValues.R = FMath::Min(MinValues.R, Pixel.R);
        MinValues.G = FMath::Min(MinValues.G, Pixel.G);
        MinValues.B = FMath::Min(MinValues.B, Pixel.B);
        MinValues.A = FMath::Min(MinValues.A, Pixel.A);

        // Update maximums
        MaxValues.R = FMath::Max(MaxValues.R, Pixel.R);
        MaxValues.G = FMath::Max(MaxValues.G, Pixel.G);
        MaxValues.B = FMath::Max(MaxValues.B, Pixel.B);
        MaxValues.A = FMath::Max(MaxValues.A, Pixel.A);
    }

    // Print the results
    UE_LOG(LogTemp, Log, TEXT("Image Statistics:"));
    UE_LOG(LogTemp, Log, TEXT("R Channel - Min: %d, Max: %d"), MinValues.R, MaxValues.R);
    UE_LOG(LogTemp, Log, TEXT("G Channel - Min: %d, Max: %d"), MinValues.G, MaxValues.G);
    UE_LOG(LogTemp, Log, TEXT("B Channel - Min: %d, Max: %d"), MinValues.B, MaxValues.B);
    UE_LOG(LogTemp, Log, TEXT("A Channel - Min: %d, Max: %d"), MinValues.A, MaxValues.A);
}


void SaveToPng(TArray<FColor> Data, const FIntPoint CaptureSize, const FString& FilePath)
{
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

    if (ImageWrapper.IsValid())
    {
        // Convert to RGB only (like your BMP) - discard alpha
        TArray<uint8> RawData;
        RawData.Reserve(Data.Num() * 3); // Only RGB, no alpha

        for (const FColor& Pixel : Data)
        {
            RawData.Add(Pixel.R);
            RawData.Add(Pixel.G);
            RawData.Add(Pixel.B);
            RawData.Add(255);
        }

        // Use RGB format (24-bit) instead of RGBA (32-bit)
        if (ImageWrapper->SetRaw(RawData.GetData(), RawData.Num(), CaptureSize.X, CaptureSize.Y, ERGBFormat::RGBA, 8))
        {
            TArray64<uint8> CompressedData = ImageWrapper->GetCompressed(100);

            if (FFileHelper::SaveArrayToFile(CompressedData, *FilePath))
            {
                UE_LOG(LogTemp, Log, TEXT("Successfully saved RGB PNG to: %s"), *FilePath);
            }
        }
    }
}
void SaveToPng(TArray<FLinearColor> Data, const FIntPoint CaptureSize, const FString& FilePath)
{
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

    if (ImageWrapper.IsValid())
    {
        // Convert to RGB only (like your BMP) - discard alpha
        TArray<uint8> RawData;
        RawData.Reserve(Data.Num() * 3); // Only RGB, no alpha

        for (const FLinearColor& Pixel : Data)
        {
            RawData.Add(FMath::FloorToInt(Pixel.R * 255));
            RawData.Add(FMath::FloorToInt(Pixel.G * 255));
            RawData.Add(FMath::FloorToInt(Pixel.B * 255));
            RawData.Add(255);
        }

        // Use RGB format (24-bit) instead of RGBA (32-bit)
        if (ImageWrapper->SetRaw(RawData.GetData(), RawData.Num(), CaptureSize.X, CaptureSize.Y, ERGBFormat::RGBA, 8))
        {
            TArray64<uint8> CompressedData = ImageWrapper->GetCompressed(100);

            if (FFileHelper::SaveArrayToFile(CompressedData, *FilePath))
            {
                UE_LOG(LogTemp, Log, TEXT("Successfully saved RGB PNG to: %s"), *FilePath);
            }
        }
    }
}
void SaveToBmp(TArray<FColor> data,const FIntPoint CaptureSize, const FString& FilePath)
// BMP file format structures
{
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

    InfoHeader.biWidth = CaptureSize.X;
    InfoHeader.biHeight = CaptureSize.Y;

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
            FColor& Pixel = data[y * InfoHeader.biWidth + x];
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

    // Save to file
    // FString BmpFilePath = FPaths::ChangeExtension(FilePath, TEXT("bmp"));
    if (FFileHelper::SaveArrayToFile(BMPData, *FilePath))
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully saved raw BMP to: %s"), *FilePath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save BMP to: %s"), *FilePath);
    }
}