#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;

layout(location = 0) out vec2 UV;
layout(location = 1) out vec2 BLURSCALE;

layout(push_constant) uniform pushConstant {
    vec2 blurScale;
} push;

void main()
{
	gl_Position = vec4(inPosition, 1.0f);
	UV = inUv;
	BLURSCALE = push.blurScale;
}