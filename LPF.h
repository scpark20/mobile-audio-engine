#pragma once
#define _USE_MATH_DEFINES
#include "IIRFilter.h"
#include <math.h>

class LPF : IIRFilter
{
public:
	LPF(float centerFreq, float samplingRate, float Q) : IIRFilter()
	{
		float theta = 2 * M_PI * centerFreq / samplingRate;
		float d = 1 / Q;
		float beta = 0.5 * (1 - d / 2 * sin(theta)) / (1 + d / 2 * sin(theta));
		float gamma = (0.5 + beta) * cos(theta);
		a0 = (0.5 + beta - gamma) / 2.0;
		a1 = 0.5 + beta - gamma;
		a2 = (0.5 + beta - gamma) / 2.0;
		b1 = 2 * gamma;
		b2 = -2 * beta;
		setParams(a0, a1, a2, b1, b2);
	}

	float getOutput(float input)
	{
		return IIRFilter::getOutput(input);
	}

};
