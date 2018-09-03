/*
 * CombFilter.cpp
 *
 *  Created on: 2015. 7. 27.
 *      Author: scpark
 */

#include "CombFilter.h"


CombFilter::CombFilter(int sampleRate, int bufferSampleLength, int delaySamples, float feedbackGain)
:Plugin(sampleRate, bufferSampleLength)
{
	this->delaySamples = delaySamples;
	this->feedbackGain = feedbackGain;
	this->delayLeftBuffer = (float *) malloc(sizeof(float) * delaySamples);
	this->delayRightBuffer = (float *) malloc(sizeof(float) * delaySamples);
	this->delayLeftPosition = 0;
	this->delayRightPosition = 0;
	memset(delayLeftBuffer, 0, sizeof(float) * delaySamples);
	memset(delayRightBuffer, 0, sizeof(float) * delaySamples);

}

CombFilter::~CombFilter()
{
	free(delayLeftBuffer);
	free(delayRightBuffer);
}

int CombFilter::process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel)
{
	for(int i=0;i<sampleLength;i++)
	{
		if(channel & LEFT)
			outBuffer[i*2] = getOutput(inBuffer[i*2], true);
		if(channel * RIGHT)
			outBuffer[i*2+1] = getOutput(inBuffer[i*+2+1], false);
	}
}

float CombFilter::getOutput(float value, bool left)
{
	if(left)
	{
		float delayedSample = delayLeftBuffer[delayLeftPosition];
		delayLeftBuffer[delayLeftPosition] *= feedbackGain;
		delayLeftBuffer[delayLeftPosition] += value;
		delayLeftPosition = (delayLeftPosition + 1) % delaySamples;
		return delayedSample;
	}
	else
	{
		float delayedSample = delayRightBuffer[delayRightPosition];
		delayRightBuffer[delayRightPosition] *= feedbackGain;
		delayRightBuffer[delayRightPosition] += value;
		delayRightPosition = (delayRightPosition + 1) % delaySamples;
		return delayedSample;
	}
}

int CombFilter::reset()
{

}
