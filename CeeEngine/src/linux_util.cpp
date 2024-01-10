/* Implementation of general utilities specific to linux.
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

#include <CeeEngine/util.h>

#include <unistd.h>
#include <fcntl.h>

namespace cee {
	namespace util {
		int ReadFile(const std::string& filepath, std::vector<uint8_t>& data) {
			int file = open(filepath.c_str(), O_RDONLY);
			if (file < 0)
				return errno;

			off_t fileSize = lseek(file, 0, SEEK_END);
			if (fileSize < 0)
				return errno;
			lseek(file, 0, SEEK_SET);

			data.clear();
			data.resize(fileSize);

			int readBytes = read(file, data.data(), fileSize);
			if (readBytes < 0)
				return errno;

			close(file);

			return readBytes;
		}
	}
}
