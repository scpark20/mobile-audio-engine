/*
 * AllPassFilter.cpp
 *
 *  Created on: 2015. 7. 27.
 *      Author: scpark
 */

#include "AllPassFilter.h"


AllPassFilter::AllPassFilter(int sampleRate, int bufferSampleLength, int delaySamples, float feedbackGain)
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

AllPassFilter::~AllPassFilter()
{
	free(delayLeftBuffer);
	free(delayRightBuffer);
}

int AllPassFilter::process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel)
{
	for(int i=0;i<sampleLength;i++)
	{
		if(channel & LEFT)
			outBuffer[i*2] = getOutput(inBuffer[i*2], true);
		if(channel & RIGHT)
		outBuffer[i*2+1] = getOutput(inBuffer[i*+2+1], false);
	}
}

float AllPassFilter::getOutput(float value, bool left)
{
	if(left)
	{
		float delayedSample = delayLeftBuffer[delayLeftPosition];
		float w = delayedSample * feedbackGain + value;
		delayLeftBuffer[delayLeftPosition] = w;
		delayLeftPosition = (delayLeftPosition + 1) % delaySamples;
		return w * -feedbackGain + delayedSample;
	}
	else
	{
		float delayedSample = delayRightBuffer[delayRightPosition];
		float w = delayedSample * feedbackGain + value;
		delayRightBuffer[delayRightPosition] = w;
		delayRightPosition = (delayRightPosition + 1) % delaySamples;
		return w * -feedbackGain + delayedSample;
	}
}

int AllPassFilter::reset()
{

}
