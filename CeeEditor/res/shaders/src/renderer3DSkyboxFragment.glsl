#version 450

layout(location = 0) in vec3 v_EyeDirection;

layout(location = 0) out vec4 color;

layout(set = 0, binding = 1) uniform samplerCube u_CubeMap;

void main() {
	color = texture(u_CubeMap, v_EyeDirection);
}
