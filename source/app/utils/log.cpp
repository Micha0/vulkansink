#include "log.h"

std::string ToHex(std::string in)
{
	const char* inPtr = in.c_str();
	char buffer [in.size()*3 + 1];
	buffer[in.size()*3] = 0;
	for(int j = 0; j < in.size(); j++)
		sprintf(&buffer[3*j], "%02X ", inPtr[j]);

	return buffer;
}

std::string ToHex(char* in, int size)
{
	std::string theString(in, size);
	return ToHex(theString);
}