/*
 * Limiter.h
 *
 *  Created on: 2015. 7. 27.
 *      Author: scpark
 */

#include "Plugin.h"
#include <math.h>
#include <stdlib.h>

#ifndef Limiter_H_
#define Limiter_H_

class Limiter : public Plugin {

public:
	Limiter(int sampleRate, int bufferSampleLength, float threshould, float ratio, float kneeWidth /* dB */, float lookupTime = 0.0f /* ms */, float dryWet = 1.0f);
	~Limiter();

	virtual int process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel = BOTH);
	virtual int reset();
private:
	float getCompressedValue(float input);
    float threshould;
	float ratio;
	float kneeWidth;
	float lookupTime;
	float dryWet;

	float p, r;
	float a1, b1, c1;
	float a2, b2;
};


#endif /* Limiter_H_ */
