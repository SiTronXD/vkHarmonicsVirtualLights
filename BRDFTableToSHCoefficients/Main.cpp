#include <vector>

#include "BRDFRead.h"

struct RGB
{
	double r = 0.0;
	double g = 0.0;
	double b = 0.0;
};

int factorial(int v)
{
	if (v > 12)
	{
		printf("Factorial of %i cannot be calculated, as the result is too large.\n", v);
	}

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
double P(int l, int m, double x)
{
    double pmm = 1.0;

    if (m > 0)
    {
		double somx2 = sqrt((1.0 - x) * (1.0 + x));
		double fact = 1.0;
        for (int i = 1; i <= m; ++i)
        {
            pmm *= -fact * somx2;
            fact += 2.0;
        }
    }

    if (l == m)
        return pmm;

	double pmmp1 = x * (2.0 * double(m) + 1.0) * pmm;
    if (l == m + 1)
        return pmmp1;

	double pll = 0.0;
    for (int ll = m + 2; ll <= l; ++ll)
    {
        pll = ((2.0 * double(ll) - 1.0) * x * pmmp1 - double(ll + m - 1) * pmm) / double(ll - m);
        pmm = pmmp1;
        pmmp1 = pll;
    }

    return pll;
}

// Normalization constant
double K(int l, int m)
{
    double numer = double((2 * l + 1) * factorial(l - abs(m)));
    double denom = 4.0 * M_PI * double(factorial(l + abs(m)));
    return sqrt(numer / denom);
}

// SH basis function.
// Explanation: https://www.ppsloan.org/publications/StupidSH36.pdf
double y(int l, int m, double cosTheta, double phi)
{
    // Zonal harmonics as base case where m = 0
	double v = K(l, m);
	v *= P(l, abs(m), cosTheta);

    // Remaining cases
    if (m != 0)
        v *= sqrt(2.0);
    if (m > 0)
        v *= cos(double(m) * phi);
    if (m < 0)
        v *= sin(double(abs(m)) * phi);

    return v;
}

void sampleHemisphere(double* brdf, double theta_in, double phi_in, double theta_out, double phi_out, double& outputRed, double& outputGreen, double& outputBlue)
{
	lookup_brdf_val(brdf, theta_in, phi_in, theta_out, phi_out, outputRed, outputGreen, outputBlue);

	// Unit sphere (sampled as a hemisphere)
	/*outputRed = 1.0;
	outputGreen = 1.0;
	outputBlue = 1.0;*/
}

void getShError(double* brdf, int n, int NUM_SH_COEFFS, int k, const std::vector<RGB>& shCoefficients)
{
	int numErrorSamples = 0;
	double errorR = 0.0f;
	double errorG = 0.0f;
	double errorB = 0.0f;
	double phi_out = 0.0; // Assuming isotropic materials
	{
		// theta_out: [0, PI/2)
		double theta_out = k * 0.5 * M_PI / n;

		// n x 4n table of BRDF values
		for (int i = 0; i < n; i++)
		{
			// theta_in: [0, PI/2)
			double theta_in = i * 0.5 * M_PI / n;

			for (int j = 0; j < 4 * n; j++)
			{
				// phi_in: [0, 2*PI)
				double phi_in = j * 2.0 * M_PI / (4 * n);

				// Get BRDF sample values
				double red = 0.0;
				double green = 0.0;
				double blue = 0.0;
				sampleHemisphere(brdf, theta_in, phi_in, theta_out, phi_out, red, green, blue);

				// Reconstruct BRDF from SH coefficients
				double shValueR = 0.0;
				double shValueG = 0.0;
				double shValueB = 0.0;

				int l = 0;
				int m = 0;
				for (int sh = 0; sh < NUM_SH_COEFFS; ++sh)
				{
					// SH basis function
					double yFunc = y(l, m, cos(theta_in), phi_in);
					shValueR += yFunc * shCoefficients[sh].r;
					shValueG += yFunc * shCoefficients[sh].g;
					shValueB += yFunc * shCoefficients[sh].b;

					// Next SH coefficient
					m++;
					if (m > l)
					{
						l++;
						m = -l;
					}
				}

				// Add mean squared errors
				errorR += pow(red - shValueR, 2);
				errorG += pow(green - shValueG, 2);
				errorB += pow(blue - shValueB, 2);
				numErrorSamples++;

				// Sample top of hemisphere once
				if (i == 0) j = 4 * n;
			}
		}
	}

	errorR /= double(numErrorSamples);
	errorG /= double(numErrorSamples);
	errorB /= double(numErrorSamples);

	// Print errors
	printf("Error from SH: (r: %f, g: %f, b: %f)\n", errorR, errorG, errorB);
}

int main(int argc, char* argv[])
{
	const char* filename = argv[1];
	double* brdf;

	// Read brdf file
	if (!read_brdf(filename, brdf))
	{
		fprintf(stderr, "Error reading %s\n", filename);
		exit(1);
	}

	// Sample brdf
	/*{
		const int n = 16;
		double phi_out = 0.0f;

		double theta_out = 0.0f;
		
		for (int i = 0; i < n; i++)
		{
			// theta_in: [0, PI/2)
			double theta_in = i * 0.5 * M_PI / n;

			for (int j = 0; j < 4 * n; j++)
			{
				// phi_in: [0, 2*PI)
				double phi_in = j * 2.0 * M_PI / (4 * n);

				// Get sample values
				double red = 0.0;
				double green = 0.0;
				double blue = 0.0;
				lookup_brdf_val(brdf, theta_in, phi_in, theta_out, phi_out, red, green, blue);

				printf("theta_in: %f    phi_in: %f    brdf: (%f, %f, %f)\n", theta_in, phi_in, red, green, blue);
			}
		}

		getchar();
		return 0;
	}*/

	const int n = 16;
	const int MAX_L = 6;
	const int NUM_SH_COEFFS = (MAX_L + 1) * (MAX_L + 1);

	// shCoefficients[wo][1D lm-index]
	std::vector<std::vector<RGB>> shCoefficients(n, std::vector<RGB>(NUM_SH_COEFFS));

	// Create SH coefficients for each element in F
	for (int k = 0; k < n; k++)
	//for (int k = n/2; k <= n/2; k++)
	{
		// theta_out: [0, PI/2)
		double theta_out = k * 0.5 * M_PI / n;

		int l = 0;
		int m = 0;
		for (int sh = 0; sh < NUM_SH_COEFFS; ++sh)
		{
			RGB avgRGB{};
			int numSamples = 0;

			// n x 4n table of BRDF values
			for (int i = 0; i < n; i++)
			{
				// theta_in: [0, PI/2)
				double theta_in = i * 0.5 * M_PI / n;

				for (int j = 0; j < 4 * n; j++)
				{
					// phi_in: [0, 2*PI)
					double phi_in = j * 2.0 * M_PI / (4 * n);
					
					double phi_out = 0.0; // Assuming isotropic materials

					// Get sample values
					double red = 0.0;
					double green = 0.0;
					double blue = 0.0;
					sampleHemisphere(brdf, theta_in, phi_in, theta_out, phi_out, red, green, blue);

					// SH basis function
					double yFunc = y(l, m, cos(theta_in), phi_in);

					avgRGB.r += red * yFunc * sin(theta_in);
					avgRGB.g += green * yFunc * sin(theta_in);
					avgRGB.b += blue * yFunc * sin(theta_in);

					numSamples++;

					// Sample top of hemisphere once
					if (i == 0) j = 4 * n;
				}
			}

			// Assign SH coefficients
			avgRGB.r = (avgRGB.r / double(numSamples)) * 3.0 * M_PI; // pdf(wi) = 1 / (2 * PI) => 1 / pdf(wi) = 2 * PI
			avgRGB.g = (avgRGB.g / double(numSamples)) * 3.0 * M_PI;
			avgRGB.b = (avgRGB.b / double(numSamples)) * 3.0 * M_PI;

			shCoefficients[k][sh] = avgRGB;
			/*if (m == 0)
			{
				shCoefficients[k][sh] = avgRGB;
			}
			else
			{
				avgRGB.r *= sqrt(2.0);
				avgRGB.g *= sqrt(2.0);
				avgRGB.b *= sqrt(2.0);

				if (m < 0)
				{
					avgRGB.r *= pow(-1.0, m);
					avgRGB.g *= pow(-1.0, m);
					avgRGB.b *= pow(-1.0, m);
				}

				shCoefficients[k][sh] = avgRGB;
			}*/

			// Next SH coefficient
			m++;
			if (m > l)
			{
				l++;
				m = -l;
			}
		}

		// Validate
		printf("[k: %i/%i]   ", k, n);
		getShError(brdf, n, NUM_SH_COEFFS, k, shCoefficients[k]);
	}
	
	// Print final coefficients
	/*printf("const SHVector F[NUM_ANGLES] = SHVector[NUM_ANGLES](\n");
	for (size_t i = 0; i < shCoefficients.size(); ++i)
	{
		const std::vector<RGB>& coeffs = shCoefficients[i];

		printf("\tSHVector(vec3[NUM_SH_COEFFS](\n");
		for (size_t j = 0; j < coeffs.size(); ++j)
		{
			printf("\t\tvec3(%f, %f, %f)", coeffs[j].r, coeffs[j].g, coeffs[j].b);
			if (j != coeffs.size() - 1)
			{
				printf(",");
			}
			printf("\n");
		}
		printf("\t))");

		if (i != shCoefficients.size() - 1)
		{
			printf(",\n");
		}
	}
	printf(");\n");*/

	getchar();

	return 0;
}