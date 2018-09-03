/*
 * ReSampler.cpp
 *
 *  Created on: Oct 27, 2015
 *      Author: scpark
 */

#include "ReSampler.h"

ReSampler::ReSampler(int sampleRate, int bufferSampleLength)
:Plugin(sampleRate, bufferSampleLength)
{
	// TODO Auto-generated constructor stub
	ownBuffer = new RingBuffer<float>(bufferSampleLength * 2);
}

ReSampler::~ReSampler()
{
	// TODO Auto-generated destructor stub
	free(ownBuffer);
}

int ReSampler::process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel)
{
	ownBuffer->write(inBuffer, sampleLength, sampleLength, false);
	int readSamples = ownBuffer->getRemainedSampleLength() / timeRate;
	ownBuffer->read(outBuffer, 0, readSamples, timeRate, timeRate * readSamples);
	return readSamples;
}

int ReSampler::reset()
{
	ownBuffer->reset();
}
