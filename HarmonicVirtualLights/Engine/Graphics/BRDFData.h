#pragma once

#include <vector>

struct RGB
{
	double rgb[3];
};

class BRDFData
{
private:
	// shCoefficients[wo][shIndex]
	std::vector<std::vector<RGB>> shCoefficients;

public:
	bool createFromFile(const std::string& filePath);

	inline const std::vector<std::vector<RGB>>& getShCoefficients() const { return this->shCoefficients; }
};