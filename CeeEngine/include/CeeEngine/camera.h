#ifndef _CEE_ENGINE_CAMERA_H
#define _CEE_ENGINE_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace cee {
	class Camera {
	public:
		Camera() = default;
		Camera(const glm::mat4& projection);

	public:
		glm::mat4 GetProjection() { return m_Projection; }
		const glm::mat4& GetProjection() const { return m_Projection; }

	protected:
		glm::vec3 m_Postition = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_Rotation = { 0.0f, 0.0f, 0.0f };

		glm::mat4 m_Projection;
	};

	class OrthographicCamera : public Camera {
	public:
		OrthographicCamera(float left, float right, float bottom, float top, float nearZ, float farZ);
		OrthographicCamera(float zoom, float aspectRatio, float nearZ, float farZ);

	private:
		float m_Zoom;
	};

	class PerspectiveCamera : public Camera {
	public:
		PerspectiveCamera(float fov = 60.0f, float aspectRatio = 1.778f, float nearZ = 0.001f, float farZ = 256.0f);

		void SetFov(float newFov);
		float GetFov() const { return m_Fov; }

	private:
		float m_Fov, m_AspectRatio, m_NearZ, m_FarZ;
	};
}

#endif
