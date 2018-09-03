#ifndef __IZ_AUDIO_ENGINE_H__
#define __IZ_AUDIO_ENGINE_H__

#include "cocos2d.h"
#include "../Manager/TweenerManager.h"
#include "../Platform/NativeDevice.h"
#include <NoteObject.h>

#if(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	#define MIXING
#else
	#define MIXING
#endif

using namespace cocos2d;

enum AudioType {
	MR = 0,
	VOICE = 1,
	NOTE = 2
};


class IZAudioEngine
{
private:
	void* audioModule;

public:
    static IZAudioEngine* getInstance();
    static IZAudioEngine* sharedEngine() { return IZAudioEngine::getInstance(); }
    static void end();
    
protected:
    IZAudioEngine();
    ~IZAudioEngine();
    
public:
    int allocPlayer(int numberOfPlayer);
    void freePlayer();

    long getBufferDelayTime();
    void setVolumes(float MRVolume, float otherVolume, float myVolume);
    int markNotePlayTime(long time);
    int unmarkNotePlayTimeAll();
    /// for general sound ///
//    int playAudio(const char* pszFilePath, float volume = 1.0f, bool loop = false);
//
//    void stopAudio(int audioID);
//    void stopAllAudios();
//
//    void pauseAudio(int audioID);
//    void pauseAllAudios();
//
//    void resumeAudio(int audioID);
//    void resumeAllAudios();
//
//    float getAudioVolume(int audioID);
//    void setAudioVolume(int audioID, float volume);
//
//    double getAudioCurrentTime(int audioID);
//    void setAudioCurrentTime(int audioID, double time);
//
//    double getAudioDuration(int audioID);

    /// for MR Sound ///
    void readyMR(char *path, AudioType audioType);
    void readyMR(NoteObject *noteObj);
    void startMR();
    void pauseMR();
    void resumeMR();
    void stopMR();
    void releaseMR();

    long getMRCurrentTime();
    void setMRCurrentTime(long time);
    long getMRDuration();

    /// for effect sound ///
    void preloadEffect(const char* pszFilePath);
    void unloadEffect(const char* pszFilePath);
    int playEffect(const char* pszFilePath, bool bLoop = false, float pitch = 1.0f, float pan = 0.0f, float velocity = 1.0f);
    int playEffect(NoteObject *noteObj);
    int startEffect();

    void stopEffect(int audioID);
    void stopAllEffects();
    void fadeStopEffect(int audioID, float time = 0.1f);

    void startRecord(const char* pszFilePath);
    void pauseRecord();
    void resumeRecord();
    void stopRecord();
};


#endif /* defined(__IZ_AUDIO_ENGINE_H__) */
