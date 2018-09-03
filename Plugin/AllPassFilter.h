/*
 * AllPassFilter.h
 *
 *  Created on: 2015. 7. 27.
 *      Author: scpark
 */

#include "Plugin.h"
#include <stdlib.h>

#ifndef ALLPASSFILTER_H_
#define ALLPASSFILTER_H_

class AllPassFilter : public Plugin {

public:
	AllPassFilter(int sampleRate, int bufferSampleLength, int delaySamples, float feedbackGain);
	~AllPassFilter();

	virtual int process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel = BOTH);
	virtual int reset();
private:
	float getOutput(float value, bool left);
	int delaySamples;
	float feedbackGain;
	float *delayLeftBuffer;
	float *delayRightBuffer;
	int delayLeftPosition;
	int delayRightPosition;
};


#endif /* ALLPASSFILTER_H_ */
