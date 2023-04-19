#pragma once

struct RGB
{
	double r = 0.0;
	double g = 0.0;
	double b = 0.0;

	void operator+=(const RGB& other)
	{
		this->r += other.r;
		this->g += other.g;
		this->b += other.b;
	}

	void operator/=(const double& value)
	{
		this->r /= value;
		this->g /= value;
		this->b /= value;
	}

	RGB operator*(const double& value) const
	{
		RGB newRGB{};
		newRGB.r = this->r * value;
		newRGB.g = this->g * value;
		newRGB.b = this->b * value;

		return newRGB;
	}

	RGB operator/(const double& value) const
	{
		RGB newRGB{};
		newRGB.r = this->r / value;
		newRGB.g = this->g / value;
		newRGB.b = this->b / value;

		return newRGB;
	}
};

int factorial(int v);

double P(int l, int m, double x);
double K(int l, int m);
double y(int l, int m, double cosTheta, double phi);