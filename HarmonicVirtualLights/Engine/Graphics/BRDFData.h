#pragma once

#include <vector>

struct RGB
{
	double r;
	double g;
	double b;
};

class BRDFData
{
private:
	const uint32_t MAX_L = 6;
	const uint32_t NUM_SH_COEFFICIENTS = (MAX_L + 1) * (MAX_L + 1);

	// shCoefficients[wo][shIndex]
	std::vector<std::vector<RGB>> shCoefficients;

public:
	bool createFromFile(const std::string& filePath);
};