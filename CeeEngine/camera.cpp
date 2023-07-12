#include <CeeEngine/camera.h>

namespace cee {
	Camera::Camera(const glm::mat4& projection)
	: m_Projection(projection)
	{
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
		m_Projection = glm::perspective(glm::radians(newFov), m_AspectRatio, m_NearZ, m_FarZ);
		m_Projection[1][1] *= -1.0f;
	}

}
