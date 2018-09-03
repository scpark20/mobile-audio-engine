//
//  AudioModule.cpp
//  Musician
//
//  Created by scpark on 2015. 9. 16..
//
//

#include "AudioModule.h"

AudioModule::AudioModule(int sampleRate, int bufferSampleLength, int systemBufferSampleLength, int streamSize)
{
    SAMPLE_RATE = sampleRate;
    BUFFER_SAMPLE_LENGTH = bufferSampleLength;
    SYSTEM_BUFFER_SAMPLE_LENGTH = systemBufferSampleLength;
    STREAM_SIZE = streamSize;
}

AudioModule::~AudioModule()
{

}

long AudioModule::getBufferDelayTime()
{
    return AudioTool::samples2ms(SYSTEM_BUFFER_SAMPLE_LENGTH, SAMPLE_RATE);
}

void AudioModule::setVolumes(float MRVolume, float otherVolume, float myVolume)
{
    audioPool->setVolumes(MRVolume, otherVolume, myVolume);
}


// for MR AUDIO
int AudioModule::releaseMR()
{
    return audioPool->releaseMR();
}

long AudioModule::getMRCurrentTime()
{
    long prevPosition = positionArray[POSITION_CHECK_COUNT - 1];
    long currentTime = AudioTool::getSystemCurrentTime();
    if(prevPosition != currentPosition)
    {
        for(int i=0;i<POSITION_CHECK_COUNT-1;i++)
        {
            positionArray[i] = positionArray[i+1];
            timeArray[i] = timeArray[i+1];
        }
        
        positionArray[POSITION_CHECK_COUNT-1] = currentPosition;
        timeArray[POSITION_CHECK_COUNT-1] = currentTime;
    }
    
    long sum = 0;
    for(int i=0;i<POSITION_CHECK_COUNT;i++)
        sum += AudioTool::samples2ms(positionArray[i] - SYSTEM_BUFFER_SAMPLE_LENGTH, SAMPLE_RATE) + (currentTime - timeArray[i]);
    
    long ret = sum / POSITION_CHECK_COUNT;

    if(ret<0)
    	ret = 0;

    //cocos2d::log("%s %d", __FUNCTION__, ret);
    return ret;
    
}

int AudioModule::setMRCurrentTime(long time)
{
	currentPosition = AudioTool::ms2samples(time, SAMPLE_RATE);
	long currentTime = AudioTool::getSystemCurrentTime();
	for(int j=0;j<POSITION_CHECK_COUNT;j++)
	{
		positionArray[j] = currentPosition;
		timeArray[j] = currentTime;
	}

    return 0;
}

long AudioModule::getMRDuration()
{
    return audioPool->getStreamDuration(MRStreamID);
}

void AudioModule::fadeStopEffect(int streamID, float time)
{
    
    audioPool->fadeStopStream(streamID, time);
}

int AudioModule::stopEffect(int streamID)
{
    
    return audioPool->stopStream(streamID);
}

int AudioModule::stopAllEffects()
{
    
    return audioPool->stopAllStream();
}

float AudioModule::getEffectVolume(int streamID)
{
    
    return audioPool->getStreamVolume(streamID);
}

int AudioModule::setEffectVolume(int streamID, float noteVelocity)
{
    return audioPool->setStreamVolume(streamID, noteVelocity);
}

int AudioModule::setEffectVolumeAbsolutely(int streamID, float volume)
{
    return audioPool->setStreamVolumeAbsolutely(streamID, volume);
}

// for COMMON
int AudioModule::load(const char *path, int offset, int length) // return sound ID
{
    
    unsigned int soundID = AudioTool::getHashValue(path);
    if(audioPool->getSound(soundID)!=nullptr)
        return -1;
    
    Sound *sound = new Sound(path, 0, 0, SAMPLE_RATE);
    audioPool->load(soundID, sound);
    return 0;
}



int AudioModule::unload(const char *path)
{
    
    unsigned int soundID = AudioTool::getHashValue(path);
    return audioPool->unload(soundID);
}

unsigned int AudioModule::GetHashValue(const char *key)
{
	return AudioTool::getHashValue(key);
}

int AudioModule::markNotePlayTime(long time)
{
	int N = effectTimeList.size();

	for(int n=0;n<N;n++)
		if(effectTimeList[n] == time) //already exist
			return -1;

	effectTimeList.push_back(time);
	return 0;
}

int AudioModule::unmarkNotePlayTimeAll()
{
	effectTimeList.clear();
}
