// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Components/DirectionalLightComponent.h"

#include "Engine/TextureCube.h"
//#include "HDRIBackdrop.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "EnvActor.generated.h"

UCLASS()
class METAHUMANDATAGEN_API AEnvActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnvActor();
	virtual ~AEnvActor();

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Capture")
	AActor* HDRI;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Capture")
	AActor* DirectionalLight;

	UPROPERTY()
	TArray<UTextureCube*> HDRITextures;

	void RandomLighting( float MinIntensity, float MaxIntensity);
	void SetLightDir(FVector NewDirection);
	void LoadHDRIByID(int32 TextureIndex);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void InitHDRIsFromFolder(const FString& Folder);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
