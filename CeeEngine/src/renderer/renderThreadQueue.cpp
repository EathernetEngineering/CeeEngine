/* Implementation of function submission queue.
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

#include <CeeEngine/renderer/renderThreadQueue.h>

#include <CeeEngine/assert.h>

namespace cee {
	RenderThreadQueue::RenderThreadQueue() {
		m_BufferBase = reinterpret_cast<uint8_t*>(malloc(100 * 1024 * 1024));
		m_BufferPtr = m_BufferBase;
		m_BufferSize = 4 * 1024 * 1024;
		m_SubmissionCount = 0;
	}

	RenderThreadQueue::RenderThreadQueue::~RenderThreadQueue() {
		free(m_BufferBase);
	}

	void* RenderThreadQueue::Submit(PFN_RenderCommand function, size_t size) {
		if ((size + sizeof(size_t) + sizeof(PFN_RenderCommand)) > (m_BufferSize - (m_BufferPtr - m_BufferBase))) {
			// TODO: Add reallocation?
			CEE_VERIFY(false, "RenderThreadQueue Overflow");
		}

		*(reinterpret_cast<PFN_RenderCommand*>(m_BufferPtr)) = function;
		m_BufferPtr += sizeof(PFN_RenderCommand);
		*(reinterpret_cast<size_t*>(m_BufferPtr)) = size;
		m_BufferPtr += sizeof(size_t) + size;
		m_SubmissionCount++;

		return m_BufferPtr - size;
	}

	void RenderThreadQueue::Execute() {
		size_t blockSize;
		uint8_t* ptr = m_BufferBase;
		PFN_RenderCommand function;
		while (m_SubmissionCount != 0) {
			function = *reinterpret_cast<PFN_RenderCommand*>(ptr);
			ptr += sizeof(PFN_RenderCommand);

			blockSize = *reinterpret_cast<size_t*>(ptr);
			ptr += sizeof(size_t);

			function(ptr);

			ptr += blockSize;

			m_SubmissionCount--;
		}
		m_BufferPtr = m_BufferBase;
	}
}
