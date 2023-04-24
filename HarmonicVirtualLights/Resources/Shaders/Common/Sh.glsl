
vec3 getIndirectLight(uint rsmSize)
{
	vec3 color = vec3(0.0f);

	for(uint y = 0; y < rsmSize; ++y)
	{
		for(uint x = 0; x < rsmSize; ++x)
		{
			vec2 uv = (vec2(float(x), float(y)) + vec2(0.5f)) / float(rsmSize);
		}
	}

	return color;
}