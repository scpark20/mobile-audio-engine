// IIRFilter.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "IIRFilter.h"
#include "LPF.h"

IIRFilter::IIRFilter()
{

}

IIRFilter::IIRFilter(float a0, float a1, float a2, float b1, float b2)
{
	this->a0 = a0;
	this->a1 = a1;
	this->a2 = a2;

	this->b1 = b1;
	this->b2 = b2;

	this->buf1 = 0;
	this->buf2 = 0;
}

void IIRFilter::setParams(float a0, float a1, float a2, float b1, float b2)
{
	this->a0 = a0;
	this->a1 = a1;
	this->a2 = a2;

	this->b1 = b1;
	this->b2 = b2;

	this->buf1 = 0;
	this->buf2 = 0;
}

float IIRFilter::getOutput(float input)
{
	float acc = b2 * buf2;
	acc += b1 * buf1;
	acc += input;

	float output = a2 * buf2;
	output += a1 * buf1;
	output += a0 * acc;

	buf2 = buf1;
	buf1 = acc;

	return output;
}



