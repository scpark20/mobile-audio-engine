#include "../IZAudioEngine.h"
#include "audio/include/AudioEngine.h"

using namespace cocos2d::experimental;

static IZAudioEngine* s_SharedEngine = nullptr;

IZAudioEngine::IZAudioEngine()
{
	CCASSERT(AudioEngine::lazyInit(),"Fail to initialize AudioEngine!");
}

IZAudioEngine::~IZAudioEngine()
{
	AudioEngine::end();
}

IZAudioEngine* IZAudioEngine::getInstance()
{
    if (s_SharedEngine == nullptr)
    {
        s_SharedEngine = new IZAudioEngine();
    }

    return s_SharedEngine;
}

void IZAudioEngine::end()
{
    if (s_SharedEngine != nullptr)
    {
    	delete s_SharedEngine;
        s_SharedEngine = nullptr;
    }
}

bool IZAudioEngine::allocPlayer(int numberOfPlayer)
{
    return true;
}

void IZAudioEngine::freePlayer()
{

}

int IZAudioEngine::playAudio(const char* pszFilePath, float volume, bool loop)
{
	return AudioEngine::play2d(pszFilePath, loop, this->interpolated(volume));
}

void IZAudioEngine::stopAudio(int audioID)
{
	AudioEngine::stop(audioID);
}

void IZAudioEngine::stopAllAudios()
{
	AudioEngine::stopAll();
}

void IZAudioEngine::pauseAudio(int audioID)
{
	AudioEngine::pause(audioID);
}

void IZAudioEngine::pauseAllAudios()
{
	AudioEngine::pauseAll();
}

void IZAudioEngine::resumeAudio(int audioID)
{
	AudioEngine::resume(audioID);
}

void IZAudioEngine::resumeAllAudios()
{
	AudioEngine::resumeAll();
}

float IZAudioEngine::getAudioVolume(int audioID)
{
	return AudioEngine::getVolume(audioID);
}

void IZAudioEngine::setAudioVolume(int audioID, float volume)
{
	AudioEngine::setVolume(audioID, this->interpolated(volume));
}

double IZAudioEngine::getAudioCurrentTime(int audioID)
{
	return AudioEngine::getCurrentTime(audioID) * 1000.0;
}

void IZAudioEngine::setAudioCurrentTime(int audioID, double time)
{
	AudioEngine::setCurrentTime(audioID, time / 1000.0);
}

double IZAudioEngine::getAudioDuration(int audioID)
{
	return AudioEngine::getDuration(audioID) * 1000.0;
}

float IZAudioEngine::getEffectVolume(int audioID)
{
	return 1.0f;
}

void IZAudioEngine::setEffectVolume(int audioID, float noteVelocity, float userVolume)
{

}

int IZAudioEngine::playEffect(const char* pszFilePath, bool bLoop, float pitch, float pan, float velocity, float userVolume)
{
	return AudioEngine::play2d(pszFilePath, bLoop, 1.0f);
}

void IZAudioEngine::stopEffect(int audioID)
{
	AudioEngine::stop(audioID);
}

void IZAudioEngine::stopAllEffects()
{
	AudioEngine::stopAll();
}

void IZAudioEngine::fadeStopEffect(int audioID, float time)
{
	AudioEngine::stop(audioID);
}

void IZAudioEngine::preloadEffect(const char* pszFilePath)
{
}

void IZAudioEngine::unloadEffect(const char* pszFilePath)
{

}

int IZAudioEngine::playUIEffect(const char* pszFilePath, bool loop,
                                      float pitch, float pan, float gain)
{
	return AudioEngine::play2d(pszFilePath, loop, gain);
}

void IZAudioEngine::stopUIEffect(int audioID)
{
	AudioEngine::stop(audioID);
}

void IZAudioEngine::startRecord(const char* pszFilePath)
{

}

void IZAudioEngine::pauseRecord()
{

}

void IZAudioEngine::resumeRecord()
{

}

void IZAudioEngine::stopRecord()
{

}
