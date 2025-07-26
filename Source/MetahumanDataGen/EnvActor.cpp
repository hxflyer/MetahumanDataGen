// Fill out your copyright notice in the Description page of Project Settings.


#include "EnvActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
// Sets default values
AEnvActor::AEnvActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	
}
AEnvActor::~AEnvActor() {

}
// Called when the game starts or when spawned
void AEnvActor::BeginPlay()
{
	Super::BeginPlay();
    InitHDRIsFromFolder(TEXT("/Game/HDRs"));
}


void AEnvActor::RandomLighting( float MinIntensity, float MaxIntensity) {
    // Generate random rotations within the specified ranges
    float RandomY = FMath::RandRange(-90,-20);
    float RandomZ = FMath::RandRange(0, 360);

    // Create new rotation (keeping Z rotation at 0)
    FRotator NewRotation = FRotator(RandomY,0.0f , RandomZ);

    // Apply the rotation to the directional light
    DirectionalLight->SetActorRotation(NewRotation);

    // Generate random intensity
    float RandomIntensity = FMath::RandRange(MinIntensity, MaxIntensity);

    // Apply the intensity to the directional light component
    if (UDirectionalLightComponent* LightComponent = Cast< UDirectionalLightComponent>(DirectionalLight->GetComponentByClass(UDirectionalLightComponent::StaticClass())))
    {
        LightComponent->SetIntensity(RandomIntensity);
        // Generate more natural color variations (warmer tones)
        float RandomR = FMath::RandRange(0.5f, 1.0f);  // Keep red high for warm light
        float RandomG = FMath::RandRange(0.5f, 1.0f);  // Moderate green
        float RandomB = FMath::RandRange(0.5f, 0.8f);  // Lower blue for warm light
        FLinearColor RandomColor = FLinearColor(RandomR, RandomG, RandomB, 1.0f);
        LightComponent->SetLightColor(RandomColor);
    }
}

void AEnvActor::SetLightDir(FVector NewDirection) {
    NewDirection.Normalize();
    FRotator NewRotation = NewDirection.Rotation();
    DirectionalLight->SetActorRotation(NewRotation);
}

// Called every frame
void AEnvActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void AEnvActor::InitHDRIsFromFolder(const FString& Folder)
{
    

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    FARFilter Filter;
    Filter.ClassPaths.Add(UTextureCube::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(*Folder));
    Filter.bRecursivePaths = true;

    TArray<FAssetData> AssetDataArray;
    AssetRegistry.GetAssets(Filter, AssetDataArray);

    for (const FAssetData& AssetData : AssetDataArray)
    {
        if (UTextureCube* TextureCube = Cast<UTextureCube>(AssetData.GetAsset()))
        {
            HDRITextures.Add(TextureCube);
            UE_LOG(LogTemp, Log, TEXT("Loaded HDRI: %s"), *TextureCube->GetName());
        }
    }

}

void AEnvActor::LoadHDRIByID(int32 TextureIndex)
{
    if (!HDRI || !HDRITextures.IsValidIndex(TextureIndex))
    {
        return;
    }
    // Try to call the Blueprint function
    FObjectProperty* CubemapProperty = FindFProperty<FObjectProperty>(HDRI->GetClass(), TEXT("Cubemap"));
    if (CubemapProperty)
    {
        CubemapProperty->SetObjectPropertyValue_InContainer(HDRI, HDRITextures[TextureIndex]);
        HDRI->RerunConstructionScripts();
        UE_LOG(LogTemp, Warning, TEXT("Set Cubemap to: %s"), *HDRITextures[TextureIndex]->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Cubemap property not found"));
    }
    // Recapture sky lighting
    //if (USkyLightComponent* SkyLight = HDRI->GetSkyLightComponent())
    {
        //SkyLight->RecaptureSky();
    }

    UE_LOG(LogTemp, Log, TEXT("Set HDRI %d on backdrop: %s"), TextureIndex, *HDRITextures[TextureIndex]->GetName());
}