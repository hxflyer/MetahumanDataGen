// Fill out your copyright notice in the Description page of Project Settings.


#include "MHActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

#include "Components/SkinnedMeshComponent.h"
#include "SkeletalRenderPublic.h"            // FFinalSkinVertex
#include "Rendering/SkeletalMeshRenderData.h"// LODRenderData / IndexBuffer
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
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

    // Search for Blueprints
    FARFilter BlueprintFilter;
    BlueprintFilter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    BlueprintFilter.PackagePaths.Add(FName(*Folder));
    BlueprintFilter.bRecursivePaths = true;

    TArray<FAssetData> BlueprintAssetDataArray;
    AssetRegistry.GetAssets(BlueprintFilter, BlueprintAssetDataArray);

    BlueprintAssetDataArray.Sort([](const FAssetData& A, const FAssetData& B)
    {
        return A.AssetName.ToString() < B.AssetName.ToString();
    });

    for (const FAssetData& AssetData : BlueprintAssetDataArray)
    {
        if (UBlueprint* BluePrint = Cast<UBlueprint>(AssetData.GetAsset()))
        {
            FString name = AssetData.GetFullName();
            MHAssets.Add(BluePrint);
            UE_LOG(LogTemp, Log, TEXT("find blueprint: %s"), *BluePrint->GetName());
        }
    }

    // Search for Materials named MI_Face_Skin_Baked_LOD0
    FARFilter MaterialFilter;
    MaterialFilter.ClassPaths.Add(UMaterialInterface::StaticClass()->GetClassPathName());
    MaterialFilter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());
    MaterialFilter.ClassPaths.Add(UMaterialInstance::StaticClass()->GetClassPathName());
    MaterialFilter.ClassPaths.Add(UMaterialInstanceConstant::StaticClass()->GetClassPathName());
     
    MaterialFilter.PackagePaths.Add(FName(*Folder));
    MaterialFilter.bRecursivePaths = true;

    TArray<FAssetData> MaterialAssetDataArray;
    AssetRegistry.GetAssets(MaterialFilter, MaterialAssetDataArray);


    // Map to group hair materials by their parent folder
    TMap<FString, TArray<UMaterialInstanceConstant*>> HairMaterialMap;

    for (const FAssetData& AssetData : MaterialAssetDataArray)
    {
        FString AssetName = AssetData.AssetName.ToString();
        if (AssetName == TEXT("MI_Face_Skin_Baked_LOD0"))
        {
            
            if (UMaterialInstanceConstant* Material = Cast<UMaterialInstanceConstant>(AssetData.GetAsset()))
            {
                FaceSkinMaterials.Add(Material);
                UE_LOG(LogTemp, Log, TEXT("Found MI_Face_Skin_Baked_LOD0 material: %s"), *AssetData.GetFullName());
            }
        }
        // Search for hair materials with pattern MID_MI_Hair_x where x is a number
        else if (AssetName.StartsWith(TEXT("MID_MI_Hair_")))
        {
            // Extract the suffix after "MID_MI_Hair_"
            FString Suffix = AssetName.RightChop(12); // Remove "MID_MI_Hair_" (12 characters)
            
            // Check if the suffix is a number
            if (Suffix.IsNumeric())
            {
                if (UMaterialInstanceConstant* Material = Cast<UMaterialInstanceConstant>(AssetData.GetAsset()))
                {
                    FString PackagePath = AssetData.PackagePath.ToString();
                    
                    // Check if the path contains "Grooms"
                    if (PackagePath.Contains(TEXT("Grooms")))
                    {
                        // Extract the parent folder name that contains the Grooms folder
                        TArray<FString> PathParts;
                        PackagePath.ParseIntoArray(PathParts, TEXT("/"), true);
                        
                        // Find the folder that contains "Grooms"
                        FString GroupName;
                        for (int32 i = 0; i < PathParts.Num() - 1; i++)
                        {
                            if (PathParts[i + 1] == TEXT("Grooms"))
                            {
                                GroupName = PathParts[i];
                                break;
                            }
                        }
                        
                        if (!GroupName.IsEmpty())
                        {
                            if (!HairMaterialMap.Contains(GroupName))
                            {
                                HairMaterialMap.Add(GroupName, TArray<UMaterialInstanceConstant*>());
                            }
                            HairMaterialMap[GroupName].Add(Material);
                            UE_LOG(LogTemp, Log, TEXT("Found hair material in group %s: %s"), *GroupName, *AssetData.GetFullName());
                        }
                    }
                }
            }
        }
        // Search for eye materials MI_EyeL_Baked and MI_EyeR_Baked
        else if (AssetName == TEXT("MI_EyeL_Baked"))
        {
            if (UMaterialInstanceConstant* Material = Cast<UMaterialInstanceConstant>(AssetData.GetAsset()))
            {
                FString PackagePath = AssetData.PackagePath.ToString();
                
                // Check if the path contains "Face/Materials"
                if (PackagePath.Contains(TEXT("Face/Materials")))
                {
                    LeftEyeMaterials.Add(Material);
                    UE_LOG(LogTemp, Log, TEXT("Found left eye material: %s"), *AssetData.GetFullName());
                }
            }
        }
        else if (AssetName == TEXT("MI_EyeR_Baked"))
        {
            if (UMaterialInstanceConstant* Material = Cast<UMaterialInstanceConstant>(AssetData.GetAsset()))
            {
                FString PackagePath = AssetData.PackagePath.ToString();
                
                // Check if the path contains "Face/Materials"
                if (PackagePath.Contains(TEXT("Face/Materials")))
                {
                    RightEyeMaterials.Add(Material);
                    UE_LOG(LogTemp, Log, TEXT("Found right eye material: %s"), *AssetData.GetFullName());
                }
            }
        }
    }

    // Convert map to array of structs without sorting
    for (auto& Group : HairMaterialMap)
    {
        FHairMaterialGroup MaterialGroup;
        MaterialGroup.Materials = Group.Value;
        HairMaterialGroups.Add(MaterialGroup);
        UE_LOG(LogTemp, Log, TEXT("Hair material group %s has %d materials"), *Group.Key, Group.Value.Num());
    }

    UE_LOG(LogTemp, Log, TEXT("Found %d MI_Face_Skin_Baked_LOD0 materials"), FaceSkinMaterials.Num());
    UE_LOG(LogTemp, Log, TEXT("Found %d hair material groups"), HairMaterialGroups.Num());
    UE_LOG(LogTemp, Log, TEXT("Found %d left eye materials"), LeftEyeMaterials.Num());
    UE_LOG(LogTemp, Log, TEXT("Found %d right eye materials"), RightEyeMaterials.Num());
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
    // Initialize visibility flags
	HairVisible = true;
	FuzzVisible = true;
	BeardVisible = true;
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
                Hair->SetEnableSimulation(false);
                //Hair->SetUseCards(true);
                //Hair->SetForcedLOD(6);
                //Hair->SetForceRenderedLOD(6);
                //UNiagaraComponent* hairNiagara 
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
                Mustache->SetEnableSimulation(false);
            }
            else if (compName.Contains("Beard")) {
                Beard = Cast<UGroomComponent>(Comp);
                Beard->SetEnableSimulation(false);
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
    if (Hair)Hair->SetVisibleFlag(HairVisible);
    if (Eyebrows)Eyebrows->SetVisibleFlag(true);
    if (Fuzz)Fuzz->SetVisibleFlag(FuzzVisible);
    if (Eyelashes)Eyelashes->SetVisibleFlag(true);
    if (Mustache)Mustache->SetVisibleFlag(BeardVisible);
    if (Beard)Beard->SetVisibleFlag(BeardVisible);
    
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
    if (Fuzz)Fuzz->SetHiddenInGame(!FuzzVisible);
}

void AMHActor::ShowFacialHairs() {
    if (Eyelashes)Eyelashes->SetHiddenInGame(false);
    if (Eyebrows)Eyebrows->SetHiddenInGame(false);
    if (Fuzz)Fuzz->SetHiddenInGame(!FuzzVisible);
}

void AMHActor::HideHair() {
    HairVisible = false;
    if (Hair) Hair->SetVisibleFlag(HairVisible);
}

void AMHActor::ShowHair() {
    HairVisible = true;
    if (Hair) Hair->SetVisibleFlag(HairVisible);
}

void AMHActor::HideFuzz() {
    FuzzVisible = false;
    if (Fuzz) Fuzz->SetVisibleFlag(FuzzVisible);
}

void AMHActor::ShowFuzz() {
    FuzzVisible = true;
    if (Fuzz) Fuzz->SetVisibleFlag(FuzzVisible);
}

void AMHActor::HideBeard() {
    BeardVisible = false;
    if (Beard) Beard->SetVisibleFlag(BeardVisible);
}

void AMHActor::ShowBeard() {
    BeardVisible = true;
    if (Beard) Beard->SetVisibleFlag(BeardVisible);
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

void AMHActor::RandomFaceMat()
{
    if (!Face)
    {
        UE_LOG(LogTemp, Warning, TEXT("Face component is null"));
        return;
    }

    if (FaceSkinMaterials.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No face skin materials available"));
        return;
    }

    // Select a random material from FaceSkinMaterials
    int32 RandomIndex = FMath::RandRange(0, FaceSkinMaterials.Num() - 1);
    UMaterialInstanceConstant* RandomMaterial = FaceSkinMaterials[RandomIndex];

    if (RandomMaterial)
    {
        Face->SetMaterial(0, RandomMaterial);
        UE_LOG(LogTemp, Log, TEXT("Applied random face material at index %d to material slot 0"), RandomIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Selected random material is null"));
    }
}

void AMHActor::RandomHairMat()
{
    if (HairMaterialGroups.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No hair material groups available"));
        return;
    }

    // Select a random group
    int32 RandomGroupIndex = FMath::RandRange(0, HairMaterialGroups.Num() - 1);
    const TArray<UMaterialInstanceConstant*>& SelectedGroup = HairMaterialGroups[RandomGroupIndex].Materials;

    if (SelectedGroup.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Selected hair material group is empty"));
        return;
    }

    // Apply materials from the selected group to different hair components
    // Use the first 3 materials in the group regardless of their suffix numbers
    if (Hair && SelectedGroup.Num() > 0)
    {
        Hair->SetMaterial(0, SelectedGroup[0]);
        UE_LOG(LogTemp, Log, TEXT("Applied hair material from group %d, material 0 to Hair"), RandomGroupIndex);
    }

    if (Eyebrows && SelectedGroup.Num() > 1)
    {
        Eyebrows->SetMaterial(0, SelectedGroup[1]);
        UE_LOG(LogTemp, Log, TEXT("Applied hair material from group %d, material 1 to Eyebrows"), RandomGroupIndex);
    }

    if (Eyelashes && SelectedGroup.Num() > 2)
    {
        Eyelashes->SetMaterial(0, SelectedGroup[2]);
        UE_LOG(LogTemp, Log, TEXT("Applied hair material from group %d, material 2 to Eyelashes"), RandomGroupIndex);
    }

    UE_LOG(LogTemp, Log, TEXT("Applied materials from hair group %d with %d materials"), RandomGroupIndex, SelectedGroup.Num());
}

void AMHActor::RandomEyeMat()
{
    if (!Face)
    {
        UE_LOG(LogTemp, Warning, TEXT("Face component is null"));
        return;
    }

    if (LeftEyeMaterials.Num() == 0 || RightEyeMaterials.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No eye materials available - Left: %d, Right: %d"), LeftEyeMaterials.Num(), RightEyeMaterials.Num());
        return;
    }

    // Select random materials from each eye array
    int32 LeftRandomIndex = FMath::RandRange(0, LeftEyeMaterials.Num() - 1);
    int32 RightRandomIndex = FMath::RandRange(0, RightEyeMaterials.Num() - 1);

    UMaterialInstanceConstant* LEyeMat = LeftEyeMaterials[LeftRandomIndex];
    UMaterialInstanceConstant* REyeMat = RightEyeMaterials[RightRandomIndex];

    if (LEyeMat && REyeMat)
    {
        Face->SetMaterial(3, LEyeMat);
        Face->SetMaterial(4, REyeMat);
        UE_LOG(LogTemp, Log, TEXT("Applied random eye materials - Left: index %d to slot 3, Right: index %d to slot 4"), LeftRandomIndex, RightRandomIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Selected eye materials are null - Left: %s, Right: %s"), 
               LEyeMat ? TEXT("Valid") : TEXT("Null"), 
               REyeMat ? TEXT("Valid") : TEXT("Null"));
    }
}

void AMHActor::SetFaceUVMat()
{
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
