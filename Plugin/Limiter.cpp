/*
 * Limiter.cpp
 *
 *  Created on: 2015. 7. 27.
 *      Author: scpark
 */

#include "Limiter.h"


Limiter::Limiter(int sampleRate, int bufferSampleLength, float threshould, float ratio, float kneeWidth /* dB */, float lookupTime /* ms */, float dryWet)
:Plugin(sampleRate, bufferSampleLength)
{
    this->threshould = threshould;
    this->ratio = ratio;
    this->kneeWidth = kneeWidth;
    this->lookupTime = lookupTime;
    this->dryWet = dryWet;

    this->p = threshould - kneeWidth / 2.0f;
    this->r = threshould + kneeWidth / 2.0f;

    this->a1 = (ratio - 1.0f) / (2.0f * (r - p));
    this->b1 = 1.0f - (2.0f * a1 * p);
    this->c1 = (1.0f - b1) * p - a1 * p * p;

    this->a2 = ratio;
    this->b2 = threshould * (1.0f - ratio);
}

Limiter::~Limiter()
{

}

int Limiter::process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel)
{
   for(int i=0;i<sampleLength;i++)
   {
        outBuffer[i*2] = getCompressedValue(inBuffer[i*2]);
        outBuffer[i*2+1] = getCompressedValue(inBuffer[i*2+1]);
   }
   return 0;
}

float Limiter::getCompressedValue(float input)
{
	float x = fabs(input);
	float sign = input < 0.0f ? -1.0f : 1.0f;
	float value;

	if(input < p)
		value = x;
	else if(input < r)
		value = a1 * x * x + b1 * x + c1;
	else
		value = a2 * x + b2;

	return sign * x;
}

int Limiter::reset()
{

}

