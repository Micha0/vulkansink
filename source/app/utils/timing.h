/* Timers .. the C++11 way ..
 *
 * Simple stopwatch-like functionality
 */
#pragma once
#include <chrono>
#include <memory>

namespace Timing
{
	class Timewatch
	{
		std::chrono::high_resolution_clock::time_point then;
	public:
		Timewatch();
		double GetNanoseconds();
	};

	std::shared_ptr<Timewatch> Start();
}

typedef std::shared_ptr<Timing::Timewatch> Timewatcher;