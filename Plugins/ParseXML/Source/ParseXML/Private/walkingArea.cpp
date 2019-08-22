#include "walkingArea.h"

walkingArea::walkingArea()
{

}

walkingArea::~walkingArea()
{

}

void walkingArea::setWalkingAreaID(const TCHAR* tempID)
{
	walkingAreaID = FString(tempID);
	UE_LOG(LogEngine, Warning, TEXT("New walkingArea ID is %s"), tempID);
}

void walkingArea::centroidCalculation()
{
	float cumulativeX = 0;
	float cumulativeY = 0;
	for (int i = 0; i <= (walkingAreaShapeCoordinates.size() - 1); i++)
	{
		if ((i % 2) == 0)
		{
			cumulativeX = cumulativeX + walkingAreaShapeCoordinates[i];
		}
		else
		{
			cumulativeY = cumulativeY + walkingAreaShapeCoordinates[i];
		}
	}
	centroidX = cumulativeX / (walkingAreaShapeCoordinates.size() / 2);
	centroidY = cumulativeY / (walkingAreaShapeCoordinates.size() / 2);
}

FVector walkingArea::directionVectorCalculator(float x0, float y0, float x1, float y1)
{
	FVector position1;
	position1.X = x0;
	position1.Y = y0;
	position1.Z = 0.0f;

	FVector position2;
	position2.X = x1;
	position2.Y = y1;
	position2.Z = 0.0f;

	FVector directionVector = position2 - position1;

	return directionVector;
}

FVector walkingArea::trafficControlLocationCalculator()
{
	NoTrafficLightatWalkingArea = false;
	if (walkingAreaShapeCoordinates.size() > 12) //To eliminate non-junction walking areas
	{
		FVector directionVector = directionVectorCalculator(walkingAreaShapeCoordinates[4], walkingAreaShapeCoordinates[5], walkingAreaShapeCoordinates[6], walkingAreaShapeCoordinates[7]);
		//FVector rightVector = directionVector.ToOrientationQuat().GetRightVector();

		trafficControlLocation.X = walkingAreaShapeCoordinates[6];
		trafficControlLocation.Y = walkingAreaShapeCoordinates[7];
		trafficControlLocation.Z = 200.0f;

		trafficLight1Orientation = directionVector.ToOrientationQuat();
	}
	else
	{
		FVector directionVector = directionVectorCalculator(walkingAreaShapeCoordinates[2], walkingAreaShapeCoordinates[3], walkingAreaShapeCoordinates[4], walkingAreaShapeCoordinates[5]);
		//FVector rightVector = directionVector.ToOrientationQuat().GetRightVector();

		trafficControlLocation.X = walkingAreaShapeCoordinates[2];
		trafficControlLocation.Y = walkingAreaShapeCoordinates[3];
		trafficControlLocation.Z = 200.0f;

		trafficLight1Orientation = directionVector.ToOrientationQuat();
	}
	return trafficControlLocation;
}

FQuat walkingArea::stopSignRotationCalculator() {
	FQuat stopSignRotationQuat;
	if (walkingAreaShapeCoordinates.size() > 12)
	{
		FVector directionVector = directionVectorCalculator(walkingAreaShapeCoordinates[4], walkingAreaShapeCoordinates[5], walkingAreaShapeCoordinates[6], walkingAreaShapeCoordinates[7]);
		stopSignRotationQuat = getStopSignOrientation(directionVector);
	}
	else
	{
		FVector directionVector = directionVectorCalculator(walkingAreaShapeCoordinates[2], walkingAreaShapeCoordinates[3], walkingAreaShapeCoordinates[4], walkingAreaShapeCoordinates[5]);
		stopSignRotationQuat = getStopSignOrientation(directionVector);
	}
	return stopSignRotationQuat;
}

FQuat walkingArea::getStopSignOrientation(FVector& currentDirectionVector) {
	//Next two lines are unique for the stop sign 3D model in our project. There is an offset of 180 degrees for the yaw value.   
	FRotator stopSignRotation = currentDirectionVector.ToOrientationQuat().GetRightVector().ToOrientationQuat().Rotator(); 
	stopSignRotation.Roll += 90.0f; //Make the stop sign stand
	stopSignRotation.Yaw += 180.0f;
	stopSignOrientation = stopSignRotation.Quaternion(); //Convert rotation to FQuat type to give it as an input while spawning the stopSign mesh (deferred actor spawn). 
	return stopSignOrientation;
}


