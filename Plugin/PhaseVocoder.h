/*
 * PhaseVocoder.h
 *
 *  Created on: Oct 22, 2015
 *      Author: scpark
 */

#ifndef PHASEVOCODER_H_
#define PHASEVOCODER_H_

#include "Plugin.h"
#include <math.h>
#include <stdlib.h>
#include <SuperpoweredFFT.h>
#include "RingBuffer.h"
#include <AudioCommon/AudioTool.h>


class PhaseVocoder: public Plugin {
public:
	PhaseVocoder(int sampleRate, int FFTLogSize, int bufferSampleLength, int overlapRatio);
	virtual ~PhaseVocoder();

	virtual int process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel = BOTH);
	virtual int reset();

private:
	int doManipulation(int CHANNEL_INDEX);
	// environment variables
	int sampleRate;
	int FFTLogSize;
	int FFTSize;
	int hopSize;
	int bufferSampleLength;
	int overlapRatio;

	// own buffer
	RingBuffer<float> *inputBuffer;
	RingBuffer<float> *outputBuffer;

	float **currentRealBuffer;
	float **currentImagBuffer;
	float **previousRealBuffer;
	float **previousImagBuffer;

	float *windowBuffer;

	//phase related variables
	double **phaseAcc;	//Accumulation

	float pitchRate = 1.0f;
	float timeRate = 1.0f;
};

#endif /* PHASEVOCODER_H_ */
