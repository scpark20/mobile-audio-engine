/*
 * ReSampler.h
 *
 *  Created on: Oct 27, 2015
 *      Author: scpark
 */

#ifndef RESAMPLER_H_
#define RESAMPLER_H_

#include <stdlib.h>
#include "Plugin.h"
#include "RingBuffer.h"

class ReSampler : public Plugin {
public:
	ReSampler(int sampleRate, int bufferSampleLength);
	virtual ~ReSampler();
	virtual int process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel = BOTH);
	virtual int reset();

private:
	RingBuffer<float> *ownBuffer;
	int ownBufferSampleLength;
	float timeRate = 1.5f;
};

#endif /* RESAMPLER_H_ */
