#include "timing.h"

namespace Timing
{
	Timewatch::Timewatch()
	: then(std::chrono::high_resolution_clock::now())
	{
	}

	double Timewatch::GetNanoseconds()
	{
		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::nanoseconds>(now - then).count() / 1000000000.0;
	}

	std::shared_ptr<Timewatch> Start()
	{
		return std::shared_ptr<Timewatch>(new Timewatch());
	}
};