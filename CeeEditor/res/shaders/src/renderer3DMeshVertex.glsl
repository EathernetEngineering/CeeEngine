#version 450 core

layout(location = 0) in vec4 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in vec2 a_TexCoords;

layout(location = 0) out vec3 v_Normal;
layout(location = 1) out vec4 v_Color;
layout(location = 2) out vec2 v_TexCoords;

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 projection;
} u_Ubo;

void main() {
	v_Normal = a_Normal;
	v_Color = a_Color;
	v_TexCoords = a_TexCoords;
	gl_Position = a_Position * u_Ubo.view * u_Ubo.projection;
}

