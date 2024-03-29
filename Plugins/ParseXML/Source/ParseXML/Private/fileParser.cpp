#include "fileParser.h"
#include "EngineMinimal.h"
#include "Runtime/XmlParser/Public/FastXml.h"
#include "Runtime/XmlParser/Public/XmlParser.h"
#include "Engine.h"
#include <cstdlib>
#include <sstream>
#include <memory>


UfileParser::UfileParser(const TCHAR* selectedFile) : selectedXMLFile(selectedFile)
{
	FVector Location = FVector(0.0f, 0.0f, 2000.0f);
	FRotator Rotation = FRotator(0.0f, 0.0f, 0.0f);
	FActorSpawnParameters SpawnParameters;
	getAllActorsOfClass();
	World->SpawnActor<AAtmosphericFog>(Location, Rotation, SpawnParameters);
	Location.Z = 100000.0f;
	ASkyLight* Skylight = World->SpawnActor<ASkyLight>(Location, Rotation, SpawnParameters);
	if (Skylight !=nullptr) {
		UE_LOG(LogEngine, Warning, TEXT("skylight spawned!"));
		Skylight->GetLightComponent()->SetIntensity(5.0f);
		//Skylight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		GEditor->BuildLighting(LightOptions);
	}
}

UfileParser::~UfileParser()
{
	
}

void UfileParser::destroyFoundActors() {
	for (int i = 0; i < FoundActors.Num(); i++) {
		World->DestroyActor(FoundActors[i]); //Destroy all actors before starting
	}
}



void UfileParser::getAllActorsOfClass() {

	UGameplayStatics::GetAllActorsOfClass(World, ARoadMesh::StaticClass(), FoundActors);
	destroyFoundActors();
	UGameplayStatics::GetAllActorsOfClass(World, AAtmosphericFog::StaticClass(), FoundActors);
	destroyFoundActors();
	UGameplayStatics::GetAllActorsOfClass(World, ASkyLight::StaticClass(), FoundActors);
	destroyFoundActors();
	UGameplayStatics::GetAllActorsOfClass(World, AStopSignMesh::StaticClass(), FoundActors);
	destroyFoundActors();
}


void UfileParser::InitializeEdgeAttributes(const TCHAR* AttributeName, const TCHAR* AttributeValue)
{
	if (FString(AttributeName).Equals(TEXT("id")))
	{
		tempEdgeID = FString(AttributeValue);
		return;
	}

	else if (FString(AttributeName).Equals(TEXT("from")))
	{
		fromNode = FString(AttributeValue);
		fromNodeSet = true;
		return;
	}

	else if (FString(AttributeName).Equals(TEXT("to")))
	{
		toNode = FString(AttributeValue);
		toNodeSet = true;
		return;
	}

	else if (FString(AttributeName).Equals(TEXT("length")))
	{
		laneLength = FString(AttributeValue);
		lengthIsSet = true;
		return;
	}

	else if (FString(AttributeName).Equals(TEXT("shape")))
	{
		if (isElementLane == true)
		{
			ShapeProcessing(AttributeValue);
			shapeIsSet = true;
		}
		return;
	}

	else if (FString(AttributeName).Equals(TEXT("width")))
	{
		laneWidth = FCString::Atof(AttributeValue);
		laneWidthIsSet = true;
	}

	else if (FString(AttributeName).Equals(TEXT("allow"))) {
		isSidewalk = true;
		return;
	}

	else return;
}

FString UfileParser::getTempNodeID()
{
	return tempNodeID;
}

bool UfileParser::setTempNodeID(const TCHAR* tempTempNodeID)
{
	tempNodeID = FString(tempTempNodeID);
	return (tempNodeID.IsEmpty());

}

void UfileParser::ShapeProcessing(const TCHAR* ShapeString)
{
	std::string CoordinateString = TCHAR_TO_UTF8(ShapeString);
	Shapecoordinates.clear();
	for (std::string::iterator it = CoordinateString.begin(); it != CoordinateString.end(); ++it)
	{
		if (*it == ',')
		{
			*it = ' ';
		}
		else continue;			//convert all commas in string stream into space
	}

	std::stringstream ss;
	ss << CoordinateString;

	float found;

	int i = 1;
	while (!ss.eof())
	{
		//check if it is valid to put stringstream object into float variable. Also check for every second index - if found multiply with with negative 1 to mirror about y axis.
		if (ss >> found)
		{
			if ((i % 2) == 0)
			{
				found = (-1) * found; //mirror the network about the x-axis. This means changing the sign of the y coordinate. 
				Shapecoordinates.push_back(found*100); //Since the default unreal engine unit is cm and the default SUMO unit is m, we perform the conversion here.
				i += 1;
			}
			else
			{
				Shapecoordinates.push_back(found*100);
				i += 1;
			}
		}
	}
}

void UfileParser::resetFlagsAndTempMembers()
{
	//isElementNode = false;
	//isTrafficNode = false;
	isPriorityNode = false;
	tempNodeID = "";
	nodeXCoordinate = nullptr;
	nodeYCoordinate = nullptr;
	Shapecoordinates.clear();
	xCoordinateIsSet = false;
	yCoordinateIsSet = false;
	shapeIsSet = false;

	//isElementEdge = false;
	tempEdgeID = "";
	fromNode = "";
	toNode = "";
	laneLength = "";
	fromNodeSet = false;
	toNodeSet = false;
	lengthIsSet = false;
}
void UfileParser::InitializetrafficLightAttributes(const TCHAR* AttributeName, const TCHAR* AttributeValue)
{
	if (FString(AttributeName).Equals(TEXT("id")))
	{
		tempTrafficLightID = FString(AttributeValue);
		return;
	}
}

void UfileParser::InitializeWalkingAreaAttributes(const TCHAR* AttributeName, const TCHAR* AttributeValue)
{
	if (FString(AttributeName).Equals(TEXT("id")))
	{
		walkingAreaID = FString(AttributeValue);
		return;
	}

	else if ((FString(AttributeName)).Equals(TEXT("shape")))
	{
		ShapeProcessing(AttributeValue);
		shapeIsSet = true;
		return;
	}
	else return;

}

void UfileParser::InitializeNodeAttributes(const TCHAR* AttributeName, const TCHAR* AttributeValue)
{
	if (FString(AttributeName).Equals(TEXT("id")))
	{
		setTempNodeID(AttributeValue);
		return;
	}

	else if ((FString(AttributeValue)).Equals(TEXT("priority")))
	{
		isPriorityNode = true;
		return;
	}

	else if ((FString(AttributeValue)).Equals(TEXT("traffic_light")) || FString(AttributeValue).Equals(TEXT("allway_stop")))
	{
		isTrafficNode = true;
		TUniquePtr<FString> tempPointer = MakeUnique<FString>(tempNodeID);//creates a new object which is 'owned' by tempPointer
		trafficControlIDList.Push(MoveTempIfPossible(tempPointer)); 
		/*
		use the move constructor to 'move' the ownership of the newly created object to 
		the new unique pointer within trafficControlIDList. As long as the pointer exists,
		the object it is pointing to (the Node ID) will exist. 
		*/
		return;
	}

	//Set temp node positions
	if (((isPriorityNode == true) || (isTrafficNode == true)) && ((FString(AttributeName)).Equals(TEXT("x"))))
	{
		nodeXCoordinate = AttributeValue;
		xCoordinateIsSet = true;
		return;
	}

	else if (((isPriorityNode == true) || (isTrafficNode == true)) && ((FString(AttributeName)).Equals(TEXT("y"))))
	{
		nodeYCoordinate = AttributeValue;
		yCoordinateIsSet = true;
		return;
	}

	else if (((isPriorityNode == true) || (isTrafficNode == true)) && ((FString(AttributeName)).Equals(TEXT("shape"))))
	{
		ShapeProcessing(AttributeValue);
		shapeIsSet = true;
		return;
	}

	else
	{
		return;
	}

}

//walking Area parsing is similar to the nodes. TMap storing the walkingArea objects is different. Also walkingArea meshes will be 
//3d at some point. 
void UfileParser::InitializewalkingArea()
{
	//unique_ptr for object creation for extended lifetime
	walkingAreaPtr currentWalkingArea = std::make_unique<walkingArea>();
	currentWalkingArea->walkingAreaShapeCoordinates = Shapecoordinates;
	currentWalkingArea->setWalkingAreaID(*walkingAreaID);

	FQuat RotationEdge(0.0f, 0.0f, 0.0f, 0.0f);
	currentWalkingArea->centroidCalculation();
	FVector origin(currentWalkingArea->centroidX, currentWalkingArea->centroidY, 0.1f);

	walkingAreaPtr movedWalkingAreaPointer = nullptr;

	FTransform SpawnTransform(RotationEdge, origin);
	ARoadMesh* MyDeferredActor = Cast<ARoadMesh>(UGameplayStatics::BeginDeferredActorSpawnFromClass(World, ARoadMesh::StaticClass(), SpawnTransform)); //Downcasting

	if (MyDeferredActor != nullptr)
	{
		FVector coordinates;
		int i = 0;
		while ((i + 2) <= currentWalkingArea->walkingAreaShapeCoordinates.size())
		{
			coordinates.X = currentWalkingArea->walkingAreaShapeCoordinates[i] - currentWalkingArea->centroidX;
			coordinates.Y = currentWalkingArea->walkingAreaShapeCoordinates[i + 1] - currentWalkingArea->centroidY;
			coordinates.Z = 0.0f;
			MyDeferredActor->vertices.Add(coordinates);
			i += 2;
		}
		UGameplayStatics::FinishSpawningActor(MyDeferredActor, SpawnTransform);
		walkingAreaContainer.walkingAreaMap.Add(*(currentWalkingArea->walkingAreaID), std::move(currentWalkingArea));
		/*
		Here we transfer ownership of the object pointed to by 'currentWalkingArea' pointer within the 
		walking area hashmap. As long as the pointer object exists, the object pointed by it will exist. 
		*/

	}
}


SimpleNodePtr UfileParser::InitializeNode()
{
	//unique_ptr for object creation for extended lifetime
	SimpleNodePtr Node = std::make_unique<SimpleNode>();
	Node->SetID(*tempNodeID);
	Node->nodeShapecoordinates = Shapecoordinates;
	Node->SetPosition(nodeXCoordinate, nodeYCoordinate);

	FQuat RotationEdge(0.0f, 0.0f, 0.0f, 0.0f);
	FVector origin(Node->NodePosition.X, Node->NodePosition.Y, 0.0f);


	FTransform SpawnTransform(RotationEdge, origin);
	ARoadMesh* MyDeferredActor = Cast<ARoadMesh>(UGameplayStatics::BeginDeferredActorSpawnFromClass(World, ARoadMesh::StaticClass(), SpawnTransform)); //Downcasting

	if (MyDeferredActor != nullptr)
	{
		FVector coordinates;
		int i=0;
		while ((i+2) <= Node->nodeShapecoordinates.size())
		{		
				coordinates.X = Node->nodeShapecoordinates[i] - Node->NodePosition.X;
				coordinates.Y = Node->nodeShapecoordinates[i + 1] - Node->NodePosition.Y;
				coordinates.Z = 0.0f;
				MyDeferredActor->vertices.Add(coordinates);	
				i += 2;
		}
		UGameplayStatics::FinishSpawningActor(MyDeferredActor, SpawnTransform);

		//initialize map with the pointer for extended node lifetime
		NodeContainer.NodeMap.Add(*tempNodeID, std::move(Node));
	}
	return Node;
}

SimpleEdgePtr UfileParser::InitializePedestrianEdge()
{
	//unique_ptr for object creation for extended lifetime
	SimpleEdgePtr Edge = std::make_unique<SimpleEdge>();
	Edge->SetID(*tempEdgeID);
	Edge->setFromID(*fromNode);
	Edge->setToID(*toNode);
	Edge->setLaneLength(*laneLength);

	int i = 0;
	std::vector<float> tempvector;
	while ((i + 3) <= (Shapecoordinates.size() - 1))
	{
		tempvector.push_back(Shapecoordinates[i]);
		tempvector.push_back(Shapecoordinates[i + 1]);
		tempvector.push_back(Shapecoordinates[i + 2]);
		tempvector.push_back(Shapecoordinates[i + 3]);

		Edge->setShapeCoordinates(tempvector);
		tempvector.clear();

		if (laneWidthIsSet == true) Edge->setSideWalkVertCoordinates(laneWidth*100); //default SUMO lane width
		else Edge->setSideWalkVertCoordinates(320);  //default SUMO lane width is 320cm. Default interstate highway lane width is 470cm (12 feet).

		FVector originCoordinates = Edge->centroid;
		FQuat RotationEdge(0.0f, 0.0f, 0.0f, 0.0f);
		FVector origin(0.0f, 0.0f, 0.0f);
		
		origin.X = originCoordinates.X;
		origin.Y = originCoordinates.Y;
		origin.Z = 0.2f;
		

		FTransform SpawnTransform(RotationEdge, origin);
		ARoadMesh* MyDeferredActor = Cast<ARoadMesh>(UGameplayStatics::BeginDeferredActorSpawnFromClass(World, ARoadMesh::StaticClass(), SpawnTransform)); //Downcasting

		if (MyDeferredActor != nullptr) {
			(MyDeferredActor->vertices) = (Edge->vertexArray);
			UGameplayStatics::FinishSpawningActor(MyDeferredActor, SpawnTransform);
		}

		FVector originCoordinatesCurb1 = Edge->CentroidcurbTop1;
		FQuat RotationEdgeCurb1(0.0f, 0.0f, 0.0f, 0.0f);
		FVector originCurb1(0.0f, 0.0f, 0.0f);

		originCurb1.X = originCoordinatesCurb1.X;
		originCurb1.Y = originCoordinatesCurb1.Y;
		originCurb1.Z = 0.2f;

		FTransform SpawnTransformCurb1(RotationEdgeCurb1, originCurb1);
		ARoadMesh* MyDeferredActor1 = Cast<ARoadMesh>(UGameplayStatics::BeginDeferredActorSpawnFromClass(World, ARoadMesh::StaticClass(), SpawnTransformCurb1)); //Downcasting

		if (MyDeferredActor1 != nullptr){
			MyDeferredActor1->vertices = Edge->curbVerticesTop1;
			UGameplayStatics::FinishSpawningActor(MyDeferredActor1, SpawnTransformCurb1);
		}

		FVector originCoordinatesCurb2 = Edge->CentroidcurbTop2;
		FQuat RotationEdgeCurb2(0.0f, 0.0f, 0.0f, 0.0f);
		FVector originCurb2(0.0f, 0.0f, 0.0f);

		originCurb2.X = originCoordinatesCurb2.X;
		originCurb2.Y = originCoordinatesCurb2.Y;
		originCurb2.Z = 0.2f;

		FTransform SpawnTransformCurb2(RotationEdgeCurb2, originCurb2);
		ARoadMesh* MyDeferredActor2 = Cast<ARoadMesh>(UGameplayStatics::BeginDeferredActorSpawnFromClass(World, ARoadMesh::StaticClass(), SpawnTransformCurb2)); //Downcasting

		if (MyDeferredActor2 !=nullptr) {
			MyDeferredActor2->vertices = Edge->curbVerticesTop2;
			UGameplayStatics::FinishSpawningActor(MyDeferredActor2, SpawnTransformCurb2);
		}
		i += 2;
	}

	//initialize map with the pointer for extended node lifetime
	EdgeContainer.EdgeMap.Add(*tempEdgeID, std::move(Edge));
	
	return Edge;
}

SimpleEdgePtr UfileParser::InitializeEdge(const TCHAR* edgeType)
{
	//unique_ptr for object creation for extended lifetime
	SimpleEdgePtr Edge = std::make_unique<SimpleEdge>();
	Edge->SetID(*tempEdgeID);
	Edge->setFromID(*fromNode);
	Edge->setToID(*toNode);
	Edge->setLaneLength(*laneLength);

	int i = 0;
	std::vector<float> tempvector;
	while ((i + 3) <= (Shapecoordinates.size() - 1))
	{
		tempvector.push_back(Shapecoordinates[i]);
		tempvector.push_back(Shapecoordinates[i+1]);
		tempvector.push_back(Shapecoordinates[i+2]);
		tempvector.push_back(Shapecoordinates[i+3]);

		Edge->setShapeCoordinates(tempvector);
		tempvector.clear();

		if (laneWidthIsSet == true)
		{
			Edge->setVertexCoordinates(laneWidth);  //default SUMO lane width
		}
		else
		{
			Edge->setVertexCoordinates(320);  //default SUMO lane width is 320cm. Default interstate highway lane width is 470cm. 
		}

		FVector originCoordinates = Edge->centroid;
		FQuat RotationEdge(0.0f, 0.0f, 0.0f, 0.0f);
		FVector origin(0.0f, 0.0f, 0.0f);
		if (FString(edgeType).Equals(TEXT("standard")))
		{
			origin.X = originCoordinates.X;
			origin.Y = originCoordinates.Y;
			origin.Z = originCoordinates.Z;
		}
		else if (FString(edgeType).Equals(TEXT("crossing")))
		{
			origin.X = originCoordinates.X;
			origin.Y = originCoordinates.Y;
			origin.Z = 0.1f;
		}
		else if (FString(edgeType).Equals(TEXT("sidewalk")))
		{
			origin.X = originCoordinates.X;
			origin.Y = originCoordinates.Y;
			origin.Z = 0.2f;
		}

		FTransform SpawnTransform(RotationEdge, origin);
		ARoadMesh* MyDeferredActor = Cast<ARoadMesh>(UGameplayStatics::BeginDeferredActorSpawnFromClass(World, ARoadMesh::StaticClass(), SpawnTransform)); //Downcasting

		if (MyDeferredActor)
		{
			(MyDeferredActor->vertices) = (Edge->vertexArray);
			UGameplayStatics::FinishSpawningActor(MyDeferredActor, SpawnTransform);
			//MyDeferredActor->FinishSpawning(SpawnLocAndRotation);
		}
		i+=2;
	}
	return Edge;
}


void UfileParser::InitializeTrafficControl(const TCHAR* controlType)//spawn two traffic lights per walking area - At the first and fourth x,y coordinate.
{
	walkingArea* currentWalkingAreaObject; 
	for (const TPair<const TCHAR*, walkingAreaPtr>& pair : walkingAreaContainer.walkingAreaMap)// Find the corresponding walking area for a traffic light for a particular junction.
	{
		FString currentKey = FString(pair.Key); //ID of the walking area.
		FString currentControlNodeID = FString(TEXT(":0_0_"));
		/*
		For stop sign from IntGen --> Netgenerate, we only check for the walking areas within one junction. 
		This is to prevent turn lane's junction walking areas to be considered when trying to place the stop sign. 
		*/
		FVector trafficControl1Location; 
		FQuat trafficControlRotation;
		FVector trafficLightScale(1.0f, 1.0f, 1.0f);

		if ((currentKey.Contains(currentControlNodeID)) && (FString(controlType).Equals(TEXT("trafficLight"))))
		{
			currentWalkingAreaObject = pair.Value.get();
			currentWalkingAreaObject->trafficControlLocationCalculator();
			trafficControl1Location = currentWalkingAreaObject->trafficControlLocation;
			trafficControlRotation = currentWalkingAreaObject->trafficLight1Orientation;
			FTransform SpawnTransform(trafficControlRotation, trafficControl1Location, trafficLightScale);
			AtrafficLightMesh* MyDeferredTrafficLight = Cast<AtrafficLightMesh>(UGameplayStatics::BeginDeferredActorSpawnFromClass(World, AtrafficLightMesh::StaticClass(), SpawnTransform)); //Downcasting
			UGameplayStatics::FinishSpawningActor(MyDeferredTrafficLight, SpawnTransform);
		}
		else if ((currentKey.Contains(currentControlNodeID)) && (FString(controlType).Equals(TEXT("stopSign"))))
		{
			currentWalkingAreaObject = pair.Value.get();

			trafficControl1Location = currentWalkingAreaObject->trafficControlLocationCalculator();
			trafficControlRotation = currentWalkingAreaObject->stopSignRotationCalculator();
			FVector stopSignScale(100.0f, 100.0f, 100.0f);
			FTransform SpawnTransform(trafficControlRotation, trafficControl1Location, stopSignScale);
			AStopSignMesh* MyDeferredStopSign = Cast<AStopSignMesh>(UGameplayStatics::BeginDeferredActorSpawnFromClass(World, AStopSignMesh::StaticClass(), SpawnTransform)); //Downcasting
			UGameplayStatics::FinishSpawningActor(MyDeferredStopSign, SpawnTransform); //deferred actor spawning because it gives us the option to change scale of actor when needed. 
		}
	}	
}
void UfileParser::iterateWalkingAreas()
{
	for (TMap<const TCHAR*, walkingAreaPtr>::TConstIterator it = walkingAreaContainer.walkingAreaMap.CreateConstIterator(); it; ++it)// Find the corresponding walking area for a traffic light for a particular junction.
	{
		FString currentKey = FString(it->Key);
		UE_LOG(LogEngine, Warning, TEXT("The walking area key is %s"), it->Key);
	}
}

bool UfileParser::loadxml()
{
	UE_LOG(LogEngine, Warning, TEXT("Loading started"));
	FText outError;
	int32 outErrorNum;
	FString Text = "";
	bool success = FFastXml::ParseXmlFile((IFastXmlCallback*)(this), selectedXMLFile.GetCharArray().GetData(), (TCHAR*)*Text, nullptr, false, false, outError, outErrorNum);
	return success;
}

bool UfileParser::ProcessXmlDeclaration(const TCHAR* ElementData, int32 XmlFileLineNumber)
{
	UE_LOG(LogEngine, Warning, TEXT("ProcessXmlDeclaration ElementData: %s, XmlFileLineNumber: %f"), ElementData, XmlFileLineNumber);
	return true;
}

bool UfileParser::ProcessElement(const TCHAR* ElementName, const TCHAR* ElementData, int32 XmlFileLineNumber)
{
	if ((FString(ElementName)).Equals(TEXT("junction")))
	{
		isElementNode = true;
		isElementEdge = false;
	}
	else if ((FString(ElementName)).Equals(TEXT("edge")))
	{
		isElementEdge = true;
		isElementNode = false;
	}
	else if ((FString(ElementName)).Equals(TEXT("lane")))
	{
		isElementLane = true;
	}
	else if ((FString(ElementName)).Equals(TEXT("tlLogic")))
	{
		isElementtrafficLight = true;
	}

	//UE_LOG(LogEngine, Warning, TEXT("ProcessElement ElementName: %s, ElementValue: %s"), ElementName, ElementData);
	return true;
}

bool UfileParser::ProcessAttribute(const TCHAR* AttributeName, const TCHAR* AttributeValue)
{
	//Checking if all the required attributes are populated for the required mesh to be spawned.

	if (isElementNode == true)
	{
		InitializeNodeAttributes(AttributeName, AttributeValue);
		if ((shapeIsSet == true) && (xCoordinateIsSet == true) && (yCoordinateIsSet == true))
		{
			InitializeNode(); 
			UE_LOG(LogEngine, Warning, TEXT("Node object created!"));
		}
	}
	else if ((isElementEdge == true) || (isElementLane == true))
	{
		InitializeEdgeAttributes(AttributeName, AttributeValue);
	}

	if (FString(AttributeValue).Equals(TEXT("walkingarea")))
	{
		isWalkingArea = true;
	}
	if (isWalkingArea == true)
	{
		UE_LOG(LogEngine, Warning, TEXT("walking area true"));
		InitializeWalkingAreaAttributes(AttributeName, AttributeValue);
	}

	if (FString(AttributeValue).Equals(TEXT("crossing")))
	{
		isCrossing = true;
	}

	if (isCrossing == true)
	{
		UE_LOG(LogEngine, Warning, TEXT("crossing is true"));
		InitializeEdgeAttributes(AttributeName, AttributeValue);
	}

	if (isElementtrafficLight == true)
	{
		InitializetrafficLightAttributes(AttributeName, AttributeValue);
	}
	return true;
}

bool UfileParser::ProcessClose(const TCHAR* Element)
{
	if ((fromNodeSet == true) && (toNodeSet == true) && (lengthIsSet == true) && (shapeIsSet == true))
	{
		if (isSidewalk == true)
		{
			InitializePedestrianEdge();
		}
		else
		{
			InitializeEdge(TEXT("standard"));
		}
		//UE_LOG(LogEngine, Warning, TEXT("Edge object created!")); 
	}
	else if ((isCrossing == true) && (shapeIsSet == true))
	{
		InitializeEdge(TEXT("crossing"));
	}
	if ((FString(Element)).Equals(TEXT("lane"))) //To allow multiple lanes in an edge element
		//we need to clear out the data of the previous lane. 
	{
		if (isWalkingArea == false)
		{
			Shapecoordinates.clear();
			shapeIsSet = false;
			lengthIsSet = false;
			isElementLane = false;
			laneWidthIsSet = false;
			isCrossing = false;
			isSidewalk = false;
		}
		else
		{
			if (shapeIsSet)
			{
				UE_LOG(LogEngine, Warning, TEXT("Walking area shape is set"));
				InitializewalkingArea();
				iterateWalkingAreas();
				UE_LOG(LogEngine, Warning, TEXT("WalkingArea created"));
				doesWalkingAreaExist = true; 
				isWalkingArea = false;
				shapeIsSet = false;
			}
		}
	}
	else
	{
		resetFlagsAndTempMembers();
	}

	if (isTrafficNode == true)
	{
		InitializeTrafficControl(TEXT("stopSign"));
		isElementtrafficLight = false;
		tempTrafficLightID = "";
		isTrafficNode = false;
	}
	return true;
}

bool UfileParser::ProcessComment(const TCHAR* Comment)
{
	//UE_LOG(LogEngine, Warning, TEXT("ProcessComment Comment: %s"), Comment);
	return true;
}









