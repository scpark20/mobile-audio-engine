/*
 * Stream.cpp
 *
 *  Created on: 2015. 5. 15.
 *      Author: IZSoft-PSC
 */

#include "Stream.h"

Stream::Stream(Sound *sound, int streamID, int _sampleRate, int _bufferSampleLength)
:state(STREAM_STATE_END), position(0), volume(1.0f), sampleRate(_sampleRate), prevStream(NULL), nextStream(NULL), bufferSampleLength(_bufferSampleLength)
{
	this->sound = sound;
	this->streamID = streamID;

	this->floatBuffer = (float*) malloc(sizeof(float) * bufferSampleLength * 2);
}

Stream::~Stream()
{
    free(floatBuffer);
}

unsigned int Stream::process(float *float_buffer, int numberOfSamples)
{
	if(END_READY)
		return 0;

	double samplesReaded = 0;
    
	if(sound!=nullptr)
		if(this->pitchRateEnable)
			samplesReaded = sound->read(float_buffer, position, numberOfSamples, leftVolume, rightVolume, leftVolumeDelta, rightVolumeDelta, pitchRate);
		else
			samplesReaded = sound->read(float_buffer, position, numberOfSamples, leftVolume, rightVolume, leftVolumeDelta, rightVolumeDelta);

	leftVolume -= samplesReaded * leftVolumeDelta;
	rightVolume -= samplesReaded * rightVolumeDelta;

	if((int)samplesReaded != numberOfSamples)
		END_READY = true;

	position += samplesReaded * pitchRate;

	return samplesReaded;
}

unsigned int Stream::process(float *destFloatBuffer, int numberOfSamples, long currentSamplePosition)
{
  	long soundPosition = currentSamplePosition - playSamplePosition;
	if(soundPosition < 0)
		soundPosition = 0;

	int offset = playSamplePosition - currentSamplePosition;
	if(offset < 0)
		offset = 0;

	int samplesToRead = numberOfSamples - offset;
	memset(floatBuffer, 0, sizeof(float) * numberOfSamples * 2);


    //cocos2d::log("%s1 %d %d %d %d", __FUNCTION__, offset, soundPosition, samplesToRead, sound->getDurationSamples());
   	double samplesReaded = 0;
    if(sound!=nullptr)
        samplesReaded = sound->read(&floatBuffer[offset], soundPosition, samplesToRead, leftVolume, rightVolume, 0, 0);

	/// if needed, process make release ///
	if(currentSamplePosition + numberOfSamples > playSamplePosition + playSampleDuration)
	{
		for(int i=0;i<numberOfSamples;i++)
		{
			int indexFromRelease = (currentSamplePosition + i) - (playSamplePosition + playSampleDuration);
			if(indexFromRelease < 0)
				continue;

			//linear attenuation
			//volume curve could be made by array
			float releaseVolume = 1.0f - (float)indexFromRelease / (float)releaseSampleDuration;
			if(releaseVolume < 0.0f) releaseVolume = 0.0f;
			else if (releaseVolume > 1.0f) releaseVolume = 1.0f;

			floatBuffer[i*2] *= releaseVolume;
			floatBuffer[i*2+1] *= releaseVolume;
		}
	}

	double sum = 0;
	for(int i=0;i<numberOfSamples;i++)
	{
		destFloatBuffer[i*2] += floatBuffer[i*2];
		destFloatBuffer[i*2+1] += floatBuffer[i*2+1];
		sum += fabs(floatBuffer[i*2]) + fabs(floatBuffer[i*2+1]);
	}
	//cocos2d::log("%s %d %d %d %f %f %f %f", __FUNCTION__, offset, position, samplesToRead, leftVolume, rightVolume, samplesReaded, sum/numberOfSamples);
	return samplesReaded;
}

int Stream::setSound(Sound *sound)
{
	this->sound = sound;
	this->position = 0;
	this->END_READY = false;
}

int Stream::releaseSound()
{
    delete sound;
    sound = nullptr;
    return 0;
}

int Stream::setState(StreamState state)
{
	this->state = state;
	if(state == STREAM_STATE_END)
		this->setPlayerID(-1);
	return 0;
}

int Stream::getState()
{
	return state;
}

void Stream::setPan(float pan)
{
	this->pan = pan;
	this->leftPan = sqrt(-2.0f * (pan - 1.0f));
	this->rightPan = sqrt(2.0f * pan);

	this->leftVolume = volume * leftPan;
	this->rightVolume = volume * rightPan;

}

int Stream::getStreamID()
{
	return streamID;
}

int Stream::setVolume(float volume)
{
	this->volume = volume;

	this->leftVolumeDelta = 0.0f;
	this->rightVolumeDelta = 0.0f;

	this->leftVolume = volume * leftPan;
	this->rightVolume = volume * rightPan;

	return 0;
}

float Stream::getVolume()
{
	return this->volume;
}

int Stream::setVolumeGradually(float targetVolume, float deltaTime)
{
	float leftTargetVolume = targetVolume * this->leftPan;
	float rightTargetVolume = targetVolume * this->rightPan;

	float newLeftVolumeDelta = (leftVolume - leftTargetVolume) / AudioTool::ms2samples(deltaTime * 1000.0f, sampleRate);
	float newRightVolumeDelta = (rightVolume - rightTargetVolume) / AudioTool::ms2samples(deltaTime * 1000.0f, sampleRate);

	if(leftVolumeDelta < newLeftVolumeDelta)
		leftVolumeDelta = newLeftVolumeDelta;

	if(rightVolumeDelta < newRightVolumeDelta)
		rightVolumeDelta = newRightVolumeDelta;

}

Sound *Stream::getSound()
{
	return sound;
}

int Stream::setPositionInTime(long time)
{
	this->position = time / 1000.0f * (float)sampleRate;
}

long Stream::getPositionInTime()
{
	long ret;
	long currentTime = AudioTool::getSystemCurrentTime();

	if(prevPosition == position)
	{
		ret = position / (float)sampleRate * 1000.0f;
		ret += currentTime - prevTime;
	}
	else
		ret = position / (float)sampleRate * 1000.0f;

	prevPosition = position;
	prevTime = currentTime;

	return ret;
}


int Stream::setPlayPositionInTime(long time)
{
	this->playSamplePosition = (long) (time / 1000.0f * (float)sampleRate);
}

long Stream::getPlayPositionInTime()
{
	return playSamplePosition / (float)sampleRate * 1000.0f;
}

int Stream::setPlayPositionInSample(long sample)
{
	this->playSamplePosition = sample;
}

long Stream::getPlayPositionInSample()
{
	return this->playSamplePosition;
}

int Stream::setPlayDurationInTime(long time)
{
	this->playSampleDuration = (long) (time / 1000.0f * (float)sampleRate);
}

long Stream::getPlayDurationInTime()
{
	return playSampleDuration / (float)sampleRate * 1000.0f;
}

int Stream::setPlayDurationInSample(long sample)
{
	this->playSampleDuration = sample;
}

long Stream::getPlayDurationInSample()
{
	return this->playSampleDuration;
}

int Stream::setReleaseDurationInTime(long time)
{
	this->releaseSampleDuration = (long) (time / 1000.0f * (float)sampleRate);
}

long Stream::getReleaseDurationInTime()
{
	return releaseSampleDuration / (float)sampleRate * 1000.0f;
}

int Stream::setReleaseDurationInSample(long sample)
{
	this->releaseSampleDuration = sample;
}

long Stream::getReleaseDurationInSample()
{
	return this->releaseSampleDuration;
}

