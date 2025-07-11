// Fill out your copyright notice in the Description page of Project Settings.


#include "MHActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

#include "Components/SkinnedMeshComponent.h"
#include "SkeletalRenderPublic.h"            // FFinalSkinVertex
#include "Rendering/SkeletalMeshRenderData.h"// LODRenderData / IndexBuffer
// Sets default values
AMHActor::AMHActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMHActor::BeginPlay()
{
	Super::BeginPlay();
    InitFromFolder(TEXT("/Game/MetaHumans"));
    LoadMetahumanByID(0);
    
}

void AMHActor::InitFromFolder(const FString& Folder)
{


    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    FARFilter Filter;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(*Folder));
    Filter.bRecursivePaths = true;

    TArray<FAssetData> AssetDataArray;
    AssetRegistry.GetAssets(Filter, AssetDataArray);
    for (const FAssetData& AssetData : AssetDataArray)
    {
        if (UBlueprint* BluePrint = Cast<UBlueprint>(AssetData.GetAsset()))
        {
            FString name = AssetData.GetFullName();
            MHAssets.Add(BluePrint);
            UE_LOG(LogTemp, Log, TEXT("find blueprint: %s"), *BluePrint->GetName());
        }
    }
}

void AMHActor::LoadMetahumanByID(int ID) {
    if (ID == MetahumanID) return;
    UBlueprint* SelectedBlueprint = MHAssets[ID];
    UClass* BlueprintClass = SelectedBlueprint->GeneratedClass;

    if (TargetMetaHuman) {
        if (IsValid(TargetMetaHuman))
        {
            TargetMetaHuman->Destroy();
            UE_LOG(LogTemp, Log, TEXT("Destroyed previous TargetMetaHuman"));
        }
        TargetMetaHuman = nullptr;
    }
    UWorld* World = GetWorld();
    if (World)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Name = FName(*FString::Printf(TEXT("Metahuman_%d"), ID));
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        TargetMetaHuman = World->SpawnActor<AActor>(BlueprintClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);


        if (TargetMetaHuman) {
            MetahumanID = ID;
            InitSetup();
            UE_LOG(LogTemp, Log, TEXT("Successfully spawned new TargetMetaHuman: %s"), *SelectedBlueprint->GetName());
        }
    }
}

void AMHActor::InitSetup() {
    //TArray<AActor*> FoundActors;
    //UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("MH"), FoundActors);
    //for (TActorIterator<AMyBlueprintActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    //{
        //AMyBlueprintActor* MyActor = *ActorItr;
        // Now you have a pointer to an instance of your Blueprint class
        // You can access its properties and call its functions.
    //}
    //TaggedActor = FoundActors[0];

    TSet<UActorComponent*> AllComponents = TargetMetaHuman->GetComponents();

    for (UActorComponent* Comp : AllComponents)
    {
        if (Comp)
        {
            UE_LOG(LogTemp, Warning, TEXT("Component: %s (Class: %s)"),
                *Comp->GetName(),
                *Comp->GetClass()->GetName());

            FString compName = *Comp->GetName();
            if (compName.Contains("Body")) {
                Body = Cast<USkinnedMeshComponent>(Comp);
            }else if (compName.Contains("Face")) {
                UE_LOG(LogTemp, Log, TEXT("Find face"));
                Face = Cast<USkinnedMeshComponent>(Comp);
                FaceSkeletal = Cast<USkeletalMeshComponent>(Face);
                if (Face)
                {
                    FaceMaterials.Empty();
                    for (int32 i = 0; i < Face->GetNumMaterials(); i++)
                    {
                        FaceMaterials.Add(Face->GetMaterial(i));
                        UE_LOG(LogTemp, Log, TEXT("Find face mat"));
                    }
                }
                
            }
            else if (compName.Contains("Hair")) {
                Hair = Cast<UGroomComponent>(Comp);
            }
            else if (compName.Contains("Eyebrows")) {
                Eyebrows = Cast<UGroomComponent>(Comp);
            }
            else if (compName.Contains("Fuzz")) {
                Fuzz = Cast<UGroomComponent>(Comp);
            }
            else if (compName.Contains("Eyelashes")) {
                Eyelashes = Cast<UGroomComponent>(Comp);
            }
            else if (compName.Contains("Mustache")) {
                Mustache = Cast<UGroomComponent>(Comp);
            }
            else if (compName.Contains("Beard")) {
                Beard = Cast<UGroomComponent>(Comp);
            }
            else if (compName.Contains("Torso")) {
                UE_LOG(LogTemp, Log, TEXT("Find Torso"));
                Torso = Comp;
            }
            else if (compName.Contains("Legs")) {
                Legs = Comp;
            }
            else if (compName.Contains("Feet")) {
                Feet = Comp;
            }
            else if (compName.Contains("SkeletalMesh")) {
                DefaultCloth = Comp;
            }
        }
    }
    FindBones();
    SetAnimation();
}

void AMHActor::FindBones()
{
    if (!Face || !Face->GetSkinnedAsset())
    {
        UE_LOG(LogTemp, Warning, TEXT("No skeletal mesh component found!"));
        return;
    }

    const FReferenceSkeleton& RefSkeleton = Face->GetSkinnedAsset()->GetRefSkeleton();

    for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
    {
        FName BoneName = RefSkeleton.GetBoneName(BoneIndex);
        int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
        if (BoneName == "head") {
            HeadBoneIdx = BoneIndex;
            UE_LOG(LogTemp, Warning, TEXT("find head bone at index %d"), BoneIndex);
            break;
        }
    }
}

FTransform AMHActor::GetHeadMat()
{
    const FReferenceSkeleton& RefSkeleton = Face->GetSkinnedAsset()->GetRefSkeleton();
    FTransform headMat = RefSkeleton.GetRawBoneAbsoluteTransform(HeadBoneIdx);
    return headMat;
}

void AMHActor::HideHairs()
{
    if (Hair)Hair->SetVisibleFlag(false);
    if (Eyebrows)Eyebrows->SetVisibleFlag(false);
    if (Fuzz)Fuzz->SetVisibleFlag(false);
    if (Eyelashes)Eyelashes->SetVisibleFlag(false);
    if (Mustache)Mustache->SetVisibleFlag(false);
    if (Beard)Beard->SetVisibleFlag(false);
    
    //Cast<USkinnedMeshComponent>(Torso)->SetVisibleFlag(false);
}
void AMHActor::ShowHairs()
{
    if (Hair)Hair->SetVisibleFlag(true);
    if (Eyebrows)Eyebrows->SetVisibleFlag(true);
    if (Fuzz)Fuzz->SetVisibleFlag(true);
    if (Eyelashes)Eyelashes->SetVisibleFlag(true);
    if (Mustache)Mustache->SetVisibleFlag(true);
    if (Beard)Beard->SetVisibleFlag(true);
    
}
void AMHActor::HideCloths()
{
    if (Torso)Cast<USkinnedMeshComponent>(Torso)->SetVisibility(false);
    if (DefaultCloth)Cast<USkinnedMeshComponent>(DefaultCloth)->SetVisibility(false);
}
void AMHActor::ShowCloths()
{
    if (Torso)Cast<USkinnedMeshComponent>(Torso)->SetVisibility(true);
    if (DefaultCloth)Cast<USkinnedMeshComponent>(DefaultCloth)->SetVisibility(true);
}

void AMHActor::HideFacialHairs() {
    if (Eyelashes)Eyelashes->SetHiddenInGame(true);
    if (Eyebrows)Eyebrows->SetHiddenInGame(true);
    if (Fuzz)Fuzz->SetHiddenInGame(true);
}

void AMHActor::ShowFacialHairs() {
    if (Eyelashes)Eyelashes->SetHiddenInGame(false);
    if (Eyebrows)Eyebrows->SetHiddenInGame(false);
    if (Fuzz)Fuzz->SetHiddenInGame(false);
}

void AMHActor::ResetFaceMat()
{
    if (!Face) return;
    for (int32 i = 0; i < FaceMaterials.Num() && i < Face->GetNumMaterials(); i++)
    {
        Face->SetMaterial(i, FaceMaterials[i]);
    }
}

void AMHActor::PrepareForCapture() {
    Face->TickPose(0.f, false);
    Face->RefreshBoneTransforms(nullptr);
}

TArray<FFinalSkinVertex> AMHActor::GetLandmarks() {
    const int32 LOD = 4;                                // Ò²¿ÉÓÃ Face->GetPredictedLODLevel()
    TArray<FFinalSkinVertex> CPUVerts;
    Face->GetCPUSkinnedVertices(CPUVerts, LOD);
    return CPUVerts;
}

void AMHActor::SetAnimation()
{
    if (!Face || !AnimSequence)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid SkeletalMeshComponent or AnimSequence"));
        return;
    }
    // Set the "Anim to Play" property
    FaceSkeletal->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    FaceSkeletal->SetAnimation(AnimSequence);
    //FaceSkeletal->Play(false);
    FaceSkeletal->SetPlayRate(0.0f);
    UE_LOG(LogTemp, Log, TEXT("Set animation '%s' on skeletal mesh component"), *AnimSequence->GetName());
}

void AMHActor::RandomExpression() {
    if(FaceSkeletal)FaceSkeletal->SetPosition(FMath::RandRange(0.0f, AnimSequence->GetPlayLength()));
}

void AMHActor::SetFaceUVMat()
{
    UE_LOG(LogTemp, Log, TEXT("ChangeFaceMaterial!!!!!!!!!"));
    if (!FaceUVMat)
    {
        UE_LOG(LogTemp, Log, TEXT("FaceMaterial is null"));
        return;
    }
    UE_LOG(LogTemp, Warning, TEXT("FaceMaterials count: %d"), FaceMaterials.Num());

    if (!Face)
    {
        UE_LOG(LogTemp, Log, TEXT("Face is null"));
        return;
    }
    
   
    // Change material for all slots
    for (int32 i = 0; i < Face->GetNumMaterials(); i++)
    {
        Face->SetMaterial(i, FaceUVMat);
        UE_LOG(LogTemp, Log, TEXT("Set material slot %d to FaceMaterial"), i);
    }
    Face->SetMaterial(1, FaceUVMat);
    Face->SetMaterial(2, DiscardMat);
    Face->SetMaterial(5, DiscardMat);
    Face->SetMaterial(6, DiscardMat);
    Face->SetMaterial(7, DiscardMat);
    Face->SetMaterial(8, DiscardMat);
    Face->SetMaterial(10, DiscardMat);
}

// Called every frame
void AMHActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

