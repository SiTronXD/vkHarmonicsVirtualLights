#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define SQRT_PI 1.7724538509055160272981674833411
#define SQRT_TWO 1.4142135623730950488016887242097

#define RSM_FOV (HALF_PI)

#define NUM_ANGLES 16
#define MAX_L 6
#define CONVOLUTION_L 4
#define NUM_SH_COEFFICIENTS ((MAX_L + 1) * (MAX_L + 1))
#define NUM_SHADER_SH_COEFFS (NUM_SH_COEFFICIENTS * 3)
struct SHData
{
	float coeffs[((NUM_SHADER_SH_COEFFS + 3) / 4) * 4];
};
layout(binding = 4) readonly buffer SHCoefficientsBuffer
{
	SHData coefficientSets[];
} shCoefficients;

layout(binding = 5) uniform sampler2D rsmDepthTex;
layout(binding = 6) uniform sampler2D rsmPositionTex;
layout(binding = 7) uniform sampler2D rsmNormalTex;
layout(binding = 8) uniform sampler2D rsmBRDFIndexTex;

int factorial(int v)
{
    int t = 1;
    for (int i = 2; i <= v; ++i)
    {
        t *= i;
    }
    return t;
}

// Associated Legendre Polynomial P(l,m,x) at x
// For more, see “Numerical Methods in C: The Art of Scientific Computing”, Cambridge University Press, 1992, pp 252-254" 
// Further explanation: https://www.cse.chalmers.se/~uffe/xjobb/Readings/GlobalIllumination/Spherical%20Harmonic%20Lighting%20-%20the%20gritty%20details.pdf
float P(int l, int m, float x)
{
    float pmm = 1.0;

    if (m > 0)
    {
        float somx2 = sqrt((1.0 - x) * (1.0 + x));
        float fact = 1.0;
        for (int i = 1; i <= m; ++i)
        {
            pmm *= -fact * somx2;
            fact += 2.0;
        }
    }

    if (l == m)
        return pmm;

    float pmmp1 = x * (2.0 * float(m) + 1.0) * pmm;
    if (l == m + 1)
        return pmmp1;

    float pll = 0.0;
    for (int ll = m + 2; ll <= l; ++ll)
    {
        pll = ((2.0 * float(ll) - 1.0) * x * pmmp1 - float(ll + m - 1) * pmm) / float(ll - m);
        pmm = pmmp1;
        pmmp1 = pll;
    }

    return pll;
}

// Normalization factor
float K(int l, int m)
{
    float numer = float((2 * l + 1) * factorial(l - abs(m)));
    float denom = 4.0 * PI * float(factorial(l + abs(m)));
    return sqrt(numer / denom);
}

// SH basis function.
// Explanation: https://www.ppsloan.org/publications/StupidSH36.pdf
float y(int l, int m, float cosTheta, float phi)
{
    // Zonal harmonics as base case where m = 0
    float v = K(l, m);
    v *= P(l, abs(m), cosTheta);

    // Remaining cases
    if (m != 0)
        v *= sqrt(2.0);
    if (m > 0)
        v *= cos(float(m) * phi);
    if (m < 0)
        v *= sin(float(abs(m)) * phi);

    return v;
}

float getCoeffLHat(int l, float alpha)
{
	if(l == 0)
	{
		return SQRT_PI * (1.0 - alpha);
	}
	
	return sqrt(PI / float(2u * l + 1u)) * (P(l - 1, 0, alpha) - P(l + 1, 0, alpha));
}

float getCoeffL(int l, int m, float hvlRadius, float hvlDistance, float cosThetaWLight, float phiWLight)
{
    // TODO: move out terms which repeats within the same band

    // Pythagorean identities
    float sinA = abs(hvlRadius / hvlDistance);
    float alpha = float(sqrt(clamp(1.0f - sinA*sinA, 0.0f, 1.0f)));

    float factor = sqrt(4.0 * PI / float(2u * l + 1u));
    float shBasisFunc = y(l, m, cosThetaWLight, phiWLight);
    float LHat = getCoeffLHat(l, alpha);

    return factor * shBasisFunc * LHat;
}

vec3 getIndirectLight(vec2 texCoord, vec3 worldPos, vec3 lightPos, vec3 normal, vec3 viewDir, uint rsmSize, uint xBrdfIndex)
{
	vec3 color = vec3(0.0f);

    float fRsmSize = float(rsmSize);

    // Tangent vectors
    vec3 tangentRight = normalize(cross(normal, viewDir)); // TODO: fix edge case where normal == viewDir
    vec3 tangentForward = cross(tangentRight, normal);
    mat3x3 worldToTangentMat = transpose(mat3x3(tangentRight, normal, tangentForward));
    
    // Obtain coefficient vector F (with cosine term)
    const float cosAngle = dot(normal, viewDir);
    const float angle = acos(clamp(cosAngle, 0.0f, 1.0f));
    const int angleIndex = clamp(
        int(angle / HALF_PI * float(NUM_ANGLES)), 
        0, 
        NUM_ANGLES - 1
    );
    const SHData F = shCoefficients.coefficientSets[xBrdfIndex * NUM_ANGLES * 2 + NUM_ANGLES + angleIndex];
    
    // Loop through each HVL
	for(uint y = 0; y < rsmSize; ++y)
    //for(uint y = 0; y < 1u; ++y)
	{
		for(uint x = 0; x < rsmSize; ++x)
        //for(uint x = 0; x < 1u; ++x)
		{
			vec2 uv = (vec2(float(x), float(y)) + vec2(0.5f)) / fRsmSize;
            vec3 hvlNormal = texture(rsmNormalTex, uv).rgb;

            // This texel does not contain a valid HVL
            if(dot(hvlNormal, hvlNormal) > 64.0f)
            {
                continue;
            }

            // HVL cache
            vec3 hvlPos = texture(rsmPositionTex, uv).rgb;

            // HVL data
            vec3 wLight = hvlPos - worldPos;
            float d = length(hvlPos - lightPos);
            float gamma = SQRT_TWO * RSM_FOV / fRsmSize;
            float hvlRadius = d * (gamma + (gamma * gamma * gamma / 3.0f)); // Taylor series approximation of tan(x)
            float hvlDistance = length(wLight);
            wLight /= hvlDistance;

            // Relative light direction
            vec3 wLightTangentSpace = worldToTangentMat * wLight;
            float cosThetaWLight = wLightTangentSpace.y;
            float phiWLight = atan(wLightTangentSpace.z, wLightTangentSpace.x);

            // L * F
            vec3 dotLF = vec3(0.0f);
            for(int lSH = 0; lSH <= CONVOLUTION_L; ++lSH)
            {
                for(int mSH = -lSH; mSH <= lSH; ++mSH)
                {
                    int index = lSH * (lSH + 1) + mSH;
                    float Llm = float(getCoeffL(lSH, mSH, hvlRadius, hvlDistance, cosThetaWLight, phiWLight));
                    dotLF += 
                        Llm * vec3(
                            F.coeffs[index * 3 + 0],    // R
                            F.coeffs[index * 3 + 1],    // G
                            F.coeffs[index * 3 + 2]     // B
                        );
                }
            }

            // Add "Lj(L * F)" from each HVL
            color += dotLF;

            // Visualize HVL sizes
            //color += hvlDistance <= hvlRadius ? vec3(0.1f, 0.0f, 0.0f) : vec3(0.0f);
		}
	}

	return color;
}