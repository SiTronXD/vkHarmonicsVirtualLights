#pragma once

class StrHelper
{
public:
	static void splitString(
		std::string& str, 
		char delim, 
		std::vector<std::string>& output);

	static std::string toTimingStr(float x);
};