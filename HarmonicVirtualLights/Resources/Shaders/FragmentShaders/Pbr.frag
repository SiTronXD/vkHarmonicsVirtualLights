#version 450

#extension GL_GOOGLE_include_directive: require

#include "../Common/Sh.glsl"
#include "../Common/ColorTransformations.glsl"

#define MAX_REFLECTION_LOD (8.0f - 1.0f)
#define SHADOW_BIAS 0.003f
#define SHADOW_MAP_SIZE 256.0f

layout(binding = 1) uniform LightCamUBO 
{
	mat4 vp;
	vec4 pos; // (x, y, z, shadow map size)
} lightCamUbo;

layout(binding = 2) uniform sampler2D brdfLutTex;
layout(binding = 3) uniform samplerCube prefilterMap;

layout(binding = 9) uniform sampler2D albedoTex;
layout(binding = 10) uniform sampler2D roughnessTex;
layout(binding = 11) uniform sampler2D metallicTex;


layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 camPos;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 4) in vec2 materialProperties;
layout(location = 5) flat in uint fragBrdfIndex;

layout(location = 0) out vec4 outColor;

// "An Efficient Representation for Irradiance Environment Maps"
// Ravi Ramamoorthi, Pat Hanrahan
vec3 getShIrradiance(vec3 normal)
{
	const float c1 = 0.429043;
	const float c2 = 0.511664;
	const float c3 = 0.743125;
	const float c4 = 0.886227;
	const float c5 = 0.247708;

	// Grace cathedral
	const vec3 L00 = vec3(0.79f, 0.44f, 0.54f);

	const vec3 L1m1 = vec3(0.39f, 0.35f, 0.60f);
	const vec3 L10 = vec3(-0.34f, -0.18f, -0.27f);
	const vec3 L11 = vec3(-0.29f, -0.06f, 0.01f);

	const vec3 L2m2 = vec3(-0.11f, -0.05f, -0.12f);
	const vec3 L2m1 = vec3(-0.26f, -0.22f, -0.47f);
	const vec3 L20 = vec3(-0.16f, -0.09f, -0.15f);
	const vec3 L21 = vec3(0.56f, 0.21f, 0.14f);
	const vec3 L22 = vec3(0.21f, -0.05f, -0.30f);

	// Eucalyptus grove
	/*const vec3 L00 = vec3(0.38f, 0.43f, 0.45f);

	const vec3 L1m1 = vec3(0.29f, 0.36f, 0.41f);
	const vec3 L10 = vec3(0.04f, 0.03f, 0.01f);
	const vec3 L11 = vec3(-0.10f, -0.10f, -0.09f);

	const vec3 L2m2 = vec3(-0.06f, -0.06f, -0.04f);
	const vec3 L2m1 = vec3(0.01f, -0.01f, -0.05f);
	const vec3 L20 = vec3(-0.09f, -0.13f, -0.15f);
	const vec3 L21 = vec3(-0.06f, -0.05f, -0.04f);
	const vec3 L22 = vec3(0.02f, 0.00f, -0.05f);*/

	float x = normal.x;
	float y = normal.y;
	float z = normal.z;

	return c1 * L22 * (x*x - y*y) + c3 * L20 * z*z + c4*L00 - c5*L20 + 
		2.0f * c1 * (L2m2*x*y + L21*x*z + L2m1*y*z) + 
		2.0f * c2 * (L11*x + L1m1*y + L10*z);
}

float distributionGGX(vec3 N, vec3 H, float roughness)
{
	//float a = roughness; // (Epic games recommends squaring roughness)
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0f);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
	denom = PI * denom * denom;

	return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0f);
	float k = r * r / 8.0f; // (k value for direct light)

	float num = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0f);
	float NdotL = max(dot(N, L), 0.0f);
	float ggx1 = geometrySchlickGGX(NdotV, roughness);
	float ggx2 = geometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0f - roughness), F0) - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

float getShadowFactor()
{
	// Transform to light's coordinate space
	vec4 lightWorldPos = lightCamUbo.vp * vec4(fragWorldPos, 1.0f);
	lightWorldPos /= lightWorldPos.w;
	lightWorldPos.y = -lightWorldPos.y;
	lightWorldPos.xy = lightWorldPos.xy * 0.5f + vec2(0.5f);

	// Sample depth texture
	const float oneOverSize = 1.0f / SHADOW_MAP_SIZE;
	vec2 smPos = lightWorldPos.xy * SHADOW_MAP_SIZE;
	vec2 fractPos = fract(smPos);
	vec2 corner0 = (floor(smPos) + vec2(0.5f)) / SHADOW_MAP_SIZE;
	float lightDepth0 = texture(shadowMapTex, corner0).r;
	float lightDepth1 = texture(shadowMapTex, corner0 + vec2(oneOverSize, 0.0f)).r;
	float lightDepth2 = texture(shadowMapTex, corner0 + vec2(0.0f, oneOverSize)).r;
	float lightDepth3 = texture(shadowMapTex, corner0 + vec2(oneOverSize, oneOverSize)).r;
	float result0 = lightWorldPos.z - SHADOW_BIAS <= lightDepth0 ? 1.0f : 0.0f;
	float result1 = lightWorldPos.z - SHADOW_BIAS <= lightDepth1 ? 1.0f : 0.0f;
	float result2 = lightWorldPos.z - SHADOW_BIAS <= lightDepth2 ? 1.0f : 0.0f;
	float result3 = lightWorldPos.z - SHADOW_BIAS <= lightDepth3 ? 1.0f : 0.0f;

	float horizResult0 = mix(result0, result1, fractPos.x);
	float horizResult1 = mix(result2, result3, fractPos.x);
	float finalResult = mix(horizResult0, horizResult1, fractPos.y);

	return finalResult;
}

void main()
{
	// Can be replaced by a buffer with multiple lights
	const uint numLights = 1;
	const vec3 lightPos = lightCamUbo.pos.xyz;
	const vec3 lightColor = vec3(1.0f, 1.0f, 1.0f) * 10.0f;

	const float ao = 1.0f;

	vec3 N = normalize(fragNormal);
	vec3 V = normalize(camPos - fragWorldPos);
	/*vec3 R = reflect(-V, N);
	vec3 albedo = srgbToLinear(texture(albedoTex, fragTexCoord).rgb);
	float roughness = texture(roughnessTex, fragTexCoord).r * materialProperties.x;
	float metallic = texture(metallicTex, fragTexCoord).r * materialProperties.y;
	vec3 F0 = vec3(0.04f);
	F0 = mix(F0, albedo, metallic);

	// Radiance per light
	vec3 Lo = vec3(0.0f);
	for(uint i = 0; i < numLights; ++i)
	{
		vec3 toLight = lightPos - fragWorldPos;
		vec3 L = normalize(toLight);
		vec3 H = normalize(V + L);
		float attenuation = 1.0f / dot(toLight, toLight);
		vec3 radiance = lightColor * attenuation;

		// Cook-torrance BRDF
		float NDF = distributionGGX(N, H, roughness);
		float G = geometrySmith(N, V, L, roughness);
		vec3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0);

		// Specular/Diffuse ratio based on fresnel
		vec3 kS = F;
		vec3 kD = vec3(1.0f) - kS;
		kD *= 1.0f - metallic;
		
		vec3 diffuse = kD * albedo / PI;

		float NdotL = max(dot(N, L), 0.0f);
		vec3 numerator = NDF * G * F;
		float denominator = 4.0f * max(dot(N, V), 0.0f) * NdotL + 0.0001f;
		vec3 specular = numerator / denominator; // (already contains kS as F)

		// Add to outgoing radiance Lo
		Lo += (diffuse + specular) * radiance * NdotL * getShadowFactor();
	}

	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0f), F0, roughness);
	vec3 indirectKs = F;
	vec3 indirectKd = 1.0f - indirectKs;
	indirectKd *= 1.0f - metallic;

	// Diffuse
	vec3 irradiance = getShIrradiance(N);
	vec3 diffuse = irradiance * albedo;

	// Specular
	vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
	vec2 envBrdf = texture(brdfLutTex, vec2(max(dot(N, V), 0.0f), roughness)).rg;
	vec3 specular = prefilteredColor * (F * envBrdf.x + envBrdf.y);
	
	// Add to ambient
	vec3 ambient = (indirectKd * diffuse + specular) * ao;

	// No IBL
	ambient = vec3(0.0f);

	vec3 color = ambient + Lo;*/

	vec3 color = getIndirectLight(fragTexCoord, fragWorldPos, lightPos, N, V, uint(lightCamUbo.pos.w), fragBrdfIndex);
	color += getDirectLight(fragWorldPos, lightPos, N, V, fragBrdfIndex) * getShadowFactor();

	outColor = vec4(color, 1.0f);
}