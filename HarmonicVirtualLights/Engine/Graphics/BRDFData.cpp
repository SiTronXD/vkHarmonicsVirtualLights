#include "pch.h"
#include "BRDFData.h"

bool BRDFData::createFromFile(const std::string& filePath)
{
	// Open file
	std::ifstream brdfFile(filePath);
	if (!brdfFile.is_open())
	{
		Log::error("Failed to open BRDF file: " + filePath);
		return false;
	}

	// Read lines
	std::string line;
	while (std::getline(brdfFile, line))
	{
		std::cout << line << std::endl;
	}

	// Close file
	brdfFile.close();

	return true;
}
