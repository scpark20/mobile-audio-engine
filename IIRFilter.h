
#pragma once

class IIRFilter
{

public:
	IIRFilter();
	IIRFilter(float a0, float a1, float a2, float b1, float b2);
	void setParams(float a0, float a1, float a2, float b1, float b2);
	float getOutput(float input);
protected:
	float a0;
	float a1;
	float a2;
	float b1;
	float b2;
	float buf1 = 0;
	float buf2 = 0;
};
