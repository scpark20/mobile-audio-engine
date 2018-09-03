#include "../IZAudioEngine.h"
#include "jni/JniHelper.h"
#include "OpenSLAudioModule.h"
#include "OpenSLAudioModule_AllMixing.h"
#include "OpenSLAudioModule_ChordMixing.h"

static IZAudioEngine* s_SharedEngine = nullptr;
#define ALL_MIXING false
#define FAST_TRACK true

IZAudioEngine::IZAudioEngine()
: audioModule(nullptr)
{
	JniMethodInfo methodInfo;

	// samplerate 얻어오기
	if (! JniHelper::getStaticMethodInfo(methodInfo,
			"com/izsoft/lib/IZSoftHelper", "getSystemSampleRate", "()I"))
	{
		methodInfo.env->DeleteLocalRef(methodInfo.classID);
	}

	jint sampleRate = methodInfo.env->CallStaticIntMethod(methodInfo.classID, methodInfo.methodID);
	methodInfo.env->DeleteLocalRef(methodInfo.classID);

	// bufferSize 얻어오기
	if (! JniHelper::getStaticMethodInfo(methodInfo,
			"com/izsoft/lib/IZSoftHelper", "getSystemAudioBuffer", "()I"))
	{
		methodInfo.env->DeleteLocalRef(methodInfo.classID);
	}

	jint bufferSize = methodInfo.env->CallStaticIntMethod(methodInfo.classID, methodInfo.methodID);
	methodInfo.env->DeleteLocalRef(methodInfo.classID);

	#ifdef MIXING
		if(ALL_MIXING)
		{
			audioModule = (AudioModule *) new OpenSLAudioModule_AllMixing(sampleRate, 100, 128, 0);
		}
		else
		{
			std::string deviceModel = NativeDevice::getDeviceModel();
			std::string targetModel = "LG-F350";
			std::transform(deviceModel.begin(), deviceModel.end(), deviceModel.begin(), ::tolower);
			std::transform(targetModel.begin(), targetModel.end(), targetModel.begin(), ::tolower);
			auto it = deviceModel.find(targetModel);
			if (it != std::string::npos)
			{
				audioModule = (AudioModule *) new OpenSLAudioModule_ChordMixing(sampleRate, 100, 128, bufferSize/4, true);
			}
			else
			{
				audioModule = (AudioModule *) new OpenSLAudioModule_ChordMixing(sampleRate, 100, 1024, bufferSize/4, false);
			}

		}

	#endif
}

IZAudioEngine::~IZAudioEngine()
{
	if(audioModule != nullptr)
		delete (AudioModule*) audioModule;
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

int IZAudioEngine::allocPlayer(int numberOfPlayer)
{
	return ((AudioModule*)audioModule)->init(numberOfPlayer);
}

void IZAudioEngine::freePlayer()
{
	((AudioModule*)audioModule)->release();
}

long IZAudioEngine::getBufferDelayTime()
{
	if(ALL_MIXING)
		return ((AudioModule*)audioModule)->getBufferDelayTime();
	else
		return ((OpenSLAudioModule_ChordMixing*)audioModule)->getBufferDelayTime();
}

void IZAudioEngine::setVolumes(float MRVolume, float otherVolume, float myVolume)
{
	((AudioModule*)audioModule)->setVolumes(MRVolume, otherVolume, myVolume);
}

/// for MR Sound ///
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

int IZAudioEngine::playEffect(const char* pszFilePath, bool bLoop,
        float pitch, float pan, float noteVelocity)
{
	return ((AudioModule *)audioModule)->playEffect(pszFilePath, pitch, noteVelocity);
}

int IZAudioEngine::playEffect(NoteObject *noteObj)
{
	return ((AudioModule *)audioModule)->playEffect(noteObj);
}

int IZAudioEngine::startEffect()
{
	return ((AudioModule *)audioModule)->startEffect();
}
void IZAudioEngine::stopEffect(int audioID)
{
	((AudioModule *)audioModule)->stopEffect(audioID);
}

void IZAudioEngine::fadeStopEffect(int streamID, float time)
{

		return ((AudioModule *)audioModule)->fadeStopEffect(streamID, time);

}

void IZAudioEngine::stopAllEffects()
{
		((AudioModule *)audioModule)->stopAllEffects();
}

void IZAudioEngine::preloadEffect(const char* pszFilePath)
{
	((AudioModule *)audioModule)->load(pszFilePath, 0, 0);
}


void IZAudioEngine::unloadEffect(const char* pszFilePath)
{
	((AudioModule *)audioModule)->unload(pszFilePath);
}

int IZAudioEngine::markNotePlayTime(long time)
{
	return ((AudioModule *)audioModule)->markNotePlayTime(time);
}

int IZAudioEngine::unmarkNotePlayTimeAll()
{
	return ((AudioModule *)audioModule)->unmarkNotePlayTimeAll();
}

void IZAudioEngine::startRecord(const char* pszFilePath)
{
	JniMethodInfo t;

	if (JniHelper::getStaticMethodInfo(t,
		"com.izsoft.lib.IZSoftHelper",
		"startRecord",
		"(IILjava/lang/String;)V"))
	{
		jstring stringArg = t.env->NewStringUTF(pszFilePath);
		t.env->CallStaticVoidMethod(t.classID, t.methodID, 128*1024, 44100, stringArg);
		t.env->DeleteLocalRef(stringArg);
		t.env->DeleteLocalRef(t.classID);
	}
}

void IZAudioEngine::pauseRecord()
{
	JniMethodInfo t;

	if (JniHelper::getStaticMethodInfo(t,
		"com.izsoft.lib.IZSoftHelper",
		"pauseRecord",
		"()V"))
	{
		t.env->CallStaticVoidMethod(t.classID, t.methodID);
		t.env->DeleteLocalRef(t.classID);
	}
}

void IZAudioEngine::resumeRecord()
{
	JniMethodInfo t;

	if (JniHelper::getStaticMethodInfo(t,
		"com.izsoft.lib.IZSoftHelper",
		"resumeRecord",
		"()V"))
	{
		t.env->CallStaticVoidMethod(t.classID, t.methodID);
		t.env->DeleteLocalRef(t.classID);
	}
}

void IZAudioEngine::stopRecord()
{
	JniMethodInfo t;

	if (JniHelper::getStaticMethodInfo(t,
		"com.izsoft.lib.IZSoftHelper",
		"stopRecord",
		"()V"))
	{
		t.env->CallStaticVoidMethod(t.classID, t.methodID);
		t.env->DeleteLocalRef(t.classID);
	}
}
