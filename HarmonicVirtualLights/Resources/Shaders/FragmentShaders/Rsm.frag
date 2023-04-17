#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out uvec4 outBrdfIndex;

void main()
{
	vec3 normal = normalize(fragNormal);

	outPosition = vec4(fragWorldPos, 1.0f);
	outNormal = vec4(normal, 1.0f);
	outBrdfIndex = uvec4(59u, 142u, 235u, 255u);
}