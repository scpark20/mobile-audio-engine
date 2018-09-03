/*
 * CombFilter.h
 *
 *  Created on: 2015. 7. 27.
 *      Author: scpark
 */

#include "Plugin.h"
#include <stdlib.h>

#ifndef COMBFILTER_H_
#define COMBFILTER_H_

class CombFilter : public Plugin {

public:
	CombFilter(int sampleRate, int bufferSampleLength, int delaySamples, float feedbackGain);
	~CombFilter();

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


#endif /* COMBFILTER_H_ */
