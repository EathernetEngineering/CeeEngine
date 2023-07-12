#version 450 core

layout(location = 0) in vec4 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in vec2 a_TexCoords;
layout(location = 4) in uint a_TexIndex;

layout(location = 0) out vec3 v_Normal;
layout(location = 1) out vec4 v_Color;
layout(location = 2) out vec2 v_TexCoords;
layout(location = 3) flat out uint v_TexIndex;

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 projection;
} u_Ubo;

void main() {
	v_Normal = a_Normal;
	v_Color = a_Color;
	v_TexCoords = a_TexCoords;
	v_TexIndex = a_TexIndex;
	gl_Position = u_Ubo.projection * u_Ubo.view * a_Position;
}
