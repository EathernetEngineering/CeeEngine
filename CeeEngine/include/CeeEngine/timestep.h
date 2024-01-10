/* Decleration of time tracker.
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
