#include <vector>

#include "BRDFRead.h"
#include "Exporter.h"

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
	RGB errorRGB{};
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
				RGB brdfRGB{};
				sampleHemisphere(brdf, theta_in, phi_in, theta_out, phi_out, brdfRGB.r, brdfRGB.g, brdfRGB.b);

				// Reconstruct BRDF from SH coefficients
				RGB shValueRGB{};

				int l = 0;
				int m = 0;
				for (int sh = 0; sh < NUM_SH_COEFFS; ++sh)
				{
					// SH basis function
					double shBasisFunc = y(l, m, cos(theta_in), phi_in);

					shValueRGB += shCoefficients[sh] * shBasisFunc;

					// Next SH coefficient
					m++;
					if (m > l)
					{
						l++;
						m = -l;
					}
				}

				// Add mean squared errors
				errorRGB.r += pow(brdfRGB.r - shValueRGB.r, 2);
				errorRGB.g += pow(brdfRGB.g - shValueRGB.g, 2);
				errorRGB.b += pow(brdfRGB.b - shValueRGB.b, 2);
				numErrorSamples++;

				// Sample top of hemisphere only once
				if (i == 0) j = 4 * n;
			}
		}
	}
	errorRGB /= double(numErrorSamples);

	// Print errors
	printf("[k: %i/%i]   ", k, n);
	printf("Error from SH: (r: %f, g: %f, b: %f)\n", errorRGB.r, errorRGB.g, errorRGB.b);
}

int main(int argc, char* argv[])
{
	const std::string materialFileName = "pink-fabric.binary";
	//const std::string materialFileName = "white-fabric.binary";
	const std::string fileName = "brdfs/" + materialFileName;
	double* brdf;

	// Read brdf file
	if (!read_brdf(fileName.c_str(), brdf))
	{
		fprintf(stderr, "Error reading %s\n", fileName.c_str());
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
					RGB sampleRGB{};
					sampleHemisphere(brdf, theta_in, phi_in, theta_out, phi_out, sampleRGB.r, sampleRGB.g, sampleRGB.b);

					// SH basis function
					double shBasisFunc = y(l, m, cos(theta_in), phi_in);
					double shFactor = shBasisFunc * sin(theta_in);

					// Add sample
					avgRGB += sampleRGB * shFactor;

					numSamples++;

					// Sample top of hemisphere only once
					if (i == 0) j = 4 * n;
				}
			}

			// Assign SH coefficients
			avgRGB = (avgRGB / double(numSamples)) * 2.0 * M_PI; // Hemisphere surface area: 2PI
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
		getShError(brdf, n, NUM_SH_COEFFS, k, shCoefficients[k]);
	}
	
	// Export
	Exporter exporter;
	exporter.writeToFile(shCoefficients, materialFileName);
	//exporter.printAsGLSLArray(shCoefficients);

	getchar();

	return 0;
}