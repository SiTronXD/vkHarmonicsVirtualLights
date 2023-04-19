#pragma once

#include <string>
#include <vector>
#include "SphericalHarmonics.h"

class Exporter
{
public:
	void writeToFile(const std::vector<std::vector<RGB>>& shCoefficients, const std::string& fileName);
	void printAsGLSLArray(const std::vector<std::vector<RGB>>& shCoefficients);
};