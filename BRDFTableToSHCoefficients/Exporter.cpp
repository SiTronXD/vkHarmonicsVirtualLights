#include <fstream>
#include "Exporter.h"

void Exporter::splitString(
	std::string& str,
	const char& delim,
	std::vector<std::string>& output)
{
	size_t pos = 0;
	while ((pos = str.find(delim)) != std::string::npos)
	{
		// Extract and add token
		output.emplace_back(str.substr(0, pos));

		// Remove front
		str.erase(0, pos + 1);
	}

	// Add remaining string
	output.emplace_back(str);
}

void Exporter::writeToFile(
	const std::vector<std::vector<RGB>>& shCoefficients, 
	std::string fileName)
{
	// Preprocess file name
	std::vector<std::string> splitString;
	this->splitString(fileName, '.', splitString);
	fileName = "exportedBRDFs/" + splitString[0] + ".shbrdf";

	// Create file
	std::ofstream brdfFile(fileName);
	if (!brdfFile.is_open())
	{
		fprintf(stderr, "Failed to export \"%s\"\n", fileName.c_str());
		return;
	}

	// Write to file
	for (size_t i = 0; i < shCoefficients.size(); ++i)
	{
		brdfFile << "k: " << i << "/" << shCoefficients.size() << std::endl;

		const std::vector<RGB>& coeffs = shCoefficients[i];
		for (size_t j = 0; j < coeffs.size(); ++j)
		{
			brdfFile << coeffs[j].r << " " << coeffs[j].g << " " << coeffs[j].b << std::endl;
		}

		brdfFile << std::endl;
	}

	// Close file
	brdfFile.close();

	printf(("Output SH BRDF file: " + fileName).c_str());
}

void Exporter::printAsGLSLArray(const std::vector<std::vector<RGB>>& shCoefficients)
{
	printf("const SHVector F[NUM_ANGLES] = SHVector[NUM_ANGLES](\n");
	for (size_t i = 0; i < shCoefficients.size(); ++i)
	{
		const std::vector<RGB>& coeffs = shCoefficients[i];

		printf("\tSHVector(vec3[NUM_SH_COEFFS](\n");
		for (size_t j = 0; j < coeffs.size(); ++j)
		{
			printf("\t\tvec3(%f, %f, %f)", coeffs[j].r, coeffs[j].g, coeffs[j].b);
			if (j != coeffs.size() - 1)
			{
				printf(",");
			}
			printf("\n");
		}
		printf("\t))");

		if (i != shCoefficients.size() - 1)
		{
			printf(",\n");
		}
	}
	printf(");\n");
}
