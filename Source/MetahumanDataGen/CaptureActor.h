// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "MyCameraActor.h"
#include "EnvActor.h"
#include "MHActor.h"

#include "CaptureActor.generated.h"

UCLASS()
class METAHUMANDATAGEN_API ACaptureActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACaptureActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	float OrbitRadius = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	float OrbitSpeed = 30.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Capture")
	AMHActor* mhActor;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Capture")
	AEnvActor* envActor;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Capture")
	AMyCameraActor* camActor;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	int32 SnapShotPerHDRI = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	int32 HDRINumPerPlayer = 5;



	int32 FrameID = 0;
	float CurrentAngle = 0.f;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	void RandomRotation();
	void RefreshTargetPosition();
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	int CaptureCount = 0;
};
