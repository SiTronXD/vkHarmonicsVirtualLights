#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject 
{
	mat4 vp;
	vec4 pos;
} ubo;

layout(push_constant) uniform PushConstantData
{
	mat4 modelMat;
	vec4 materialProperties; // vec4(roughness, metallic, 0, 0)
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out vec3 camPos;
layout(location = 3) out vec2 fragTexCoord;
layout(location = 4) out vec2 materialProperties;

void main()
{
	vec4 worldPos = pc.modelMat * vec4(inPosition, 1.0);

	gl_Position = ubo.vp * worldPos;
	fragNormal = inNormal;
	fragWorldPos = worldPos.xyz;
	camPos = ubo.pos.xyz;
	fragTexCoord = inTexCoord;
	materialProperties = pc.materialProperties.xy;
}