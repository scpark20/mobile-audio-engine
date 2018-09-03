/*
 * Compressor.h
 *
 *  Created on: 2015. 7. 27.
 *      Author: scpark
 */

#include "Plugin.h"
#include "../AudioCommon/AudioTool.h"
#include <math.h> 
#include <stdlib.h>

#ifndef Compressor_H_
#define Compressor_H_

class Compressor : public Plugin {

public:
	Compressor
	(int sampleRate, int bufferSampleLength,
	float threshould = 0.9f, float ratio = 0.2f, float kneeWidth = 0.1f /* dB */,
	float lookupTime = 3.0f /* ms */, float attackTime = 3.0f /* ms */, float releaseTime = 5.0f/* ms */);
	~Compressor();

	virtual int process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel = BOTH);
	virtual int reset();

private:
	float startAmplitude;
	float threshould;
	float ratio;
	float kneeWidth;
	float lookupTime;
	float attackTime;
	float releaseTime;

	float *inputRingBuffer;
	int ringBufferSampleLength;
	int lookupSampleLength;
	int writePosition;
	int readPosition;

	float a1, b1, c1; // soft knee coefficient
	float a2, b2; // over threshould coefficient
	float p, r; // soft knee region

	float att, rel;
	float envelope;
	float sum;
};


#endif /* Compressor_H_ */
