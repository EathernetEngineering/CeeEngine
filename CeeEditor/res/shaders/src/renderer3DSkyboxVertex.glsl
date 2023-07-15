#version 450 core

layout(location = 0) in vec3 a_Position;

layout(location = 0) out vec3 v_EyeDirection;

layout(set = 0, binding = 0) uniform MatricesUbObject {
	mat4 view;
	mat4 projection;
} u_MatricesUbo;

void main() {
	mat4 inverseProjection = inverse(u_MatricesUbo.projection);
	mat3 inverseView = transpose(mat3(u_MatricesUbo.view));
	vec3 unprojected = (inverseProjection * vec4(a_Position, 1.0f)).xyz;
	v_EyeDirection = inverseView * unprojected;

	gl_Position = vec4(a_Position, 1.0f);
}
