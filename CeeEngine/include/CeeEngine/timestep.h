#ifndef CEE_ENGINE_TIMESTEP_H
#define CEE_ENGINE_TIMESTEP_H

#include <ctime>
#include <cstdint>

namespace cee {
struct Timestep {
	time_t sec;
	uint64_t nsec;
};

void GetTime(Timestep *t);
void GetTimeStep(Timestep *start, Timestep *end, Timestep *result);
}

#endif
