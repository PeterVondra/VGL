#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// This shader performs downsampling on a texture,
// as taken from Call Of Duty method, presented at ACM Siggraph 2014.
// This particular method was customly designed to eliminate
// "pulsating artifacts and temporal stability issues".

// Remember to add bilinear minification filter for this texture!
// Remember to use a floating-point texture format (for HDR)!
// Remember to use edge clamping for this texture!
layout(binding = 0) uniform sampler2D src_image;

layout(location = 0) in vec2 UV;
layout(location = 0) out vec4 downsample;

void main()
{
    vec2 srcTexelSize = 1.0 / textureSize(src_image, 0);
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===
    vec3 a = texture(src_image, vec2(UV.x - 2*x, UV.y + 2*y)).rgb;
    vec3 b = texture(src_image, vec2(UV.x,       UV.y + 2*y)).rgb;
    vec3 c = texture(src_image, vec2(UV.x + 2*x, UV.y + 2*y)).rgb;

    vec3 d = texture(src_image, vec2(UV.x - 2*x, UV.y)).rgb;
    vec3 e = texture(src_image, vec2(UV.x,       UV.y)).rgb;
    vec3 f = texture(src_image, vec2(UV.x + 2*x, UV.y)).rgb;

    vec3 g = texture(src_image, vec2(UV.x - 2*x, UV.y - 2*y)).rgb;
    vec3 h = texture(src_image, vec2(UV.x,       UV.y - 2*y)).rgb;
    vec3 i = texture(src_image, vec2(UV.x + 2*x, UV.y - 2*y)).rgb;

    vec3 j = texture(src_image, vec2(UV.x - x, UV.y + y)).rgb;
    vec3 k = texture(src_image, vec2(UV.x + x, UV.y + y)).rgb;
    vec3 l = texture(src_image, vec2(UV.x - x, UV.y - y)).rgb;
    vec3 m = texture(src_image, vec2(UV.x + x, UV.y - y)).rgb;

    // Apply weighted distribution:
    // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
    // a,b,d,e * 0.125
    // b,c,e,f * 0.125
    // d,e,g,h * 0.125
    // e,f,h,i * 0.125
    // j,k,l,m * 0.5
    // This shows 5 square areas that are being sampled. But some of them overlap,
    // so to have an energy preserving downsample we need to make some adjustments.
    // The weights are the distributed, so that the sum of j,k,l,m (e.g.)
    // contribute 0.5 to the final color output. The code below is written
    // to effectively yield this sum. We get:
    // 0.125*5 + 0.03125*4 + 0.0625*4 = 1
    downsample.rgb = e*0.125;
    downsample.rgb += (a+c+g+i)*0.03125;
    downsample.rgb += (b+d+f+h)*0.0625;
    downsample.rgb += (j+k+l+m)*0.125;
    downsample.a = 1.0f;

    downsample = max(downsample, 0.0001f);
}