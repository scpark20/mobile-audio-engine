#ifndef OpenSLAudioModule_AllMixing_H_
#define OpenSLAudioModule_AllMixing_H_

/*

 * OpenSLAudioModule_AllMixing.h
 *
 *  Created on: 2015. 5. 18.
 *      Author: IZSoft-PSC
 */
#include <assert.h>
#include <jni.h>
#include <string.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <pthread.h>
#include <vector>
#include <map>

#include "OpenSLAudioModule.h"
#include "../AudioCommon/Sound.h"
#include "../AudioCommon/Stream.h"
#include "../AudioCommon/AudioPool.h"
#include <sched.h>

class OpenSLAudioModule_AllMixing : public AudioModule {
	public:

	OpenSLAudioModule_AllMixing(int _systemSampleRate, int streamSize, int bufferSampleLength, int systemBufferSampleLength);
	virtual ~OpenSLAudioModule_AllMixing();

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

	// for OWN
	static void BufferQueueCallback(SLBufferQueueItf queueItf, void *pContext);

private:
	short int *shortPcmData;

    SLObjectItf slEngine;
    SLObjectItf player;
    SLObjectItf OutputMix;

    SLEngineItf EngineItf;
	SLPlayItf playItf;
	SLBufferQueueItf bufferQueueItf;
	SLVolumeItf volumeItf;
	SLPlaybackRateItf playbackRateItf;
	SLEffectSendItf effectSendItf;

	SLDataSource audioSource;
	SLDataLocator_BufferQueue bufferQueue;
	SLDataLocator_Address addressQueue;
	SLDataFormat_PCM pcm;
	SLDataSink audioSink;
	SLDataLocator_OutputMix locator_outputmix;

	SLBufferQueueState state;
	CallbackContext *context;

	AAssetManager *assetManager;
};



#endif /* OpenSLAudioModule_AllMixing_H_ */
