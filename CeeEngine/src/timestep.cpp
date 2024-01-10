/* Implementation of time tracker.
 *   Copyright (C) 2023-2024  Chloe Eather.
 *
 *   This file is part of CeeEngine.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>. */

#include <CeeEngine/timestep.h>
#include <CeeEngine/platform.h>

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
