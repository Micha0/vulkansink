#pragma once

#ifdef USE_PROFILER
#include "communications/socketclient.h"
#include <string>
#include <memory>

// boiler-plate
#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)
#define MAKE_UNIQUE(x) CONCATENATE(x, __COUNTER__)

#define PROFILE_DETAIL(a, b) std::unique_ptr<Profiler::ScopeProfiler> MAKE_UNIQUE(a) = Profiler::ProfileScope(b);
#define PROFILE PROFILE_DETAIL(prof_var, __PRETTY_FUNCTION__)
#define PROFILE_CUST(a) PROFILE_DETAIL(CONCATENATE_DETAIL(prof_var, a), a)

namespace Profiler
{
	struct ScopeProfiler
	{
		int m_ProfileId;

		ScopeProfiler(std::string scope);
		~ScopeProfiler();
	};

	void Initialize();
	void Destroy();

	std::unique_ptr<ScopeProfiler> ProfileScope(std::string name);

};
#else
#define PROFILE (void)0
#define PROFILE_CUST(a) (void)0
namespace Profiler
{
    static void Initialize()
    {};
}
#endif