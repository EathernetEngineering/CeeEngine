/* Implementation of camera.
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

#include <CeeEngine/camera.h>

namespace cee {
	Camera::Camera(const glm::mat4& projection)
	: m_Projection(projection), m_Transform(glm::mat4(1.0f))
	{
	}

	void Camera::SetPosition(const glm::vec3& position) {
		m_Postition = position;
		m_Transform[3][0] = m_Postition[0];
		m_Transform[3][1] = m_Postition[1];
		m_Transform[3][2] = m_Postition[2];
	}

	void Camera::Translate(const glm::vec3& translateBy) {
		m_Postition += translateBy;
		m_Transform[3][0] = m_Postition[0];
		m_Transform[3][1] = m_Postition[1];
		m_Transform[3][2] = m_Postition[2];
	}

	void Camera::SetRotation(const glm::vec3& vector) {
		m_Rotation = glm::normalize(vector);
	}

	void Camera::Rotate(float angle, const glm::vec3& vector) {
		glm::vec3 normalisedVector = glm::normalize(vector);
		m_Transform[3][0] = 0.0f;
		m_Transform[3][1] = 0.0f;
		m_Transform[3][2] = 0.0f;
		m_Transform = glm::rotate(m_Transform, angle, normalisedVector);
		m_Rotation = m_Transform * glm::vec4(m_Rotation, 1.0f);
		m_Transform[3][0] = m_Postition[0];
		m_Transform[3][1] = m_Postition[1];
		m_Transform[3][2] = m_Postition[2];
	}

	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top, float nearZ, float farZ)
	: Camera(glm::ortho(left, right, bottom, top, nearZ, farZ))
	{
	}

	OrthographicCamera::OrthographicCamera(float zoom, float aspectRatio, float nearZ, float farZ)
	: OrthographicCamera(-aspectRatio * zoom, aspectRatio * zoom, -zoom, zoom, nearZ, farZ)
	{
	}

	PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio, float nearZ, float farZ)
	: Camera(glm::perspective(glm::radians(fov), aspectRatio, nearZ, farZ)), m_Fov(fov), m_AspectRatio(aspectRatio),
	  m_NearZ(nearZ), m_FarZ(farZ)
	{
		m_Projection[1][1] *= -1.0f;
	}

	void PerspectiveCamera::SetFov(float newFov) {
		m_Fov = newFov;
		m_Projection = glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_NearZ, m_FarZ);
		m_Projection[1][1] *= -1.0f;
	}

	void PerspectiveCamera::SetAspectRatio(float newAspectRatio) {
		m_AspectRatio = newAspectRatio;
		m_Projection = glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_NearZ, m_FarZ);
		m_Projection[1][1] *= -1.0f;
	}

}
