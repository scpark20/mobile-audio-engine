/*
 * OnePoleLPF.cpp
 *
 *  Created on: 2015. 7. 27.
 *      Author: scpark
 */

#include "OnePoleLPF.h"


OnePoleLPF::OnePoleLPF(int sampleRate, int bufferSampleLength, int centerFreq)
:Plugin(sampleRate, bufferSampleLength)
{
	float thetac = 2.0f * M_PI * (float) centerFreq / (float) sampleRate;
	float gamma = 2.0f - cos(thetac);
	feedbackGain = sqrt(gamma * gamma - 1.0f) - gamma;
	feedforwardGain = 1 + feedbackGain;
	this->delayedLeftBuffer = 0;
	this->delayedRightBuffer = 0;
}

OnePoleLPF::~OnePoleLPF()
{

}

int OnePoleLPF::process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel)
{
	for(int i=0;i<sampleLength;i++)
	{
		if(channel & LEFT)
			outBuffer[i*2] = getOutput(inBuffer[i*2], true);
		if(channel & RIGHT)
			outBuffer[i*2+1] = getOutput(inBuffer[i*+2+1], false);
	}
}

float OnePoleLPF::getOutput(float input, bool left)
{
	if(left)
	{
		float output = delayedLeftBuffer * -feedbackGain + input * feedforwardGain;
		delayedLeftBuffer = output;
		return output;
	}
	else
	{
		float output = delayedRightBuffer * -feedbackGain + input * feedforwardGain;
		delayedRightBuffer = output;
		return output;
	}
}

int OnePoleLPF::reset()
{

}

