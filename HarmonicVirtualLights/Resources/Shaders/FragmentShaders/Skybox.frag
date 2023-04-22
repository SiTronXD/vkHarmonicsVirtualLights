#version 450

layout(set = 0, binding = 9) uniform samplerCube cubeSampler;

layout(location = 0) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

vec3 srgbToLinear(vec3 col)
{
	return pow(col, vec3(2.2f));
}

vec3 linearToSrgb(vec3 col)
{
	return pow(col, vec3(1.0f / 2.2f));
}

void main()
{
	// Sample cube map
	vec3 color = texture(cubeSampler, fragPos, 0.0f).rgb;

	outColor = vec4(color, 1.0f);
}