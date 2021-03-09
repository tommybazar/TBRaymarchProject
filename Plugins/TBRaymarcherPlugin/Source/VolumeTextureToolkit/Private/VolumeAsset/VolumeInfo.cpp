// Copyright 2021 Tomas Bartipan and Technical University of Munich.
// Licensed under MIT license - See License.txt for details.
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision) and Ryan Brucks
// (original raymarching code).

#include "VolumeAsset/VolumeInfo.h"

#include "sstream"
#include "string"

int64 FVolumeInfo::GetTotalBytes()
{
	return Dimensions.X * Dimensions.Y * Dimensions.Z * BytesPerVoxel;
}

int64 FVolumeInfo::GetTotalVoxels()
{
	return Dimensions.X * Dimensions.Y * Dimensions.Z;
}

float FVolumeInfo::NormalizeValue(float InValue)
{
	if (!bIsNormalized)
	{
		return InValue;
	}
	// Normalize on the range of [Min, Max]
	return ((InValue - MinValue) / (MaxValue - MinValue));
}

float FVolumeInfo::DenormalizeValue(float InValue)
{
	if (!bIsNormalized)
	{
		return InValue;
	}
	return ((InValue * (MaxValue - MinValue)) + MinValue);
}

float FVolumeInfo::NormalizeRange(float InRange)
{
	if (!bIsNormalized)
	{
		return InRange;
	}
	// Normalize the range from [Max - Min] to 1
	return ((InRange) / (MaxValue - MinValue));
}

float FVolumeInfo::DenormalizeRange(float InRange)
{
	if (!bIsNormalized)
	{
		return InRange;
	}
	// Normalize the range from [Max - Min] to 1
	return (InRange * (MaxValue - MinValue));
}

FString FVolumeInfo::ToString() const
{
	FString text = "File name " + DataFileName + " details:" + "\nDimensions = " + Dimensions.ToString() +
				   "\nSpacing : " + Spacing.ToString() + "\nWorld Size MM : " + Dimensions.ToString() +
				   "\nDefault window center : " + FString::SanitizeFloat(DefaultWindowingParameters.Center) +
				   "\nDefault window width : " + FString::SanitizeFloat(DefaultWindowingParameters.Width) + "\nOriginal Range : [" +
				   FString::SanitizeFloat(MinValue) + " - " + FString::SanitizeFloat(MaxValue) + "]";
	return text;
}

FVolumeInfo FVolumeInfo::ParseFromString(const FString FileString)
{
	// #TODO UE probably has a nicer string parser than istringstream...
	// And the way I'm doing this is the ugliest you could imagine.
	// But hey, this is probably literally the first C++ code I ever wrote in Unreal, so I'm keeping it this way, so
	// I can look at it and shed a tear of remembering the sweet, sweet days of yesteryear.

	FVolumeInfo OutVolumeInfo;
	OutVolumeInfo.bParseWasSuccessful = false;

	// #TODO stop being sentimental and use FConsole::Parse()
	{
		std::string MyStdString(TCHAR_TO_UTF8(*FileString));
		std::istringstream inStream = std::istringstream(MyStdString);

		std::string ReadWord;

		// Skip until we get to Dimensions.
		while (inStream.good() && ReadWord != "DimSize")
		{
			inStream >> ReadWord;
		}
		// Should be at the "=" after DimSize now.
		if (inStream.good())
		{
			// Get rid of equal sign.
			inStream >> ReadWord;
			// Read the three values;
			inStream >> OutVolumeInfo.Dimensions.X;
			inStream >> OutVolumeInfo.Dimensions.Y;
			inStream >> OutVolumeInfo.Dimensions.Z;
		}
		else
		{
			return OutVolumeInfo;
		}

		// Go back to beginning
		inStream = std::istringstream(MyStdString);
		// Skip until we get to spacing.
		while (inStream.good() && ReadWord != "ElementSpacing" && ReadWord != "ElementSize")
		{
			inStream >> ReadWord;
		}
		// Should be at the "=" after ElementSpacing/ElementSize now.
		if (inStream.good())
		{
			// Get rid of equal sign.
			inStream >> ReadWord;
			// Read the three values;
			inStream >> OutVolumeInfo.Spacing.X;
			inStream >> OutVolumeInfo.Spacing.Y;
			inStream >> OutVolumeInfo.Spacing.Z;

			OutVolumeInfo.WorldDimensions = OutVolumeInfo.Spacing * FVector(OutVolumeInfo.Dimensions);
		}
		else
		{
			return OutVolumeInfo;
		}

		// Go back to beginning
		inStream = std::istringstream(MyStdString);
		// Skip until we get to ElementType
		while (inStream.good() && ReadWord != "ElementType")
		{
			inStream >> ReadWord;
		}
		// Should be at the "=" after ElementType now.
		if (inStream.good())
		{
			// Get rid of equal sign.
			inStream >> ReadWord;

			inStream >> ReadWord;
			if (ReadWord == "MET_UCHAR")
			{
				OutVolumeInfo.VoxelFormat = EVolumeVoxelFormat::UnsignedChar;
			}
			else if (ReadWord == "MET_CHAR")
			{
				OutVolumeInfo.VoxelFormat = EVolumeVoxelFormat::SignedChar;
			}

			else if (ReadWord == "MET_USHORT")
			{
				OutVolumeInfo.VoxelFormat = EVolumeVoxelFormat::UnsignedShort;
			}
			else if (ReadWord == "MET_SHORT")
			{
				OutVolumeInfo.VoxelFormat = EVolumeVoxelFormat::SignedShort;
			}
			else if (ReadWord == "MET_UINT")
			{
				OutVolumeInfo.VoxelFormat = EVolumeVoxelFormat::UnsignedInt;
			}
			else if (ReadWord == "MET_INT")
			{
				OutVolumeInfo.VoxelFormat = EVolumeVoxelFormat::SignedInt;
			}
			else if (ReadWord == "MET_FLOAT")
			{
				OutVolumeInfo.VoxelFormat = EVolumeVoxelFormat::Float;
			}
			else
			{
				return OutVolumeInfo;
			}
		}
		else
		{
			return OutVolumeInfo;
		}

		OutVolumeInfo.BytesPerVoxel = FVolumeInfo::VoxelFormatByteSize(OutVolumeInfo.VoxelFormat);
		OutVolumeInfo.bIsSigned = FVolumeInfo::IsVoxelFormatSigned(OutVolumeInfo.VoxelFormat);

		// Check for compressed data size tag.

		// Go back to beginning
		inStream = std::istringstream(MyStdString);
		// Skip until we get to ElementType
		while (inStream.good() && ReadWord != "CompressedDataSize")
		{
			inStream >> ReadWord;
		}
		// Should be at the "=" after ElementType now.
		if (inStream.good())
		{
			OutVolumeInfo.bIsCompressed = true;

			// Get rid of equal sign.
			inStream >> ReadWord;

			inStream >> OutVolumeInfo.CompressedBytes;
		}

		// Go back to beginning
		inStream = std::istringstream(MyStdString);
		// Skip until we get to ElementType
		while (inStream.good() && ReadWord != "ElementDataFile")
		{
			inStream >> ReadWord;
		}
		// Should be at the "=" after ElementType now.
		if (inStream.good())
		{
			// Get rid of equal sign.
			inStream >> ReadWord;

			inStream >> ReadWord;
		}
		else
		{
			return OutVolumeInfo;
		}
		OutVolumeInfo.DataFileName = FString(ReadWord.c_str());
		OutVolumeInfo.bParseWasSuccessful = true;
		// Return with constructor that sets success to true.
		return OutVolumeInfo;
	}
}
