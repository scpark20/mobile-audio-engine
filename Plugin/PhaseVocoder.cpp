/*
 * PhaseVocoder.cpp
 *
 *  Created on: Oct 22, 2015
 *      Author: scpark
 */

#include "PhaseVocoder.h"


PhaseVocoder::PhaseVocoder(int sampleRate, int FFTLogSize, int bufferSampleLength, int overlapRatio)
:Plugin(sampleRate, bufferSampleLength), FFTLogSize(FFTLogSize), overlapRatio(overlapRatio)
{
	// TODO Auto-generated constructor stub
	if(FFTLogSize == 1) FFTSize = 2;
	else if(FFTLogSize == 2) FFTSize = 4;
	else if(FFTLogSize == 3) FFTSize = 8;
	else if(FFTLogSize == 4) FFTSize = 16;
	else if(FFTLogSize == 5) FFTSize = 32;
	else if(FFTLogSize == 6) FFTSize = 64;
	else if(FFTLogSize == 7) FFTSize = 128;
	else if(FFTLogSize == 8) FFTSize = 256;
	else if(FFTLogSize == 9) FFTSize = 512;
	else if(FFTLogSize == 10) FFTSize = 1024;
	else if(FFTLogSize == 11) FFTSize = 2048;
	else if(FFTLogSize == 12) FFTSize = 4096;
	else if(FFTLogSize == 13) FFTSize = 8192;
	else if(FFTLogSize == 14) FFTSize = 16384;
	else if(FFTLogSize == 15) FFTSize = 32768;
	else if(FFTLogSize == 16) FFTSize = 65536;

	hopSize = FFTSize / overlapRatio;
	allocateStereoBuffer(currentRealBuffer, FFTSize);
	allocateStereoBuffer(currentImagBuffer, FFTSize);

	allocateStereoBuffer(previousRealBuffer, FFTSize);
	allocateStereoBuffer(previousImagBuffer, FFTSize);

	allocateStereoDoubleBuffer(phaseAcc, FFTSize);

	for(int i=0;i<FFTSize;i++)
	{
		phaseAcc[LEFT_INDEX][i] = 0.0f;
		phaseAcc[RIGHT_INDEX][i] = 0.0f;
	}

	windowBuffer = malloc(FFTSize * sizeof(float));
	AudioTool::putWindowBuffer(windowBuffer, FFTSize);

	inputBuffer = new RingBuffer<float>(FFTSize * 4);
	outputBuffer = new RingBuffer<float>(FFTSize * 4);
}

PhaseVocoder::~PhaseVocoder() {
	// TODO Auto-generated destructor stub

	releaseStereoBuffer(currentRealBuffer);
	releaseStereoBuffer(currentImagBuffer);

	releaseStereoBuffer(previousRealBuffer);
	releaseStereoBuffer(previousImagBuffer);

	releaseStereoBuffer(phaseAcc);

	free(windowBuffer);
}

int PhaseVocoder::process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel)
{
	if(inputBuffer->write(inBuffer, sampleLength, sampleLength, false)<0)
		goto ERROR;

	while(outputBuffer->getRemainedSampleLength() < sampleLength)
	{
		if(inputBuffer->readSeparately(currentRealBuffer[LEFT_INDEX], currentRealBuffer[RIGHT_INDEX], -FFTSize, FFTSize, pitchRate, 0)<0)
			goto ERROR;

		memset(currentImagBuffer[LEFT_INDEX], 0, sizeof(float) * FFTSize);
		memset(currentImagBuffer[RIGHT_INDEX], 0, sizeof(float) * FFTSize);

		if(inputBuffer->readSeparately(previousRealBuffer[LEFT_INDEX], previousRealBuffer[RIGHT_INDEX], -(FFTSize + hopSize * timeRate * pitchRate), FFTSize, pitchRate, hopSize)<0)
			goto ERROR;

		memset(previousImagBuffer[LEFT_INDEX], 0, sizeof(float) * FFTSize);
		memset(previousImagBuffer[RIGHT_INDEX], 0, sizeof(float) * FFTSize);


		doManipulation(LEFT_INDEX);
		doManipulation(RIGHT_INDEX);

		if(outputBuffer->writeSeparately(currentRealBuffer[LEFT_INDEX], currentRealBuffer[RIGHT_INDEX], FFTSize, hopSize, true)<0)
			goto ERROR;

	}

	if(outputBuffer->read(outBuffer, 0, sampleLength, 1.0f, sampleLength)<0)
		goto ERROR;
	return sampleLength;

ERROR:
	cocos2d::log("%s ERROR", __FUNCTION__);
	exit(0);
	memset(outBuffer, 0, sampleLength * sizeof(float) * 2);
	return sampleLength;
}

int PhaseVocoder::doManipulation(int CHANNEL_INDEX)
{
	// FFT
	AudioTool::bufferMultiply(currentRealBuffer[CHANNEL_INDEX], currentRealBuffer[CHANNEL_INDEX], windowBuffer, FFTSize);
	SuperpoweredFFTComplex(currentRealBuffer[CHANNEL_INDEX], currentImagBuffer[CHANNEL_INDEX], FFTLogSize, true);

	AudioTool::bufferMultiply(previousRealBuffer[CHANNEL_INDEX], previousRealBuffer[CHANNEL_INDEX], windowBuffer, FFTSize);
	SuperpoweredFFTComplex(previousRealBuffer[CHANNEL_INDEX], previousImagBuffer[CHANNEL_INDEX], FFTLogSize, true);



	for(int i=0;i<FFTSize/2;i++)
	{
		double currentMagnitude = AudioTool::getMagnitude(currentRealBuffer[CHANNEL_INDEX][i], currentImagBuffer[CHANNEL_INDEX][i]);
		double currentPhase = AudioTool::getPhase(currentRealBuffer[CHANNEL_INDEX][i], currentImagBuffer[CHANNEL_INDEX][i]);
		double previousPhase = AudioTool::getPhase(previousRealBuffer[CHANNEL_INDEX][i], previousImagBuffer[CHANNEL_INDEX][i]);

		phaseAcc[CHANNEL_INDEX][i] += (currentPhase - previousPhase);
		phaseAcc[CHANNEL_INDEX][i] = AudioTool::wrapPhase(phaseAcc[CHANNEL_INDEX][i]);

		currentRealBuffer[CHANNEL_INDEX][i] = currentMagnitude * cos(phaseAcc[CHANNEL_INDEX][i]);
		currentImagBuffer[CHANNEL_INDEX][i] = currentMagnitude * sin(phaseAcc[CHANNEL_INDEX][i]);

		if(i>0)
		{
			currentRealBuffer[CHANNEL_INDEX][FFTSize-i] = currentRealBuffer[CHANNEL_INDEX][i];
			currentImagBuffer[CHANNEL_INDEX][FFTSize-i] = -currentRealBuffer[CHANNEL_INDEX][i];
		}
	}

	// IFFT
	SuperpoweredFFTComplex(currentRealBuffer[CHANNEL_INDEX], currentImagBuffer[CHANNEL_INDEX], FFTLogSize, false);
	AudioTool::bufferMultiply(currentRealBuffer[CHANNEL_INDEX], currentRealBuffer[CHANNEL_INDEX], windowBuffer, FFTSize);

	for(int i=0;i<FFTSize;i++)
		currentRealBuffer[CHANNEL_INDEX][i] /= FFTSize * overlapRatio / 2;
	return 0;
}


int PhaseVocoder::reset()
{

}
