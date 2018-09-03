/*
 * Compressor.cpp
 *
 *  Created on: 2015. 7. 27.
 *      Author: scpark
 */

#include "Compressor.h"


Compressor::Compressor(int sampleRate, int bufferSampleLength, float threshould /* float */, float ratio /* y/x */, float kneeWidth /* float */, float lookupTime /* ms */, float attackTime /* ms */, float releaseTime /* ms */)
:Plugin(sampleRate, bufferSampleLength)
{
    this->threshould = threshould;
    this->ratio = ratio;
    this->kneeWidth = kneeWidth;
    this->attackTime = attackTime;
    this->releaseTime = releaseTime;
    this->lookupTime = lookupTime;
    this->lookupSampleLength = AudioTool::ms2samples(lookupTime, sampleRate);
    this->ringBufferSampleLength = lookupSampleLength + bufferSampleLength;
    this->inputRingBuffer = (float *) malloc(sizeof(float) * ringBufferSampleLength * 2);
    memset(inputRingBuffer, 0, sizeof(float) * ringBufferSampleLength * 2);
    this->readPosition = 0;
    this->writePosition = ringBufferSampleLength - bufferSampleLength;

    p = threshould - kneeWidth / 2.0f;
    r = threshould + kneeWidth / 2.0f;
    a1 = (ratio - 1.0f) / (2.0f * (r - p));
    b1 = 1.0f - (2.0f * a1 * p);
    c1 = (1.0f - b1) * p - a1 * p * p;

    a2 = ratio;
    b2 = threshould * (1.0f - ratio);

    att = (attackTime == 0.0f) ? 0.0f : exp (-1.0f / (sampleRate * attackTime / 1000.0f));
    rel = (releaseTime == 0.0f) ? 0.0f : exp (-1.0f / (sampleRate * releaseTime / 1000.0f));
    envelope = 0.0f;
    sum = 0;
}

Compressor::~Compressor()
{
	free(inputRingBuffer);
}

int Compressor::process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel)
{
	int copySampleLength = ringBufferSampleLength - writePosition > sampleLength ? sampleLength : ringBufferSampleLength - writePosition;
	memcpy(&inputRingBuffer[writePosition * 2], inBuffer, sizeof(float) * copySampleLength * 2);

	if(copySampleLength < sampleLength)
		memcpy(inputRingBuffer, &inBuffer[copySampleLength * 2], sizeof(float) * (sampleLength - copySampleLength) * 2);

	double RMS = 0;
	for(int i=0;i<sampleLength;i++)
	{
		/* find RMS */
		/*
		if(i==0)
			for(int j=0;j<lookupSampleLength;j++)
			{
				int sampleIndex = (readPosition + i + j) % ringBufferSampleLength;
				sum += inputRingBuffer[sampleIndex * 2] * inputRingBuffer[sampleIndex * 2]; // left channel
				sum += inputRingBuffer[sampleIndex * 2 + 1] * inputRingBuffer[sampleIndex * 2 + 1]; // right channel
			}
		else
		*/
		{
			int prevSampleIndex = AudioTool::getRingBufferPosition(readPosition + i - 1, ringBufferSampleLength);
			sum -= inputRingBuffer[prevSampleIndex * 2] * inputRingBuffer[prevSampleIndex * 2]; //left channel
			sum -= inputRingBuffer[prevSampleIndex * 2 + 1] * inputRingBuffer[prevSampleIndex * 2 + 1]; //right channel

			int addSampleIndex = (readPosition + i + lookupSampleLength) % ringBufferSampleLength;
			sum += inputRingBuffer[addSampleIndex * 2] * inputRingBuffer[addSampleIndex * 2]; //left channel
			sum += inputRingBuffer[addSampleIndex * 2 + 1] * inputRingBuffer[addSampleIndex * 2 + 1]; //right channel


		}

		if(sum<0.0f) //it can be possible because of inaccuracy of float point
			sum = 0.0f;

		RMS = sum / (lookupSampleLength * 2);
		RMS = sqrt(sum);


		/* get Gain */
		float theta = RMS > envelope ? att : rel;
		envelope = (1.0f - theta) * RMS + theta * envelope;

		float gain;
		if(envelope <= p) //do not compress
			gain = 1.0f;
		else if(envelope < r)//within soft-knee
		{
			float out = (a1 * envelope * envelope) + (b1 * envelope) + c1;
		    gain = out / envelope;
		}
		else //linear compress
		{
			float out = (a2 * envelope) + b2;
			gain = out / envelope;
		}

		/* OUTPUT */
		float leftInput = inputRingBuffer[((readPosition + i) % ringBufferSampleLength) * 2];
		float rightInput = inputRingBuffer[((readPosition + i) % ringBufferSampleLength) * 2 + 1];

		outBuffer[i*2] = leftInput * gain;
		outBuffer[i*2+1] = rightInput * gain;

	}
	writePosition = (writePosition + sampleLength) % ringBufferSampleLength;
	readPosition = (readPosition + sampleLength) % ringBufferSampleLength;
}

int Compressor::reset()
{
	memset(inputRingBuffer, 0, sizeof(float) * ringBufferSampleLength * 2);
	this->readPosition = 0;
	this->writePosition = ringBufferSampleLength - bufferSampleLength;

	envelope = 0.0f;
	sum = 0;

	return 0;
}
