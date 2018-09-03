/*
 * Reverb.h
 *
 *  Created on: 2015. 7. 27.
 *      Author: scpark
 */

#include "Plugin.h"
#include "CombFilter.h"
#include "AllPassFilter.h"
#include "OnePoleLPF.h"
#include "cocos2d.h"
#include "../AudioCommon/AudioTool.h"

#ifndef REVERB_H_
#define REVERB_H_

class Reverb : public Plugin {
public:
	Reverb(int sampleRate, int bufferSampleLength, float delay, float feedback, float dryWet);
	~Reverb();

	virtual int process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel = BOTH);
	virtual int reset();
private:
	CombFilter **combFilterArray;
	AllPassFilter **allPassFilterArray;
	OnePoleLPF **onePoleLPFArray;

	int primeList[10000];
	float **buffer;
	float dryWet;
	int DELAY_INDEX_MAX;
};


#endif /* REVERB_H_ */
