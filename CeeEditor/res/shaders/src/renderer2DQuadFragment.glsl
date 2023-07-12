#version 450 core

layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_TexCoords;

layout(location = 0) out vec4 color;

layout(set = 1, binding = 0) uniform sampler u_ImageSampler;
layout(set = 1, binding = 1) uniform texture2D u_Image;


void main() {
	vec4 outColor = v_Color * texture(sampler2D(u_Image, u_ImageSampler), v_TexCoords);
	color = outColor;
}
