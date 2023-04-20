#pragma once

#include <string>
#include <vector>
#include "SphericalHarmonics.h"

class Exporter
{
private:
	void splitString(
		std::string& str,
		const char& delim,
		std::vector<std::string>& output);
public:
	void writeToFile(
		const std::vector<std::vector<RGB>>& shCoefficients, 
		const std::vector<std::vector<RGB>>& shCoefficientsCos,
		std::string fileName);
	void printAsGLSLArray(const std::vector<std::vector<RGB>>& shCoefficients);
};