// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GroomComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "MHActor.generated.h"

UCLASS()
class METAHUMANDATAGEN_API AMHActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMHActor();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	UMaterialInterface* FaceUVMat;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	UMaterialInterface* DiscardMat;
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Capture")
	AActor* TargetMetaHuman;
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Capture")
	UAnimSequence* AnimSequence;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void PrepareForCapture();
	void SetFaceUVMat();
	void ResetFaceMat();
	void HideHairs();
	void ShowHairs();
	void HideFacialHairs();
	void ShowFacialHairs();
	void HideCloths();
	void ShowCloths();
	TArray<FFinalSkinVertex> GetLandmarks();
	FTransform GetHeadMat();

	void LoadMetahumanByID(int id);

	void RandomExpression();
	int32 MetahumanID = -1;

	// Store found actors
	UPROPERTY()
	AActor*  TaggedActor;
	UPROPERTY()
	TArray<UMaterialInterface*> FaceMaterials;
	UPROPERTY()
	TArray <UBlueprint*> MHAssets;
	UPROPERTY()
	USkinnedMeshComponent* Body;
	UPROPERTY()
	USkinnedMeshComponent* Face;
	UPROPERTY()
	USkeletalMeshComponent* FaceSkeletal;
	UGroomComponent* Hair;
	UGroomComponent* Eyebrows;
	UGroomComponent* Fuzz;
	UGroomComponent* Eyelashes;
	UGroomComponent*  Mustache;
	UGroomComponent*  Beard;
	UActorComponent*  Torso;
	UActorComponent*  Legs;
	UActorComponent*  Feet;
	UActorComponent* DefaultCloth;
private:

	void InitFromFolder(const FString& Folder);
	void InitSetup();
	

	int32 HeadBoneIdx;

	void FindBones();
	void SetAnimation();

};
