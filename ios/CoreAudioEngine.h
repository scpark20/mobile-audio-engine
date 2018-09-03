//
//  CoreAudioEngine.h
//  Musician
//
//  Created by musician on 2015. 4. 29..
//
//

#ifndef __Musician__CoreAudioEngine__
#define __Musician__CoreAudioEngine__

#include <string>
#include <stdlib.h>
#include <mach/mach_time.h>
#include <NoteObject.h>
#include "cocos2d.h"

class CoreAudioEngine
{
private:
    void* _engine;
    
    CoreAudioEngine();
public:
    CoreAudioEngine(int systemSampleRate, int streamSize, int minBufferSampleLength, int maxBufferSampleLength);
    virtual ~CoreAudioEngine();
    
    int load(const char *path, int offset, int length);
    int unload(const char *path);
    int play(const char *path, float pitch, float userVolume, float noteVelocity);
    int play(NoteObject *noteObj, float userVolume);
    void fadeStopEffect(int streamID, float time);
    int stopStream(int streamID);
    int stopAllStream();
    float getStreamVolume(int streamID);
    int setStreamVolume(int streamID, float userVolume, float noteVelocity);
    
    void release();
};

#endif /* defined(__Musician__CoreAudioEngine__) */
