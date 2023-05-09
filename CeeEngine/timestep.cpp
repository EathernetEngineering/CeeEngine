#include <CeeEngine/CeeTimestep.h>
#include <CeeEngine/CeeEnginePlatform.h>

#if defined(CEE_PLATFORM_WINDOWS)
#include <Windows.h>
#elif defined(CEE_PLATFORM_LINUX)
#include <time.h>
#endif

namespace cee {
	void GetTime(Timestep *t) {
	timespec ts;
#if defined(CEE_PLATFORM_WINDOWS)
	timespec_get(&ts, TIME_UTC);
#elif defined(CEE_PLATFORM_LINUX)
	clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
	t->sec = ts.tv_sec;
	t->nsec = ts.tv_nsec;

	}

	void GetTimeStep(cee::Timestep* start, cee::Timestep* end, Timestep *result)
	{
		if (start->nsec > end->nsec) {
			result->nsec = (end->nsec + 1000000000) - start->nsec;
			result->sec = end->sec - start->sec + 1;
			return;
		}
		result->nsec = end->nsec - start->nsec;
		result->sec = end->sec - start->sec;
	}

}
