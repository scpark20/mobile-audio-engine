

/*
 * OnePoleLPF.h
 *
 *  Created on: 2015. 7. 27.
 *      Author: scpark
 */

#include "Plugin.h"
#include <math.h>
#include <stdlib.h>

#ifndef OnePoleLPF_H_
#define OnePoleLPF_H_

class OnePoleLPF : public Plugin {

public:
	OnePoleLPF(int sampleRate, int bufferSampleLength, int centerFreq);
	~OnePoleLPF();

	virtual int process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel = BOTH);
	virtual int reset();
private:
	float getOutput(float value, bool left);
	float feedbackGain;
	float feedforwardGain;
	float delayedLeftBuffer;
	float delayedRightBuffer;
};


#endif /* OnePoleLPF_H_ */

