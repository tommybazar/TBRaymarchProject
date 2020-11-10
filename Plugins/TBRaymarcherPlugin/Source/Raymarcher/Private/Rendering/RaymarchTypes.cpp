// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).

#include "Rendering/RaymarchTypes.h"

FString GetDirectionName(FCubeFace Face)
{
	switch (Face)
	{
		case FCubeFace::XPositive:
			return FString("+X");
		case FCubeFace::XNegative:
			return FString("-X");
		case FCubeFace::YPositive:
			return FString("+Y");
		case FCubeFace::YNegative:
			return FString("-Y");
		case FCubeFace::ZPositive:
			return FString("+Z");
		case FCubeFace::ZNegative:
			return FString("-Z");
		default:
			return FString("Something went wrong here!");
	}
}

bool SortDescendingWeights(const std::pair<FCubeFace, float>& a, const std::pair<FCubeFace, float>& b)
{
	return (a.second > b.second);
}

FMajorAxes FMajorAxes::GetMajorAxes(FVector LightPos)
{
	FMajorAxes RetVal;
	std::vector<std::pair<FCubeFace, float>> faceVector;

	for (int i = 0; i < 6; i++)
	{
		// Dot of position and face normal yields cos(angle)
		float weight = FVector::DotProduct(FCubeFaceNormals[i], LightPos);

		// Need to make sure we go along the axis with a positive weight, not negative.
		weight = (weight > 0 ? weight * weight : 0);
		RetVal.FaceWeight.push_back(std::make_pair(FCubeFace(i), weight));
	}
	// Sort so that the 3 major axes are the first.
	std::sort(RetVal.FaceWeight.begin(), RetVal.FaceWeight.end(), SortDescendingWeights);
	return RetVal;
}
