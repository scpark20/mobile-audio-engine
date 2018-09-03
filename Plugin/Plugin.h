/*
 * Plugin.h
 *
 *  Created on: 2015. 7. 27.
 *      Author: scpark
 */

#if(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    #include <arm_neon.h>
    #include <cpu-features.h>
#endif

#ifndef PLUGIN_H_
#define PLUGIN_H_

typedef enum {
	LEFT = 1,
	RIGHT = 2,
	BOTH = 3
} channel_t;

class Plugin {
public:
	Plugin(int sampleRate, int bufferSampleLength)
	{
		this->sampleRate = sampleRate;
		this->bufferSampleLength = bufferSampleLength;
	}
	virtual ~Plugin()
	{

	}
	virtual int process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel = BOTH) = 0;
	virtual int reset() = 0;
protected:
	int sampleRate;
	int bufferSampleLength;

};

#endif /* PLUGIN_H_ */
