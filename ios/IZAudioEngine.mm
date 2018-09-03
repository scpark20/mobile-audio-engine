//
//  IZAudioEngine.mm
//  HappyPianistUS
//
//  Created by youngdoo kim on 14. 4. 5..
//
//

#include "OALSimpleAudioEx.h"
#include "IZ_Record.h"

#include "../IZAudioEngine.h"
#include "CoreAudioModule.h"

using namespace cocos2d;
using namespace cocos2d::experimental;

static IZAudioEngine* s_SharedEngine = NULL;

IZAudioEngine::IZAudioEngine()
{
    audioModule = (void *) new CoreAudioModuleProxy(44100, 512, 0, 100);
}

IZAudioEngine::~IZAudioEngine()
{
    
}

IZAudioEngine* IZAudioEngine::getInstance()
{
    if (s_SharedEngine == NULL)
    {
        s_SharedEngine = new IZAudioEngine();
        
    }
    
    return s_SharedEngine;
}

void IZAudioEngine::end()
{
    if (s_SharedEngine != NULL)
    {
        [[OALSimpleAudioEx sharedInstance] release];
        [[IZ_Record getInstance] destroy];
        
        delete s_SharedEngine;
        s_SharedEngine = NULL;
    }
}

int IZAudioEngine::allocPlayer(int numberOfPlayer)
{
    return ((AudioModule*)audioModule)->init(numberOfPlayer);
}

void IZAudioEngine::freePlayer()
{
    
}

int IZAudioEngine::markNotePlayTime(long time)
{
    return ((AudioModule*)audioModule)->markNotePlayTime(time);
}

int IZAudioEngine::unmarkNotePlayTimeAll()
{
    return ((AudioModule*)audioModule)->unmarkNotePlayTimeAll();
}

long IZAudioEngine::getBufferDelayTime()
{
    return ((AudioModule*)audioModule)->getBufferDelayTime();
}

void IZAudioEngine::setVolumes(float MRVolume, float otherVolume, float myVolume)
{
    ((AudioModule*)audioModule)->setVolumes(MRVolume, otherVolume, myVolume);
}

//int IZAudioEngine::playAudio(const char* pszFilePath, float volume, bool loop)
//{
//    NSString *fileName = [NSString stringWithUTF8String:pszFilePath];
//    return [[OALSimpleAudioEx sharedInstance] playAudio:fileName volume:AudioTool::userVolumeScale(volume*100.0f) pan:0.0f loop:loop];
//}
//
//void IZAudioEngine::stopAudio(int audioID)
//{
//    [[OALSimpleAudioEx sharedInstance] stopAudio:audioID];
//}
//
//void IZAudioEngine::stopAllAudios()
//{
//    [[OALSimpleAudioEx sharedInstance] stopAllAudios];
//}
//
//void IZAudioEngine::pauseAudio(int audioID)
//{
//    [[OALSimpleAudioEx sharedInstance] pauseAudio:audioID];
//}
//
//void IZAudioEngine::pauseAllAudios()
//{
//    [[OALSimpleAudioEx sharedInstance] pauseAllAudios];
//}
//
//void IZAudioEngine::resumeAudio(int audioID)
//{
//    [[OALSimpleAudioEx sharedInstance] resumeAudio:audioID];
//}
//
//void IZAudioEngine::resumeAllAudios()
//{
//    [[OALSimpleAudioEx sharedInstance] resumeAllAudios];
//}
//
//float IZAudioEngine::getAudioVolume(int audioID)
//{
//    return [[OALSimpleAudioEx sharedInstance] getAudioVolume:audioID];
//}
//
//void IZAudioEngine::setAudioVolume(int audioID, float volume)
//{
//    [[OALSimpleAudioEx sharedInstance] setAudioVolume:audioID volume:AudioTool::userVolumeScale(volume*100.0f)];
//}
//
//double IZAudioEngine::getAudioCurrentTime(int audioID)
//{
//    return [[OALSimpleAudioEx sharedInstance] getAudioCurrentTime:audioID] * 1000.0;
//}
//
//void IZAudioEngine::setAudioCurrentTime(int audioID, double time)
//{
//    [[OALSimpleAudioEx sharedInstance] setAudioCurrentTime:audioID time:time / 1000.0];
//}
//
//double IZAudioEngine::getAudioDuration(int audioID)
//{
//    return [[OALSimpleAudioEx sharedInstance] getAudioDuration:audioID];
//}

// for MR Sound ///
void IZAudioEngine::readyMR(char *path, AudioType audioType)
{
    ((AudioModule*)audioModule)->readyMR(path, audioType);
}

void IZAudioEngine::readyMR(NoteObject *noteObj)
{
    ((AudioModule*)audioModule)->readyMR(noteObj);
}

void IZAudioEngine::startMR()
{
    ((AudioModule*)audioModule)->startMR();
}

void IZAudioEngine::pauseMR()
{
    ((AudioModule*)audioModule)->pauseMR();
}

void IZAudioEngine::resumeMR()
{
    ((AudioModule*)audioModule)->resumeMR();
}

void IZAudioEngine::stopMR()
{
    ((AudioModule*)audioModule)->stopMR();
}

void IZAudioEngine::releaseMR()
{
    ((AudioModule*)audioModule)->releaseMR();
}

long IZAudioEngine::getMRCurrentTime()
{
    return ((AudioModule*)audioModule)->getMRCurrentTime();
}

void IZAudioEngine::setMRCurrentTime(long time)
{
    ((AudioModule*)audioModule)->setMRCurrentTime(time);
}

long IZAudioEngine::getMRDuration()
{
    return ((AudioModule*)audioModule)->getMRDuration();
}

void IZAudioEngine::preloadEffect(const char* pszFilePath)
{
#ifndef MIXING
    NSString *fileName = [NSString stringWithUTF8String:pszFilePath];
    [[OALSimpleAudioEx sharedInstance] preloadEffect:fileName];
#else
    std::string path = FileUtils::getInstance()->fullPathForFilename(pszFilePath);
    ((AudioModule*)audioModule)->load(path.c_str(), 0, 0);
#endif
}

void IZAudioEngine::unloadEffect(const char* pszFilePath)
{
#ifndef MIXING
    NSString *fileName = [NSString stringWithUTF8String:pszFilePath];
    [[OALSimpleAudioEx sharedInstance] unloadEffect:fileName];
#else
    std::string path = FileUtils::getInstance()->fullPathForFilename(pszFilePath);
    ((AudioModule*)audioModule)->unload(path.c_str());
#endif
}

//int IZAudioEngine::playEffect(const char* pszFilePath, bool bLoop,
//                              float pitch, float pan, float gain)
//{
//    NSString *fileName = [NSString stringWithUTF8String:pszFilePath];
//    return [[OALSimpleAudioEx sharedInstance] playEffect:fileName volume:this->interpolated(gain) pitch:pitch pan:pan loop:bLoop];
//}


int IZAudioEngine::playEffect(const char* pszFilePath, bool bLoop, float pitch, float pan, float velocity)
{
#ifndef MIXING
    NSString *fileName = [NSString stringWithUTF8String:pszFilePath];
    return [[OALSimpleAudioEx sharedInstance] playEffect:fileName volume:(velocity / 128.0f) pitch:pitch pan:pan loop:bLoop];
#else
    std::string path = FileUtils::getInstance()->fullPathForFilename(pszFilePath);
    return ((AudioModule*)audioModule)->playEffect(path.c_str(), pitch, velocity);
#endif
}


int IZAudioEngine::playEffect(NoteObject *noteObj)
{
#ifndef MIXING
    NSString *fileName = [NSString stringWithUTF8String:noteObj->m_FullPath];
    return [[OALSimpleAudioEx sharedInstance] playEffect:fileName volume:(noteObj->m_velocity / 128.0f) pitch:noteObj->m_Pitch pan:noteObj->m_Pan loop:false];
#else
    return ((AudioModule*)audioModule)->playEffect(noteObj);
#endif

}


int IZAudioEngine::startEffect()
{
    return 0;
}

void IZAudioEngine::stopEffect(int audioID)
{
#ifndef MIXING
    [[OALSimpleAudioEx sharedInstance] stopEffect:audioID];
#else
    ((AudioModule*)audioModule)->stopEffect(audioID);
#endif
}

void IZAudioEngine::stopAllEffects()
{
#ifndef MIXING
    [[OALSimpleAudioEx sharedInstance] stopAllEffects];
#else
    ((AudioModule*)audioModule)->stopAllEffects();
#endif
}

#ifdef MIXING
void IZAudioEngine::fadeStopEffect(int audioID, float time)
{
    ((AudioModule*)audioModule)->fadeStopEffect(audioID, time);
}
#endif


void IZAudioEngine::startRecord(const char* pszFilePath)
{
    NSString *fileName = [NSString stringWithUTF8String:pszFilePath];
    [[IZ_Record getInstance] recordStart:fileName];
}

void IZAudioEngine::pauseRecord()
{
    [[IZ_Record getInstance] recordPause];
}

void IZAudioEngine::resumeRecord()
{
    [[IZ_Record getInstance] recordResume];
}

void IZAudioEngine::stopRecord()
{
    [[IZ_Record getInstance] recordStop];
}