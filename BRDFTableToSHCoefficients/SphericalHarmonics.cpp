#include "SphericalHarmonics.h"
#include "BRDFRead.h"

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

// Normalization factor
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