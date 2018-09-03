//
//  CoreAudioModuleProxy.h
//  Musician
//
//  Created by musician on 2015. 4. 29..
//
//

#ifndef __Musician__CoreAudioModuleProxy__
#define __Musician__CoreAudioModuleProxy__

#include <string>
#include <stdlib.h>
#include <mach/mach_time.h>
#include <NoteObject.h>
#include "cocos2d.h"
#include "../AudioCommon/AudioModule.h"
#include "../AudioCommon/AudioPool.h"
#include "../AudioCommon/AudioTool.h"
#include "../AudioCommon/Sound.h"

class CoreAudioModuleProxy : public AudioModule
{
private:
    void* audioModule;
    
public:
    CoreAudioModuleProxy(int systemSampleRate, int bufferSampleLength, int systemBufferSampleLength, int streamSize);
    virtual ~CoreAudioModuleProxy();

    virtual int init(int playerNumberMax);
    virtual int release();
    
    // for MR AUDIO
    virtual int readyMR(const char* path, AudioType audioType);
    virtual int readyMR(NoteObject* noteObj);
    
    virtual int startMR();
    virtual int pauseMR();
    virtual int resumeMR();
    virtual int stopMR();
    
    // for NOTE EFFECT
    virtual int playEffect(const char *path, float pitch, float noteVelocity); // return Stream ID
    virtual int playEffect(NoteObject *noteObj); // return Stream ID
    
    virtual int startEffect();
};
#endif /* defined(__Musician__CoreAudioModuleProxy__) */
