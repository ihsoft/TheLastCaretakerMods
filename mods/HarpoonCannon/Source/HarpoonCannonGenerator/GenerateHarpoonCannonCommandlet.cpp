// HAND-WRITTEN BUILD TOOL SOURCE: generates the first compile-only cannon
// Blueprint and optical HUD skeleton. This module is never shipped.
// Current Voyage identities are gated by ../GAME_DERIVED_SOURCES.md.

#include "GenerateHarpoonCannonCommandlet.h"

#if WITH_EDITOR

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "BlueprintGraphNames.h"
#include "Camera/CameraComponent.h"
#include "CannonAssetNames.h"
#include "Components/BoxComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Engine/Blueprint.h"
#include "EdGraphSchema_K2.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMesh.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_InputKey.h"
#include "K2Node_Self.h"
#include "K2Node_SpawnActorFromClass.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/Material.h"
#include "MeshDescription.h"
#include "MeshDescriptionBuilder.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BoxElem.h"
#include "StaticMeshAttributes.h"
#include "UObject/SavePackage.h"
#include "VoyageBaseUserWidget.h"
#include "VoyageCustomModuleComponent.h"
#include "VoyageDynamicCollisionComponent.h"
#include "InteractiveInterface.h"
#include "InteractiveObjectComponent.h"
#include "VoyageItem.h"
#include "VoyageModuleActor.h"
#include "VoyageModuleComponent.h"
#include "VoyageVehiclePawn.h"
#include "VoyagePlayerController.h"
#include "WidgetBlueprint.h"

namespace
{
namespace GraphPins = BlueprintGraphNames::Pins;
namespace BinaryPins = BlueprintGraphNames::Pins::Binary;

template <typename NodeType>
NodeType* FinishGraphNode(NodeType* Node, UEdGraph* Graph, int32 X, int32 Y)
{
    Node->CreateNewGuid();
    Node->PostPlacedNewNode();
    Node->SetFlags(RF_Transactional);
    Node->AllocateDefaultPins();
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    return Node;
}

UK2Node_SpawnActorFromClass* FinishSpawnActorNode(
    UK2Node_SpawnActorFromClass* Node,
    UEdGraph* Graph,
    int32 X,
    int32 Y)
{
    Node->CreateNewGuid();
    Node->SetFlags(RF_Transactional);
    Node->AllocateDefaultPins();
    Node->PostPlacedNewNode();
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    return Node;
}

UK2Node_CallFunction* AddCall(UEdGraph* Graph, UFunction* Function, int32 X, int32 Y)
{
    check(Function);
    UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(Graph);
    Node->SetFromFunction(Function);
    return FinishGraphNode(Node, Graph, X, Y);
}

UK2Node_VariableGet* AddVariableGet(
    UEdGraph* Graph,
    const FName VariableName,
    int32 X,
    int32 Y)
{
    UK2Node_VariableGet* Node = NewObject<UK2Node_VariableGet>(Graph);
    Node->VariableReference.SetSelfMember(VariableName);
    return FinishGraphNode(Node, Graph, X, Y);
}

UK2Node_VariableSet* AddVariableSet(
    UEdGraph* Graph,
    const FName VariableName,
    int32 X,
    int32 Y)
{
    UK2Node_VariableSet* Node = NewObject<UK2Node_VariableSet>(Graph);
    Node->VariableReference.SetSelfMember(VariableName);
    return FinishGraphNode(Node, Graph, X, Y);
}

UEdGraphPin* RequirePin(UEdGraphNode* Node, const FName Name)
{
    UEdGraphPin* Pin = Node->FindPin(Name);
    checkf(Pin, TEXT("Missing pin '%s' on node '%s'"), *Name.ToString(), *Node->GetName());
    return Pin;
}

void SetDefault(UEdGraphNode* Node, const FName Name, const TCHAR* Value)
{
    RequirePin(Node, Name)->DefaultValue = Value;
}

void SetDefault(UEdGraphNode* Node, const FName Name, const int32 Value)
{
    RequirePin(Node, Name)->DefaultValue = LexToString(Value);
}

bool Connect(const UEdGraphSchema_K2* Schema, UEdGraphPin* Source, UEdGraphPin* Target)
{
    if (!Schema->TryCreateConnection(Source, Target))
    {
        UE_LOG(LogTemp, Error, TEXT("Could not connect %s.%s to %s.%s"),
            *Source->GetOwningNode()->GetName(), *Source->PinName.ToString(),
            *Target->GetOwningNode()->GetName(), *Target->PinName.ToString());
        return false;
    }
    return true;
}

bool SaveGeneratedAsset(UPackage* Package, UObject* Asset)
{
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(),
        FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
}

bool CompileGeneratedBlueprint(UBlueprint* Blueprint)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error)
    {
        UE_LOG(LogTemp, Error, TEXT("Compilation failed for %s"), *Blueprint->GetPathName());
        return false;
    }

    return true;
}

bool SaveCompiledBlueprint(UPackage* Package, UBlueprint* Blueprint)
{
    if (!CompileGeneratedBlueprint(Blueprint))
    {
        return false;
    }

    return SaveGeneratedAsset(Package, Blueprint);
}

UVoyageItem* CreateLeafItemReferenceStub()
{
    UPackage* Package = CreatePackage(CannonAssetNames::LeafItemPackageName);
    UVoyageItem* Item = NewObject<UVoyageItem>(
        Package,
        FName(CannonAssetNames::LeafItemAssetName),
        RF_Public | RF_Standalone);
    if (!Item || !SaveGeneratedAsset(Package, Item))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the editor-only leaf-item reference stub"));
        return nullptr;
    }
    return Item;
}

UBlueprint* CreateCameraDroneReferenceStub()
{
    UPackage* Package = CreatePackage(CannonAssetNames::CameraDronePackageName);
    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
        AActor::StaticClass(),
        Package,
        FName(CannonAssetNames::CameraDroneAssetName),
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        CannonAssetNames::GeneratorCameraDroneReferenceName);
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the editor-only Camera Drone class-reference stub"));
        return nullptr;
    }

    UEdGraph* ActivationGraph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint,
        CannonAssetNames::SetDroneActiveFunctionName,
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph<UFunction>(
        Blueprint,
        ActivationGraph,
        true,
        nullptr);

    UK2Node_FunctionEntry* ActivationEntry = nullptr;
    for (UEdGraphNode* Node : ActivationGraph->Nodes)
    {
        if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node))
        {
            ActivationEntry = Entry;
            break;
        }
    }
    if (!ActivationEntry)
    {
        UE_LOG(LogTemp, Error, TEXT("Camera Drone reference stub activation function has no entry node"));
        return nullptr;
    }

    FEdGraphPinType BooleanPinType;
    BooleanPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    if (!ActivationEntry->CreateUserDefinedPin(
            CannonAssetNames::DroneActivePinName,
            BooleanPinType,
            EGPD_Output) ||
        !ActivationEntry->CreateUserDefinedPin(
            CannonAssetNames::DroneUpdateDeployPinName,
            BooleanPinType,
            EGPD_Output))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to author the stock Camera Drone activation signature"));
        return nullptr;
    }

    if (!SaveCompiledBlueprint(Package, Blueprint))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the editor-only Camera Drone class-reference stub"));
        return nullptr;
    }
    return Blueprint;
}

struct FObjFace
{
    FString ObjectName;
    FString MaterialName;
    TArray<int32> VertexIndices;
};

struct FObjSource
{
    TArray<FVector> Positions;
    TArray<FObjFace> Faces;
};

bool ParseObjSource(FObjSource& Source)
{
    const FString SourcePath = FPaths::Combine(
        FPaths::ProjectDir(),
        CannonAssetNames::VisualSourceRelativePath);
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *SourcePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to read Harpoon visual source: %s"), *SourcePath);
        return false;
    }

    FString CurrentObject;
    FString CurrentMaterial;
    TArray<FString> Lines;
    Text.ParseIntoArrayLines(Lines);
    for (FString& Line : Lines)
    {
        Line.TrimStartAndEndInline();
        if (Line.StartsWith(CannonAssetNames::ObjVertexPrefix))
        {
            TArray<FString> Tokens;
            Line.RightChop(FCString::Strlen(CannonAssetNames::ObjVertexPrefix))
                .ParseIntoArrayWS(Tokens);
            if (Tokens.Num() != 3)
            {
                UE_LOG(LogTemp, Error, TEXT("Malformed OBJ vertex: %s"), *Line);
                return false;
            }
            Source.Positions.Emplace(
                FCString::Atod(*Tokens[0]),
                FCString::Atod(*Tokens[1]),
                FCString::Atod(*Tokens[2]));
            continue;
        }
        if (Line.StartsWith(CannonAssetNames::ObjObjectPrefix))
        {
            CurrentObject = Line.RightChop(
                FCString::Strlen(CannonAssetNames::ObjObjectPrefix));
            continue;
        }
        if (Line.StartsWith(CannonAssetNames::ObjMaterialPrefix))
        {
            CurrentMaterial = Line.RightChop(
                FCString::Strlen(CannonAssetNames::ObjMaterialPrefix));
            continue;
        }
        if (!Line.StartsWith(CannonAssetNames::ObjFacePrefix))
        {
            continue;
        }

        TArray<FString> Tokens;
        Line.RightChop(FCString::Strlen(CannonAssetNames::ObjFacePrefix))
            .ParseIntoArrayWS(Tokens);
        if (CurrentObject.IsEmpty() || Tokens.Num() < 3)
        {
            UE_LOG(LogTemp, Error, TEXT("Malformed OBJ face or missing object: %s"), *Line);
            return false;
        }

        FObjFace& Face = Source.Faces.Emplace_GetRef();
        Face.ObjectName = CurrentObject;
        Face.MaterialName = CurrentMaterial;
        Face.VertexIndices.Reserve(Tokens.Num());
        for (const FString& Token : Tokens)
        {
            FString VertexToken;
            FString Remainder;
            if (!Token.Split(
                    CannonAssetNames::ObjIndexSeparator,
                    &VertexToken,
                    &Remainder,
                    ESearchCase::CaseSensitive))
            {
                VertexToken = Token;
            }
            int32 VertexIndex = FCString::Atoi(*VertexToken);
            VertexIndex = VertexIndex > 0
                ? VertexIndex - 1
                : Source.Positions.Num() + VertexIndex;
            if (!Source.Positions.IsValidIndex(VertexIndex))
            {
                UE_LOG(LogTemp, Error, TEXT("OBJ face has invalid vertex index: %s"), *Line);
                return false;
            }
            Face.VertexIndices.Add(VertexIndex);
        }
    }

    if (Source.Positions.IsEmpty() || Source.Faces.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Harpoon OBJ source contains no usable geometry"));
        return false;
    }
    return true;
}

bool IncludesObject(
    const FString& ObjectName,
    const TCHAR* const* IncludedObjects,
    const int32 IncludedObjectCount)
{
    for (int32 Index = 0; Index < IncludedObjectCount; ++Index)
    {
        if (ObjectName.Equals(IncludedObjects[Index], ESearchCase::CaseSensitive))
        {
            return true;
        }
    }
    return false;
}

UStaticMesh* CreateVisualMesh(
    const FObjSource& Source,
    const TCHAR* PackageName,
    const TCHAR* AssetName,
    const TCHAR* const* IncludedObjects,
    const int32 IncludedObjectCount,
    const FVector& Pivot,
    const bool bCreateFabricatorInteractionCollision)
{
    UPackage* Package = CreatePackage(PackageName);
    UStaticMesh* Mesh = NewObject<UStaticMesh>(
        Package,
        FName(AssetName),
        RF_Public | RF_Standalone);
    if (!Mesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to allocate Harpoon visual mesh %s"), AssetName);
        return nullptr;
    }

    FMeshDescription MeshDescription;
    FStaticMeshAttributes(MeshDescription).Register();
    FMeshDescriptionBuilder Builder;
    Builder.SetMeshDescription(&MeshDescription);
    Builder.SetNumUVLayers(1);
    TMap<int32, FVertexID> Vertices;
    TMap<FString, FPolygonGroupID> PolygonGroups;
    int32 TriangleCount = 0;
    for (const FObjFace& Face : Source.Faces)
    {
        if (!IncludesObject(Face.ObjectName, IncludedObjects, IncludedObjectCount))
        {
            continue;
        }

        const FString MaterialName = Face.MaterialName.IsEmpty()
            ? CannonAssetNames::VisualFallbackMaterialSlotName.ToString()
            : Face.MaterialName;
        FPolygonGroupID* PolygonGroup = PolygonGroups.Find(MaterialName);
        if (!PolygonGroup)
        {
            const FName MaterialSlotName(*MaterialName);
            const FPolygonGroupID NewPolygonGroup = Builder.AppendPolygonGroup(MaterialSlotName);
            PolygonGroups.Add(MaterialName, NewPolygonGroup);
            Mesh->GetStaticMaterials().Add(FStaticMaterial(
                UMaterial::GetDefaultMaterial(MD_Surface),
                MaterialSlotName));
            PolygonGroup = PolygonGroups.Find(MaterialName);
        }

        TArray<FVertexID> FaceVertices;
        FaceVertices.Reserve(Face.VertexIndices.Num());
        for (const int32 SourceVertexIndex : Face.VertexIndices)
        {
            FVertexID* Vertex = Vertices.Find(SourceVertexIndex);
            if (!Vertex)
            {
                const FVertexID NewVertex = Builder.AppendVertex(
                    Source.Positions[SourceVertexIndex] - Pivot);
                Vertices.Add(SourceVertexIndex, NewVertex);
                Vertex = Vertices.Find(SourceVertexIndex);
            }
            FaceVertices.Add(*Vertex);
        }

        for (int32 Index = 1; Index + 1 < FaceVertices.Num(); ++Index)
        {
            Builder.AppendTriangle(
                FaceVertices[0],
                FaceVertices[Index + 1],
                FaceVertices[Index],
                *PolygonGroup);
            ++TriangleCount;
        }
    }
    if (TriangleCount == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Harpoon visual mesh %s selected no OBJ faces"), AssetName);
        return nullptr;
    }
    Builder.SetAllEdgesHardness(true);

    UStaticMesh::FBuildMeshDescriptionsParams BuildParameters;
    BuildParameters.bBuildSimpleCollision = false;
    BuildParameters.bCommitMeshDescription = true;
    const TArray<const FMeshDescription*> MeshDescriptions = {&MeshDescription};
    if (!Mesh->BuildFromMeshDescriptions(MeshDescriptions, BuildParameters))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to build Harpoon visual mesh %s"), AssetName);
        return nullptr;
    }
    if (bCreateFabricatorInteractionCollision)
    {
        UBodySetup* BodySetup = Mesh->GetBodySetup();
        if (!BodySetup)
        {
            Mesh->CreateBodySetup();
            BodySetup = Mesh->GetBodySetup();
        }
        if (!BodySetup)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create Harpoon interaction collision for %s"), AssetName);
            return nullptr;
        }

        const FBoxSphereBounds MeshBounds = Mesh->GetBounds();
        FKBoxElem InteractionBox(
            MeshBounds.BoxExtent.X * 2.0,
            MeshBounds.BoxExtent.Y * 2.0,
            MeshBounds.BoxExtent.Z * 2.0);
        InteractionBox.Center = MeshBounds.Origin;
        BodySetup->RemoveSimpleCollision();
        BodySetup->AggGeom.BoxElems.Add(MoveTemp(InteractionBox));
        BodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
        BodySetup->InvalidatePhysicsData();
    }
    if (!SaveGeneratedAsset(Package, Mesh))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save Harpoon visual mesh %s"), AssetName);
        return nullptr;
    }
    return Mesh;
}

UStaticMesh* CreateLoadedConnectorReferenceStub()
{
    const FObjSource StubSource{
        {
            FVector(-0.5, -0.5, -0.5), FVector(0.5, -0.5, -0.5),
            FVector(0.5, 0.5, -0.5), FVector(-0.5, 0.5, -0.5),
            FVector(-0.5, -0.5, 0.5), FVector(0.5, -0.5, 0.5),
            FVector(0.5, 0.5, 0.5), FVector(-0.5, 0.5, 0.5)
        },
        {
            {CannonAssetNames::ReferenceStubObjectName, CannonAssetNames::ReferenceStubObjectName, {0, 3, 2, 1}},
            {CannonAssetNames::ReferenceStubObjectName, CannonAssetNames::ReferenceStubObjectName, {4, 5, 6, 7}},
            {CannonAssetNames::ReferenceStubObjectName, CannonAssetNames::ReferenceStubObjectName, {0, 1, 5, 4}},
            {CannonAssetNames::ReferenceStubObjectName, CannonAssetNames::ReferenceStubObjectName, {1, 2, 6, 5}},
            {CannonAssetNames::ReferenceStubObjectName, CannonAssetNames::ReferenceStubObjectName, {2, 3, 7, 6}},
            {CannonAssetNames::ReferenceStubObjectName, CannonAssetNames::ReferenceStubObjectName, {3, 0, 4, 7}}
        }
    };
    constexpr const TCHAR* StubObjects[] = {CannonAssetNames::ReferenceStubObjectName};
    return CreateVisualMesh(
        StubSource,
        CannonAssetNames::LoadedConnectorPackageName,
        CannonAssetNames::LoadedConnectorAssetName,
        StubObjects,
        UE_ARRAY_COUNT(StubObjects),
        FVector::ZeroVector,
        CannonAssetNames::MovingMeshBuildsFabricatorInteractionCollision);
}

USCS_Node* AddRootNode(
    USimpleConstructionScript* ConstructionScript,
    UClass* ComponentClass,
    const FName ComponentName)
{
    USCS_Node* Node = ConstructionScript->CreateNode(ComponentClass, ComponentName);
    ConstructionScript->AddNode(Node);
    return Node;
}

USCS_Node* AddChildNode(
    USimpleConstructionScript* ConstructionScript,
    USCS_Node* Parent,
    UClass* ComponentClass,
    const FName ComponentName)
{
    USCS_Node* Node = ConstructionScript->CreateNode(ComponentClass, ComponentName);
    Parent->AddChildNode(Node);
    return Node;
}

void ConfigureBodyMesh(
    USCS_Node* Node,
    UStaticMesh* Mesh,
    const FVector& RelativeLocation,
    const FVector& RelativeScale)
{
    UStaticMeshComponent* Template = CastChecked<UStaticMeshComponent>(Node->ComponentTemplate);
    Template->SetStaticMesh(Mesh);
    Template->SetRelativeLocation(RelativeLocation);
    Template->SetRelativeScale3D(RelativeScale);
    Template->SetCollisionProfileName(CannonAssetNames::BlockAllDynamicCollisionProfileName);
    Template->SetGenerateOverlapEvents(false);
    Template->SetSimulatePhysics(false);
}

void ConfigureDecorativeMesh(
    USCS_Node* Node,
    UStaticMesh* Mesh,
    const FVector& RelativeLocation,
    const FRotator& RelativeRotation,
    const FVector& RelativeScale)
{
    UStaticMeshComponent* Template = CastChecked<UStaticMeshComponent>(Node->ComponentTemplate);
    Template->SetStaticMesh(Mesh);
    Template->SetRelativeLocation(RelativeLocation);
    Template->SetRelativeRotation(RelativeRotation);
    Template->SetRelativeScale3D(RelativeScale);
    Template->SetCollisionProfileName(CannonAssetNames::NoCollisionProfileName);
    Template->SetGenerateOverlapEvents(false);
    Template->SetSimulatePhysics(false);
}

UWidgetBlueprint* CreateOpticalHud()
{
    UPackage* Package = CreatePackage(CannonAssetNames::HudPackageName);
    UWidgetBlueprint* Blueprint = CastChecked<UWidgetBlueprint>(
        FKismetEditorUtilities::CreateBlueprint(
            UVoyageBaseUserWidget::StaticClass(),
            Package,
            FName(CannonAssetNames::HudAssetName),
            BPTYPE_Normal,
            UWidgetBlueprint::StaticClass(),
            UWidgetBlueprintGeneratedClass::StaticClass(),
            CannonAssetNames::GeneratorWidgetName));

    UCanvasPanel* Root = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(),
        CannonAssetNames::HudRootName);
    Blueprint->WidgetTree->RootWidget = Root;

    UTextBlock* Reticle = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(),
        CannonAssetNames::ReticleName);
    Reticle->SetText(FText::FromString(CannonAssetNames::ReticleText));
    UCanvasPanelSlot* ReticleSlot = Root->AddChildToCanvas(Reticle);
    ReticleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
    ReticleSlot->SetAlignment(FVector2D(0.5, 0.5));
    ReticleSlot->SetAutoSize(true);

    UTextBlock* Distance = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(),
        CannonAssetNames::DistanceTextName);
    Distance->SetText(FText::FromString(CannonAssetNames::DistancePlaceholderText));
    Distance->SetVisibility(ESlateVisibility::Collapsed);
    UCanvasPanelSlot* DistanceSlot = Root->AddChildToCanvas(Distance);
    DistanceSlot->SetAnchors(FAnchors(0.5f, 0.5f));
    DistanceSlot->SetAlignment(FVector2D(0.5, 0.0));
    DistanceSlot->SetPosition(FVector2D(0.0, 48.0));
    DistanceSlot->SetAutoSize(true);

    if (!SaveCompiledBlueprint(Package, Blueprint))
    {
        return nullptr;
    }
    return Blueprint;
}

bool AddCannonOperatorInterface(UBlueprint* Blueprint)
{
    if (!FBlueprintEditorUtils::ImplementNewInterface(
            Blueprint,
            UInteractiveInterface::StaticClass()->GetClassPathName()))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to add the stock interactive interface to the cannon operator"));
        return false;
    }
    return true;
}

bool ConfigureCannonOperatorGraph(UBlueprint* Blueprint)
{
    if (!AddCannonOperatorInterface(Blueprint))
    {
        return false;
    }

    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!Graph)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannon operator event graph is missing"));
        return false;
    }

    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    UFunction* InteractFunction = UInteractiveInterface::StaticClass()->FindFunctionByName(
        CannonAssetNames::InteractActionFunctionName);
    check(InteractFunction);

    UK2Node_Event* Interact = NewObject<UK2Node_Event>(Graph);
    Interact->EventReference.SetExternalMember(
        CannonAssetNames::InteractActionFunctionName,
        UInteractiveInterface::StaticClass());
    Interact->bOverrideFunction = true;
    FinishGraphNode(Interact, Graph, 0, 0);

    UK2Node_CallFunction* IsCharacterValid = AddCall(
        Graph,
        UKismetSystemLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid)),
        260,
        0);

    UK2Node_IfThenElse* CharacterValidBranch = NewObject<UK2Node_IfThenElse>(Graph);
    FinishGraphNode(CharacterValidBranch, Graph, 500, 0);

    UK2Node_CallFunction* PartMatches = AddCall(
        Graph,
        UKismetMathLibrary::StaticClass()->FindFunctionByName(
            BlueprintGraphNames::MathFunctions::EqualInt),
        260,
        100);
    SetDefault(
        PartMatches,
        BinaryPins::RightOperand,
        CannonAssetNames::DefaultInteractivePartId);

    UK2Node_CallFunction* IndexMatches = AddCall(
        Graph,
        UKismetMathLibrary::StaticClass()->FindFunctionByName(
            BlueprintGraphNames::MathFunctions::EqualInt),
        260,
        260);
    SetDefault(
        IndexMatches,
        BinaryPins::RightOperand,
        CannonAssetNames::DefaultInteractiveIndex);

    UK2Node_CallFunction* InteractionMatches = AddCall(
        Graph,
        UKismetMathLibrary::StaticClass()->FindFunctionByName(
            BlueprintGraphNames::MathFunctions::BooleanAnd),
        500,
        180);

    UK2Node_IfThenElse* InteractionBranch = NewObject<UK2Node_IfThenElse>(Graph);
    FinishGraphNode(InteractionBranch, Graph, 740, 0);

    UK2Node_CallFunction* GetController = AddCall(
        Graph,
        APawn::StaticClass()->FindFunctionByName(
            BlueprintGraphNames::ActorFunctions::GetController),
        740,
        280);

    UK2Node_DynamicCast* CastPlayerController = NewObject<UK2Node_DynamicCast>(Graph);
    CastPlayerController->TargetType = AVoyagePlayerController::StaticClass();
    FinishGraphNode(CastPlayerController, Graph, 980, 0);
    CastPlayerController->SetPurity(false);

    UK2Node_CallFunction* EnterVehicle = AddCall(
        Graph,
        AVoyageVehiclePawn::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(AVoyageVehiclePawn, OnEnterVehicle)),
        1240,
        0);

    UK2Node_InputKey* ExitKey = NewObject<UK2Node_InputKey>(Graph);
    ExitKey->InputKey = FKey(FName(CannonAssetNames::DirectExitMarkerKeyName));
    ExitKey->bConsumeInput = true;
    ExitKey->bExecuteWhenPaused = false;
    ExitKey->bOverrideParentBinding = false;
    FinishGraphNode(ExitKey, Graph, 0, 560);

    UK2Node_CallFunction* ExitVehicle = AddCall(
        Graph,
        AVoyageVehiclePawn::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(AVoyageVehiclePawn, OnExitVehicle)),
        300,
        560);

    UK2Node_Event* Destroyed = NewObject<UK2Node_Event>(Graph);
    Destroyed->EventReference.SetExternalMember(
        BlueprintGraphNames::Events::ActorReceiveDestroyed,
        AActor::StaticClass());
    Destroyed->bOverrideFunction = true;
    FinishGraphNode(Destroyed, Graph, 0, 760);

    UK2Node_CallFunction* ExitBeforeDestroy = AddCall(
        Graph,
        AVoyageVehiclePawn::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(AVoyageVehiclePawn, OnExitVehicle)),
        300,
        760);

    bool bConnected = true;
    bConnected &= Connect(
        Schema,
        RequirePin(Interact, CannonAssetNames::EventCharacterPinName),
        RequirePin(IsCharacterValid, GraphPins::Object));
    bConnected &= Connect(
        Schema,
        RequirePin(Interact, GraphPins::Then),
        RequirePin(CharacterValidBranch, GraphPins::Execute));
    bConnected &= Connect(
        Schema,
        RequirePin(IsCharacterValid, GraphPins::ReturnValue),
        RequirePin(CharacterValidBranch, GraphPins::Condition));
    bConnected &= Connect(
        Schema,
        RequirePin(CharacterValidBranch, GraphPins::Then),
        RequirePin(InteractionBranch, GraphPins::Execute));
    bConnected &= Connect(
        Schema,
        RequirePin(Interact, CannonAssetNames::EventPartIdPinName),
        RequirePin(PartMatches, BinaryPins::LeftOperand));
    bConnected &= Connect(
        Schema,
        RequirePin(Interact, CannonAssetNames::EventInteractIndexPinName),
        RequirePin(IndexMatches, BinaryPins::LeftOperand));
    bConnected &= Connect(
        Schema,
        RequirePin(PartMatches, GraphPins::ReturnValue),
        RequirePin(InteractionMatches, BinaryPins::LeftOperand));
    bConnected &= Connect(
        Schema,
        RequirePin(IndexMatches, GraphPins::ReturnValue),
        RequirePin(InteractionMatches, BinaryPins::RightOperand));
    bConnected &= Connect(
        Schema,
        RequirePin(InteractionMatches, GraphPins::ReturnValue),
        RequirePin(InteractionBranch, GraphPins::Condition));
    bConnected &= Connect(
        Schema,
        RequirePin(Interact, CannonAssetNames::EventCharacterPinName),
        RequirePin(GetController, GraphPins::FunctionTarget));
    bConnected &= Connect(
        Schema,
        RequirePin(InteractionBranch, GraphPins::Then),
        CastPlayerController->GetExecPin());
    bConnected &= Connect(
        Schema,
        RequirePin(GetController, GraphPins::ReturnValue),
        CastPlayerController->GetCastSourcePin());
    bConnected &= Connect(
        Schema,
        CastPlayerController->GetValidCastPin(),
        RequirePin(EnterVehicle, GraphPins::Execute));
    bConnected &= Connect(
        Schema,
        CastPlayerController->GetCastResultPin(),
        RequirePin(EnterVehicle, CannonAssetNames::PlayerControllerPinName));
    bConnected &= Connect(
        Schema,
        ExitKey->GetPressedPin(),
        RequirePin(ExitVehicle, GraphPins::Execute));
    bConnected &= Connect(
        Schema,
        RequirePin(Destroyed, GraphPins::Then),
        RequirePin(ExitBeforeDestroy, GraphPins::Execute));
    return bConnected;
}

bool AddOperatorActorVariable(UBlueprint* Blueprint)
{
    FEdGraphPinType OperatorType;
    OperatorType.PinCategory = UEdGraphSchema_K2::PC_Object;
    OperatorType.PinSubCategoryObject = AActor::StaticClass();
    if (!FBlueprintEditorUtils::AddMemberVariable(
            Blueprint,
            CannonAssetNames::OperatorActorVariableName,
            OperatorType))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to add the shell operator reference"));
        return false;
    }
    return true;
}

bool ConfigureShellOperatorLifecycle(
    UBlueprint* Blueprint,
    UBlueprint* OperatorBlueprint,
    UBlueprint* CameraDroneReferenceBlueprint)
{
    if (!AddOperatorActorVariable(Blueprint))
    {
        return false;
    }

    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!Graph || !OperatorBlueprint || !OperatorBlueprint->GeneratedClass ||
        (CannonAssetNames::UsesStockCameraDroneClass &&
            (!CameraDroneReferenceBlueprint || !CameraDroneReferenceBlueprint->GeneratedClass)))
    {
        UE_LOG(LogTemp, Error, TEXT("Shell operator lifecycle inputs are incomplete"));
        return false;
    }

    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    UK2Node_Event* Tick = NewObject<UK2Node_Event>(Graph);
    Tick->EventReference.SetExternalMember(
        BlueprintGraphNames::Events::ActorReceiveTick,
        AActor::StaticClass());
    Tick->bOverrideFunction = true;
    FinishGraphNode(Tick, Graph, 0, 0);

    UK2Node_CallFunction* HasFabricatedTag = AddCall(
        Graph,
        AActor::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(AActor, ActorHasTag)),
        260,
        160);
    SetDefault(
        HasFabricatedTag,
        GraphPins::Tag,
        *CannonAssetNames::FabricatedActorTag.ToString());

    UK2Node_IfThenElse* FabricatedBranch = NewObject<UK2Node_IfThenElse>(Graph);
    FinishGraphNode(FabricatedBranch, Graph, 500, 0);

    UK2Node_CallFunction* GetShellTransform = AddCall(
        Graph,
        AActor::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(AActor, GetTransform)),
        500,
        180);

    UK2Node_SpawnActorFromClass* SpawnOperator = NewObject<UK2Node_SpawnActorFromClass>(Graph);
    FinishSpawnActorNode(SpawnOperator, Graph, 760, 0);
    UClass* SpawnedOperatorClass = OperatorBlueprint->GeneratedClass.Get();
    if (CannonAssetNames::OperatorLifecycleProbe ==
        CannonAssetNames::EOperatorLifecycleProbe::NativeActor)
    {
        SpawnedOperatorClass = AActor::StaticClass();
    }
    else if (CannonAssetNames::UsesStockCameraDroneClass)
    {
        SpawnedOperatorClass = CameraDroneReferenceBlueprint->GeneratedClass.Get();
    }
    SpawnOperator->GetClassPin()->DefaultObject = SpawnedOperatorClass;
    SpawnOperator->PinDefaultValueChanged(SpawnOperator->GetClassPin());
    SetDefault(
        SpawnOperator,
        CannonAssetNames::CollisionHandlingOverridePinName,
        CannonAssetNames::AlwaysSpawnCollisionHandling);

    UK2Node_CallFunction* ActivateCameraDrone = nullptr;
    if (CannonAssetNames::ActivatesStockCameraDrone)
    {
        UFunction* ActivationFunction =
            CameraDroneReferenceBlueprint->GeneratedClass->FindFunctionByName(
                CannonAssetNames::SetDroneActiveFunctionName);
        if (!ActivationFunction)
        {
            UE_LOG(LogTemp, Error, TEXT("Camera Drone reference stub activation function is missing"));
            return false;
        }
        ActivateCameraDrone = AddCall(Graph, ActivationFunction, 1020, -140);
        SetDefault(
            ActivateCameraDrone,
            CannonAssetNames::DroneActivePinName,
            CannonAssetNames::BooleanTrueDefault);
        SetDefault(
            ActivateCameraDrone,
            CannonAssetNames::DroneUpdateDeployPinName,
            CannonAssetNames::BooleanTrueDefault);
    }

    UK2Node_Self* Self = NewObject<UK2Node_Self>(Graph);
    FinishGraphNode(Self, Graph, 760, 260);

    UK2Node_VariableSet* StoreOperator = AddVariableSet(
        Graph,
        CannonAssetNames::OperatorActorVariableName,
        1040,
        0);

    UK2Node_CallFunction* AttachOperator = AddCall(
        Graph,
        AActor::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(AActor, K2_AttachToActor)),
        1320,
        0);
    SetDefault(
        AttachOperator,
        CannonAssetNames::LocationRulePinName,
        CannonAssetNames::SnapToTargetAttachmentRule);
    SetDefault(
        AttachOperator,
        CannonAssetNames::RotationRulePinName,
        CannonAssetNames::SnapToTargetAttachmentRule);
    SetDefault(
        AttachOperator,
        CannonAssetNames::ScaleRulePinName,
        CannonAssetNames::SnapToTargetAttachmentRule);
    SetDefault(
        AttachOperator,
        CannonAssetNames::WeldSimulatedBodiesPinName,
        CannonAssetNames::BooleanFalseDefault);

    UK2Node_CallFunction* OffsetCameraDrone = nullptr;
    if (CannonAssetNames::OffsetsStockCameraDrone)
    {
        OffsetCameraDrone = AddCall(
            Graph,
            AActor::StaticClass()->FindFunctionByName(
                GET_FUNCTION_NAME_CHECKED(AActor, K2_SetActorRelativeLocation)),
            1580,
            -140);
        SetDefault(
            OffsetCameraDrone,
            CannonAssetNames::NewRelativeLocationPinName,
            CannonAssetNames::StockCameraDroneProbeRelativeLocationDefault);
        SetDefault(
            OffsetCameraDrone,
            CannonAssetNames::SweepPinName,
            CannonAssetNames::BooleanFalseDefault);
        SetDefault(
            OffsetCameraDrone,
            CannonAssetNames::TeleportPinName,
            CannonAssetNames::BooleanTrueDefault);
    }

    UK2Node_CallFunction* IsSpawnedOperatorValid = nullptr;
    UK2Node_IfThenElse* SpawnValidityBranch = nullptr;
    UK2Node_VariableGet* BarrelForSpawnMarker = nullptr;
    UK2Node_CallFunction* HideBarrelForSpawnMarker = nullptr;
    if (CannonAssetNames::MarksStockCameraDroneSpawnValidity)
    {
        IsSpawnedOperatorValid = AddCall(
            Graph,
            UKismetSystemLibrary::StaticClass()->FindFunctionByName(
                GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid)),
            1000,
            -360);
        SpawnValidityBranch = NewObject<UK2Node_IfThenElse>(Graph);
        FinishGraphNode(SpawnValidityBranch, Graph, 1240, -360);
        BarrelForSpawnMarker = AddVariableGet(
            Graph,
            CannonAssetNames::BarrelBodyName,
            1240,
            -180);
        HideBarrelForSpawnMarker = AddCall(
            Graph,
            USceneComponent::StaticClass()->FindFunctionByName(
                CannonAssetNames::SetVisibilityFunctionName),
            1480,
            -360);
        SetDefault(
            HideBarrelForSpawnMarker,
            CannonAssetNames::NewVisibilityPinName,
            CannonAssetNames::BooleanFalseDefault);
        SetDefault(
            HideBarrelForSpawnMarker,
            CannonAssetNames::PropagateToChildrenPinName,
            CannonAssetNames::BooleanTrueDefault);
    }

    UK2Node_CallFunction* DisableLifecycleTick = AddCall(
        Graph,
        AActor::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(AActor, SetActorTickEnabled)),
        1600,
        0);
    SetDefault(
        DisableLifecycleTick,
        CannonAssetNames::TickEnabledPinName,
        CannonAssetNames::BooleanFalseDefault);

    UK2Node_CallFunction* DelayedValidityWait = nullptr;
    UK2Node_VariableGet* OperatorAfterDelay = nullptr;
    UK2Node_CallFunction* IsOperatorValidAfterDelay = nullptr;
    UK2Node_IfThenElse* DelayedValidityBranch = nullptr;
    UK2Node_VariableGet* BarrelAfterDelay = nullptr;
    UK2Node_CallFunction* RestoreBarrelAfterDelay = nullptr;
    if (CannonAssetNames::ChecksStockCameraDroneDelayedValidity)
    {
        DelayedValidityWait = AddCall(
            Graph,
            UKismetSystemLibrary::StaticClass()->FindFunctionByName(
                GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Delay)),
            1840,
            0);
        SetDefault(
            DelayedValidityWait,
            CannonAssetNames::DelayDurationPinName,
            CannonAssetNames::StockCameraDroneDelayedValiditySecondsDefault);
        OperatorAfterDelay = AddVariableGet(
            Graph,
            CannonAssetNames::OperatorActorVariableName,
            1840,
            180);
        IsOperatorValidAfterDelay = AddCall(
            Graph,
            UKismetSystemLibrary::StaticClass()->FindFunctionByName(
                GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid)),
            2080,
            180);
        DelayedValidityBranch = NewObject<UK2Node_IfThenElse>(Graph);
        FinishGraphNode(DelayedValidityBranch, Graph, 2320, 0);
        BarrelAfterDelay = AddVariableGet(
            Graph,
            CannonAssetNames::BarrelBodyName,
            2320,
            180);
        RestoreBarrelAfterDelay = AddCall(
            Graph,
            USceneComponent::StaticClass()->FindFunctionByName(
                CannonAssetNames::SetVisibilityFunctionName),
            2560,
            0);
        SetDefault(
            RestoreBarrelAfterDelay,
            CannonAssetNames::NewVisibilityPinName,
            CannonAssetNames::BooleanTrueDefault);
        SetDefault(
            RestoreBarrelAfterDelay,
            CannonAssetNames::PropagateToChildrenPinName,
            CannonAssetNames::BooleanTrueDefault);
    }

    UK2Node_Event* Destroyed = NewObject<UK2Node_Event>(Graph);
    Destroyed->EventReference.SetExternalMember(
        BlueprintGraphNames::Events::ActorReceiveDestroyed,
        AActor::StaticClass());
    Destroyed->bOverrideFunction = true;
    FinishGraphNode(Destroyed, Graph, 0, 520);

    UK2Node_VariableGet* OperatorForValidity = AddVariableGet(
        Graph,
        CannonAssetNames::OperatorActorVariableName,
        260,
        680);
    UK2Node_CallFunction* IsOperatorValid = AddCall(
        Graph,
        UKismetSystemLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid)),
        500,
        680);
    UK2Node_IfThenElse* DestroyBranch = NewObject<UK2Node_IfThenElse>(Graph);
    FinishGraphNode(DestroyBranch, Graph, 760, 520);
    UK2Node_CallFunction* DestroyOperator = AddCall(
        Graph,
        AActor::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(AActor, K2_DestroyActor)),
        1040,
        520);

    bool bConnected = true;
    bConnected &= Connect(
        Schema,
        RequirePin(Tick, GraphPins::Then),
        RequirePin(FabricatedBranch, GraphPins::Execute));
    bConnected &= Connect(
        Schema,
        RequirePin(HasFabricatedTag, GraphPins::ReturnValue),
        RequirePin(FabricatedBranch, GraphPins::Condition));
    bConnected &= Connect(
        Schema,
        RequirePin(FabricatedBranch, GraphPins::Then),
        RequirePin(SpawnOperator, GraphPins::Execute));
    bConnected &= Connect(
        Schema,
        RequirePin(GetShellTransform, GraphPins::ReturnValue),
        RequirePin(SpawnOperator, GraphPins::SpawnTransform));
    bConnected &= Connect(
        Schema,
        RequirePin(Self, GraphPins::FunctionTarget),
        RequirePin(SpawnOperator, GraphPins::Owner));
    if (SpawnValidityBranch)
    {
        bConnected &= Connect(
            Schema,
            RequirePin(SpawnOperator, GraphPins::Then),
            RequirePin(SpawnValidityBranch, GraphPins::Execute));
        bConnected &= Connect(
            Schema,
            SpawnOperator->GetResultPin(),
            RequirePin(IsSpawnedOperatorValid, GraphPins::Object));
        bConnected &= Connect(
            Schema,
            RequirePin(IsSpawnedOperatorValid, GraphPins::ReturnValue),
            RequirePin(SpawnValidityBranch, GraphPins::Condition));
        bConnected &= Connect(
            Schema,
            RequirePin(SpawnValidityBranch, GraphPins::Then),
            RequirePin(HideBarrelForSpawnMarker, GraphPins::Execute));
        bConnected &= Connect(
            Schema,
            RequirePin(BarrelForSpawnMarker, CannonAssetNames::BarrelBodyName),
            RequirePin(HideBarrelForSpawnMarker, GraphPins::FunctionTarget));
        bConnected &= Connect(
            Schema,
            RequirePin(HideBarrelForSpawnMarker, GraphPins::Then),
            RequirePin(ActivateCameraDrone, GraphPins::Execute));
        bConnected &= Connect(
            Schema,
            SpawnOperator->GetResultPin(),
            RequirePin(ActivateCameraDrone, GraphPins::FunctionTarget));
        bConnected &= Connect(
            Schema,
            RequirePin(ActivateCameraDrone, GraphPins::Then),
            RequirePin(StoreOperator, GraphPins::Execute));
        bConnected &= Connect(
            Schema,
            RequirePin(SpawnValidityBranch, GraphPins::Else),
            RequirePin(DisableLifecycleTick, GraphPins::Execute));
    }
    else if (ActivateCameraDrone)
    {
        bConnected &= Connect(
            Schema,
            RequirePin(SpawnOperator, GraphPins::Then),
            RequirePin(ActivateCameraDrone, GraphPins::Execute));
        bConnected &= Connect(
            Schema,
            SpawnOperator->GetResultPin(),
            RequirePin(ActivateCameraDrone, GraphPins::FunctionTarget));
        bConnected &= Connect(
            Schema,
            RequirePin(ActivateCameraDrone, GraphPins::Then),
            RequirePin(StoreOperator, GraphPins::Execute));
    }
    else
    {
        bConnected &= Connect(
            Schema,
            RequirePin(SpawnOperator, GraphPins::Then),
            RequirePin(StoreOperator, GraphPins::Execute));
    }
    bConnected &= Connect(
        Schema,
        SpawnOperator->GetResultPin(),
        RequirePin(StoreOperator, CannonAssetNames::OperatorActorVariableName));
    bConnected &= Connect(
        Schema,
        RequirePin(StoreOperator, GraphPins::Then),
        RequirePin(AttachOperator, GraphPins::Execute));
    bConnected &= Connect(
        Schema,
        SpawnOperator->GetResultPin(),
        RequirePin(AttachOperator, GraphPins::FunctionTarget));
    bConnected &= Connect(
        Schema,
        RequirePin(Self, GraphPins::FunctionTarget),
        RequirePin(AttachOperator, GraphPins::ParentActor));
    if (OffsetCameraDrone)
    {
        bConnected &= Connect(
            Schema,
            RequirePin(AttachOperator, GraphPins::Then),
            RequirePin(OffsetCameraDrone, GraphPins::Execute));
        bConnected &= Connect(
            Schema,
            SpawnOperator->GetResultPin(),
            RequirePin(OffsetCameraDrone, GraphPins::FunctionTarget));
        bConnected &= Connect(
            Schema,
            RequirePin(OffsetCameraDrone, GraphPins::Then),
            RequirePin(DisableLifecycleTick, GraphPins::Execute));
    }
    if (DelayedValidityWait)
    {
        bConnected &= Connect(
            Schema,
            RequirePin(DisableLifecycleTick, GraphPins::Then),
            RequirePin(DelayedValidityWait, GraphPins::Execute));
        bConnected &= Connect(
            Schema,
            RequirePin(DelayedValidityWait, GraphPins::Then),
            RequirePin(DelayedValidityBranch, GraphPins::Execute));
        bConnected &= Connect(
            Schema,
            RequirePin(OperatorAfterDelay, CannonAssetNames::OperatorActorVariableName),
            RequirePin(IsOperatorValidAfterDelay, GraphPins::Object));
        bConnected &= Connect(
            Schema,
            RequirePin(IsOperatorValidAfterDelay, GraphPins::ReturnValue),
            RequirePin(DelayedValidityBranch, GraphPins::Condition));
        bConnected &= Connect(
            Schema,
            RequirePin(DelayedValidityBranch, GraphPins::Then),
            RequirePin(RestoreBarrelAfterDelay, GraphPins::Execute));
        bConnected &= Connect(
            Schema,
            RequirePin(BarrelAfterDelay, CannonAssetNames::BarrelBodyName),
            RequirePin(RestoreBarrelAfterDelay, GraphPins::FunctionTarget));
    }
    else
    {
        bConnected &= Connect(
            Schema,
            RequirePin(AttachOperator, GraphPins::Then),
            RequirePin(DisableLifecycleTick, GraphPins::Execute));
    }
    bConnected &= Connect(
        Schema,
        RequirePin(Destroyed, GraphPins::Then),
        RequirePin(DestroyBranch, GraphPins::Execute));
    bConnected &= Connect(
        Schema,
        RequirePin(OperatorForValidity, CannonAssetNames::OperatorActorVariableName),
        RequirePin(IsOperatorValid, GraphPins::Object));
    bConnected &= Connect(
        Schema,
        RequirePin(IsOperatorValid, GraphPins::ReturnValue),
        RequirePin(DestroyBranch, GraphPins::Condition));
    bConnected &= Connect(
        Schema,
        RequirePin(DestroyBranch, GraphPins::Then),
        RequirePin(DestroyOperator, GraphPins::Execute));
    bConnected &= Connect(
        Schema,
        RequirePin(OperatorForValidity, CannonAssetNames::OperatorActorVariableName),
        RequirePin(DestroyOperator, GraphPins::FunctionTarget));
    return bConnected;
}

UBlueprint* CreateCannonPawn()
{
    UPackage* Package = CreatePackage(CannonAssetNames::PawnPackageName);
    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
        AVoyageVehiclePawn::StaticClass(),
        Package,
        FName(CannonAssetNames::PawnAssetName),
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        CannonAssetNames::GeneratorBlueprintName);
    if (!Blueprint || !Blueprint->SimpleConstructionScript)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the cannon pawn Blueprint"));
        return nullptr;
    }

    if (CannonAssetNames::OperatorLifecycleProbe ==
        CannonAssetNames::EOperatorLifecycleProbe::MinimalVehicleBlueprint)
    {
        if (!SaveCompiledBlueprint(Package, Blueprint))
        {
            return nullptr;
        }
        return Blueprint;
    }

    USimpleConstructionScript* ConstructionScript = Blueprint->SimpleConstructionScript;
    USCS_Node* MountRoot = AddRootNode(
        ConstructionScript,
        USceneComponent::StaticClass(),
        CannonAssetNames::MountRootName);
    USCS_Node* YawPivot = AddChildNode(
        ConstructionScript,
        MountRoot,
        USceneComponent::StaticClass(),
        CannonAssetNames::YawPivotName);
    USCS_Node* PitchPivot = AddChildNode(
        ConstructionScript,
        YawPivot,
        USceneComponent::StaticClass(),
        CannonAssetNames::PitchPivotName);
    USCS_Node* OpticalAxis = AddChildNode(
        ConstructionScript,
        PitchPivot,
        USceneComponent::StaticClass(),
        CannonAssetNames::OpticalAxisName);

    CastChecked<USceneComponent>(PitchPivot->ComponentTemplate)->SetRelativeLocation(
        CannonAssetNames::PitchPivotRelativeLocation);
    CastChecked<USceneComponent>(OpticalAxis->ComponentTemplate)->SetRelativeLocation(
        CannonAssetNames::OpticalAxisRelativeLocation);

    if (CannonAssetNames::OperatorLifecycleProbe ==
        CannonAssetNames::EOperatorLifecycleProbe::PassiveTransformBlueprint)
    {
        if (!SaveCompiledBlueprint(Package, Blueprint))
        {
            return nullptr;
        }
        return Blueprint;
    }

    USCS_Node* InteractiveObject = AddChildNode(
        ConstructionScript,
        MountRoot,
        UInteractiveObjectComponent::StaticClass(),
        CannonAssetNames::OperatorInteractionComponentName);
    CastChecked<USceneComponent>(InteractiveObject->ComponentTemplate)->SetRelativeLocation(
        CannonAssetNames::OperatorInteractionRelativeLocation);

    if (CannonAssetNames::OperatorLifecycleProbe ==
        CannonAssetNames::EOperatorLifecycleProbe::InteractiveComponentBlueprint)
    {
        if (!SaveCompiledBlueprint(Package, Blueprint))
        {
            return nullptr;
        }
        return Blueprint;
    }

    USCS_Node* Collision = AddChildNode(
        ConstructionScript,
        MountRoot,
        UBoxComponent::StaticClass(),
        CannonAssetNames::OperatorInteractionBoxName);
    UBoxComponent* CollisionTemplate = CastChecked<UBoxComponent>(Collision->ComponentTemplate);
    CollisionTemplate->SetBoxExtent(CannonAssetNames::OperatorInteractionBoxExtent);
    CollisionTemplate->SetRelativeLocation(CannonAssetNames::OperatorInteractionRelativeLocation);
    CollisionTemplate->SetCollisionProfileName(CannonAssetNames::InteractiveCollisionProfileName);
    CollisionTemplate->SetGenerateOverlapEvents(false);

    if (CannonAssetNames::OperatorLifecycleProbe ==
        CannonAssetNames::EOperatorLifecycleProbe::InteractionBoxBlueprint)
    {
        if (!SaveCompiledBlueprint(Package, Blueprint))
        {
            return nullptr;
        }
        return Blueprint;
    }

    if (CannonAssetNames::OperatorLifecycleProbe ==
            CannonAssetNames::EOperatorLifecycleProbe::InteractionInterfaceBlueprint ||
        CannonAssetNames::UsesStockCameraDroneClass)
    {
        if (!AddCannonOperatorInterface(Blueprint) ||
            !SaveCompiledBlueprint(Package, Blueprint))
        {
            return nullptr;
        }
        return Blueprint;
    }

    USCS_Node* CharacterLocation = AddChildNode(
        ConstructionScript,
        MountRoot,
        USceneComponent::StaticClass(),
        CannonAssetNames::OperatorCharacterLocationName);
    USCS_Node* OpticalCamera = AddChildNode(
        ConstructionScript,
        OpticalAxis,
        UCameraComponent::StaticClass(),
        CannonAssetNames::OpticalCameraName);

    USceneComponent* CharacterLocationTemplate =
        CastChecked<USceneComponent>(CharacterLocation->ComponentTemplate);
    CharacterLocationTemplate->SetRelativeLocation(
        CannonAssetNames::OperatorCharacterRelativeLocation);
    CharacterLocationTemplate->ComponentTags.Add(CannonAssetNames::ExitComponentTag);

    UCameraComponent* CameraTemplate = CastChecked<UCameraComponent>(OpticalCamera->ComponentTemplate);
    CameraTemplate->FieldOfView = CannonAssetNames::OpticalHorizontalFovDegrees;
    CameraTemplate->bUsePawnControlRotation = false;
    CameraTemplate->bAutoActivate = false;

    if (!ConfigureCannonOperatorGraph(Blueprint))
    {
        return nullptr;
    }

    if (!SaveCompiledBlueprint(Package, Blueprint))
    {
        return nullptr;
    }
    return Blueprint;
}

UBlueprint* CreateLeafPlacementProbe(
    UStaticMesh* BaseMesh,
    UStaticMesh* YawMesh,
    UStaticMesh* PitchMesh,
    UStaticMesh* LoadedConnectorMesh,
    UVoyageItem* LeafItem,
    UBlueprint* OperatorBlueprint,
    UBlueprint* CameraDroneReferenceBlueprint)
{
    UPackage* Package = CreatePackage(CannonAssetNames::LeafProbePackageName);
    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
        AVoyageModuleActor::StaticClass(),
        Package,
        FName(CannonAssetNames::LeafProbeAssetName),
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        CannonAssetNames::GeneratorLeafProbeName);
    if (!Blueprint || !Blueprint->SimpleConstructionScript)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the leaf placement probe Blueprint"));
        return nullptr;
    }

    USimpleConstructionScript* ConstructionScript = Blueprint->SimpleConstructionScript;
    USCS_Node* ModuleMountRoot = AddRootNode(
        ConstructionScript,
        UBoxComponent::StaticClass(),
        CannonAssetNames::ModuleMountRootName);
    USCS_Node* DynamicCollision = AddRootNode(
        ConstructionScript,
        UVoyageDynamicCollisionComponent::StaticClass(),
        CannonAssetNames::DynamicCollisionName);
    USCS_Node* BaseBody = AddChildNode(
        ConstructionScript,
        ModuleMountRoot,
        UStaticMeshComponent::StaticClass(),
        CannonAssetNames::BaseBodyName);
    USCS_Node* YawPivot = AddChildNode(
        ConstructionScript,
        ModuleMountRoot,
        USceneComponent::StaticClass(),
        CannonAssetNames::YawPivotName);
    USCS_Node* TurretBody = AddChildNode(
        ConstructionScript,
        YawPivot,
        UStaticMeshComponent::StaticClass(),
        CannonAssetNames::TurretBodyName);
    USCS_Node* PitchPivot = AddChildNode(
        ConstructionScript,
        YawPivot,
        USceneComponent::StaticClass(),
        CannonAssetNames::PitchPivotName);
    USCS_Node* BarrelBody = AddChildNode(
        ConstructionScript,
        PitchPivot,
        UStaticMeshComponent::StaticClass(),
        CannonAssetNames::BarrelBodyName);
    USCS_Node* LoadedConnector = LoadedConnectorMesh
        ? AddChildNode(
            ConstructionScript,
            PitchPivot,
            UStaticMeshComponent::StaticClass(),
            CannonAssetNames::LoadedConnectorName)
        : nullptr;

    UBoxComponent* ModuleMountTemplate =
        CastChecked<UBoxComponent>(ModuleMountRoot->ComponentTemplate);
    ModuleMountTemplate->SetBoxExtent(FVector(
        CannonAssetNames::CollisionHalfWidthCentimeters,
        CannonAssetNames::CollisionHalfWidthCentimeters,
        CannonAssetNames::CollisionHalfHeightCentimeters));
    ModuleMountTemplate->SetRelativeLocation(CannonAssetNames::CollisionRelativeLocation);
    ModuleMountTemplate->SetCollisionProfileName(
        CannonAssetNames::BlockAllDynamicCollisionProfileName);
    ModuleMountTemplate->SetGenerateOverlapEvents(
        CannonAssetNames::ModuleMountGeneratesInteractionOverlaps);
    ModuleMountTemplate->SetSimulatePhysics(false);

    UVoyageDynamicCollisionComponent* DynamicCollisionTemplate =
        CastChecked<UVoyageDynamicCollisionComponent>(DynamicCollision->ComponentTemplate);
    DynamicCollisionTemplate->bAutoWeld = true;
    DynamicCollisionTemplate->bAutoweldIgnoreNormal = false;

    ConfigureBodyMesh(
        BaseBody,
        BaseMesh,
        CannonAssetNames::VisualMeshRelativeLocation,
        CannonAssetNames::VisualMeshRelativeScale);
    ConfigureBodyMesh(
        TurretBody,
        YawMesh,
        CannonAssetNames::VisualMeshRelativeLocation,
        CannonAssetNames::VisualMeshRelativeScale);
    ConfigureBodyMesh(
        BarrelBody,
        PitchMesh,
        CannonAssetNames::VisualMeshRelativeLocation,
        CannonAssetNames::VisualMeshRelativeScale);
    if (LoadedConnector)
    {
        ConfigureDecorativeMesh(
            LoadedConnector,
            LoadedConnectorMesh,
            CannonAssetNames::LoadedConnectorRelativeLocation,
            CannonAssetNames::LoadedConnectorRelativeRotation,
            CannonAssetNames::LoadedConnectorRelativeScale);
    }
    CastChecked<USceneComponent>(PitchPivot->ComponentTemplate)->SetRelativeLocation(
        CannonAssetNames::PitchPivotRelativeLocation);

    Blueprint->ComponentClassOverrides.Emplace(FBPComponentClassOverride(
        CannonAssetNames::ModuleComponentName,
        UVoyageCustomModuleComponent::StaticClass()));

    // Shell-only restoration deliberately emits no Fabricated/BeginPlay/Tick
    // operator path. The historical lifecycle helpers above are not invoked.

    if (!CompileGeneratedBlueprint(Blueprint))
    {
        return nullptr;
    }

    AVoyageModuleActor* ClassDefaultObject =
        CastChecked<AVoyageModuleActor>(Blueprint->GeneratedClass->GetDefaultObject());
    UVoyageModuleComponent* ModuleTemplate = ClassDefaultObject->ModuleComponent;
    if (!ModuleTemplate || !ModuleTemplate->IsA<UVoyageCustomModuleComponent>())
    {
        UE_LOG(LogTemp, Error, TEXT("Leaf probe did not construct the custom native module override"));
        return nullptr;
    }
    ModuleTemplate->ItemAsset = LeafItem;

    if (!SaveGeneratedAsset(Package, Blueprint))
    {
        return nullptr;
    }
    return Blueprint;
}
}

UGenerateHarpoonCannonCommandlet::UGenerateHarpoonCannonCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateHarpoonCannonCommandlet::Main(const FString& Params)
{
    if (!FParse::Param(*Params, CannonAssetNames::ShellOnlyParameter) ||
        CannonAssetNames::IncludeBaseGameLoadedConnectorReference)
    {
        UE_LOG(LogTemp, Error, TEXT("Only explicit -ShellOnly with the mod-authored connector proxy is currently revalidated; historical operator generation is blocked"));
        return 1;
    }
    if (FPackageName::DoesPackageExist(CannonAssetNames::PawnPackageName) ||
        FPackageName::DoesPackageExist(CannonAssetNames::HudPackageName) ||
        FPackageName::DoesPackageExist(CannonAssetNames::LeafProbePackageName) ||
        FPackageName::DoesPackageExist(CannonAssetNames::BaseMeshPackageName) ||
        FPackageName::DoesPackageExist(CannonAssetNames::YawMeshPackageName) ||
        FPackageName::DoesPackageExist(CannonAssetNames::PitchMeshPackageName) ||
        (CannonAssetNames::IncludeBaseGameLoadedConnectorReference &&
            FPackageName::DoesPackageExist(CannonAssetNames::LoadedConnectorPackageName)) ||
        (CannonAssetNames::UsesStockCameraDroneClass &&
            FPackageName::DoesPackageExist(CannonAssetNames::CameraDronePackageName)) ||
        FPackageName::DoesPackageExist(CannonAssetNames::LeafItemPackageName))
    {
        UE_LOG(LogTemp, Error, TEXT("Harpoon Cannon generated assets already exist"));
        return 1;
    }

    FObjSource VisualSource;
    if (!ParseObjSource(VisualSource))
    {
        return 1;
    }
    UStaticMesh* BaseMesh = CreateVisualMesh(
        VisualSource,
        CannonAssetNames::BaseMeshPackageName,
        CannonAssetNames::BaseMeshAssetName,
        CannonAssetNames::StaticMountObjects,
        UE_ARRAY_COUNT(CannonAssetNames::StaticMountObjects),
        FVector::ZeroVector,
        CannonAssetNames::BaseMeshBuildsFabricatorInteractionCollision);
    UStaticMesh* YawMesh = CreateVisualMesh(
        VisualSource,
        CannonAssetNames::YawMeshPackageName,
        CannonAssetNames::YawMeshAssetName,
        CannonAssetNames::YawAssemblyObjects,
        UE_ARRAY_COUNT(CannonAssetNames::YawAssemblyObjects),
        FVector::ZeroVector,
        CannonAssetNames::MovingMeshBuildsFabricatorInteractionCollision);
    UStaticMesh* PitchMesh = CreateVisualMesh(
        VisualSource,
        CannonAssetNames::PitchMeshPackageName,
        CannonAssetNames::PitchMeshAssetName,
        CannonAssetNames::PitchAssemblyObjects,
        UE_ARRAY_COUNT(CannonAssetNames::PitchAssemblyObjects),
        CannonAssetNames::PitchPivotRelativeLocation,
        CannonAssetNames::MovingMeshBuildsFabricatorInteractionCollision);
    UStaticMesh* LoadedConnectorMesh =
        CannonAssetNames::IncludeBaseGameLoadedConnectorReference
        ? CreateLoadedConnectorReferenceStub()
        : nullptr;
    UVoyageItem* LeafItem = CreateLeafItemReferenceStub();
    UBlueprint* LeafProbe =
        BaseMesh && YawMesh && PitchMesh && LeafItem
        ? CreateLeafPlacementProbe(
            BaseMesh,
            YawMesh,
            PitchMesh,
            LoadedConnectorMesh,
            LeafItem,
            nullptr,
            nullptr)
        : nullptr;
    if (!BaseMesh || !YawMesh || !PitchMesh || !LeafItem || !LeafProbe ||
        (CannonAssetNames::IncludeBaseGameLoadedConnectorReference &&
            !LoadedConnectorMesh))
    {
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("Generated OBJ-backed Harpoon visual meshes"));
    if (CannonAssetNames::IncludeBaseGameLoadedConnectorReference)
    {
        UE_LOG(LogTemp, Display, TEXT("Generated editor-only stock connector reference stub %s"), CannonAssetNames::LoadedConnectorPackageName);
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("Using the mod-authored connector proxy for this isolated visual checkpoint"));
    }
    UE_LOG(LogTemp, Display, TEXT("Generated editor-only item reference stub %s"), CannonAssetNames::LeafItemObjectPath);
    UE_LOG(LogTemp, Display, TEXT("Shell-only: no operator, Drone, HUD, possession or lifecycle marker generated"));
    UE_LOG(LogTemp, Display, TEXT("Generated temporary leaf replacement probe %s"), CannonAssetNames::LeafProbePackageName);
    return 0;
}

#endif
